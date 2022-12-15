import time
import threading
import matplotlib
import matplotlib.pyplot as plt
from torch.utils.tensorboard import SummaryWriter
from Dataset import *
from Model import *

# Use different matplotlib backend to avoid weird error
matplotlib.use('Agg')

dataset = Dataset(directory='G:/PointCloudEngineDataset/', sequenceFrameCount=3)

checkpointDirectory = 'G:/PointCloudEngineCheckpoints/'
checkpointNameStart = 'Checkpoint'
checkpointNameEnd = '.pt'

epoch = 0
batchSize = 4
snapshotSkip = 256
batchIndexStart = 0
learningRate = 1e-3
schedulerDecayRate = 0.95
schedulerDecaySkip = 100000
batchCount = dataset.trainingSequenceCount // batchSize

# Create model, optimizer and scheduler
model = PullPushModel(8, 8, 16).to(device)
optimizer = torch.optim.Adam(model.parameters(), lr=learningRate, betas=(0.9, 0.999))
scheduler = torch.optim.lr_scheduler.ExponentialLR(optimizer, gamma=schedulerDecayRate, verbose=False)

factorOcclusion = 1.0
factorDepth = 1.0
factorColor = 1.0
factorNormal = 1.0

# Use this directory for the visualization of loss graphs in the Tensorboard at http://localhost:6006/
checkpointDirectory += 'Sequence with Flow Inpainting Only/'
summaryWriter = SummaryWriter(log_dir=checkpointDirectory)

# Try to load the last checkpoint and continue training from there
if os.path.exists(checkpointDirectory + checkpointNameStart + checkpointNameEnd):
    print('Loading existing checkpoint from "' + checkpointDirectory + checkpointNameStart + checkpointNameEnd + '"')
    checkpoint = torch.load(checkpointDirectory + checkpointNameStart + checkpointNameEnd)

    epoch = checkpoint['Epoch']
    batchIndexStart = checkpoint['BatchIndexStart']
    model.load_state_dict(checkpoint['Model'])
    optimizer.load_state_dict(checkpoint['Optimizer'])
    scheduler.load_state_dict(checkpoint['Scheduler'])

# Make order of training sequences random but predictable
numpy.random.seed(0)
randomIndices = numpy.arange(dataset.trainingSequenceCount)

# Restore state of random order
for i in range(epoch + 1):
    numpy.random.shuffle(randomIndices)

# Use threads to concurrently preload the next items in the dataset (slightly reduces sequence loading bottleneck)
preloadThreadCount = batchSize
preloadThreads = [None] * preloadThreadCount
batchNextSequence = [None] * batchSize

for threadIndex in range(preloadThreadCount):
    batchNextSequence[threadIndex] = dataset.GetTrainingSequence(randomIndices[batchIndexStart * batchSize + threadIndex])

# Train for infinite epochs
while True:
    print('Learning rate: ' + str(scheduler.get_last_lr()[0]))

    # Loop over the dataset batches
    for batchIndex in range(batchIndexStart, batchCount):
        iteration = epoch * batchCount + batchIndex

        # Wait for all the data to be loaded
        for threadIndex in range(preloadThreadCount):
            if preloadThreads[threadIndex] is not None:
                preloadThreads[threadIndex].join()
                preloadThreads[threadIndex] = None

        # Move data into a dictionary using render modes as keys and value tensors of shape (sequenceFrameCount, batchSize, C, H, W)
        sequence = {}
        
        for renderMode in dataset.renderModes:
            sequence[renderMode] = []

        for frameIndex in range(dataset.sequenceFrameCount):
            for renderMode in dataset.renderModes:
                tensorsBatch = []

                for sampleIndex in range(batchSize):
                    tensorsBatch.append(batchNextSequence[sampleIndex][frameIndex][renderMode])

                sequence[renderMode].append(torch.stack(tensorsBatch, dim=0))

        for renderMode in dataset.renderModes:
            sequence[renderMode] = torch.stack(sequence[renderMode], dim=0)

        # Reset for next iteration
        batchNextSequence = [None] * batchSize

        def preload_thread_load_next(batchNextSequence, threadIndex, sequenceIndex):
            batchNextSequence[threadIndex] = dataset.GetTrainingSequence(sequenceIndex)

        for threadIndex in range(preloadThreadCount):
            if preloadThreads[threadIndex] is None:
                sequenceIndex = randomIndices[batchIndex * batchSize + threadIndex]
                preloadThreads[threadIndex] = threading.Thread(target=preload_thread_load_next, args=(batchNextSequence, threadIndex, sequenceIndex))
                preloadThreads[threadIndex].start()

        # Generate the output frames either in chronological or in reverse order (starting with the last frame, used to improve temporal stability)
        frameGenerationOrder = range(dataset.sequenceFrameCount) if (iteration % 2 == 0) else range(dataset.sequenceFrameCount -1, -1, -1)

        # Create the inputs/outputs/targets for each frame in the sequence
        inputs = []
        outputs = []
        targets = []

        for frameIndex in frameGenerationOrder:
            input = torch.cat([sequence['PointsSparseForeground'][frameIndex], sequence['PointsSparseDepth'][frameIndex], sequence['PointsSparseColor'][frameIndex], sequence['PointsSparseNormalScreen'][frameIndex]], dim=1)

            # Keep the input surface pixels (should fix issue that already "perfect" input does get blurred a lot)
            output = model(input)
            outputOcclusion = output[:, 0:1, :, :]
            outputDepthColorNormal = output[:, 1:8, :, :]
            maskSurface = sequence['PointsSparseForeground'][frameIndex] * (1.0 - outputOcclusion)
            outputDepthColorNormal = maskSurface * input[:, 1:8, :, :] + (1.0 - maskSurface) * outputDepthColorNormal
            output = torch.cat([outputOcclusion, outputDepthColorNormal], dim=1)

            target = torch.cat([sequence['PointsSparseOcclusion'][frameIndex], sequence['MeshDepth'][frameIndex], sequence['MeshColor'][frameIndex], sequence['MeshNormalScreen'][frameIndex]], dim=1)

            inputs.append(input)
            outputs.append(output)
            targets.append(target)

        inputs = torch.stack(inputs, dim=0)
        outputs = torch.stack(outputs, dim=0)
        targets = torch.stack(targets, dim=0)

        # Accumulate loss terms over triplets in the sequence
        lossOcclusion = []
        lossDepth = []
        lossColor = []
        lossNormal = []

        for tripletFrameIndex in range(1, dataset.sequenceFrameCount - 1):
            inputPrevious = inputs[tripletFrameIndex - 1]
            input = inputs[tripletFrameIndex]
            inputNext = inputs[tripletFrameIndex + 1]

            outputPrevious = outputs[tripletFrameIndex - 1]
            output = outputs[tripletFrameIndex]
            outputNext = outputs[tripletFrameIndex + 1]

            targetPrevious = targets[tripletFrameIndex - 1]
            target = targets[tripletFrameIndex]
            targetNext = targets[tripletFrameIndex + 1]

            # TODO: Add temporal loss term using warped frames
            lossOcclusionTriplet = factorOcclusion * torch.nn.functional.binary_cross_entropy(output[:, 0:1, :, :], target[:, 0:1, :, :], reduction='mean')
            lossDepthTriplet = factorDepth * torch.nn.functional.mse_loss(output[:, 1:2, :, :], target[:, 1:2, :, :], reduction='mean')
            lossColorTriplet = factorColor * torch.nn.functional.mse_loss(output[:, 2:5, :, :], target[:, 2:5, :, :], reduction='mean')
            lossNormalTriplet = factorNormal * torch.nn.functional.mse_loss(output[:, 5:8, :, :], target[:, 5:8, :, :], reduction='mean')

            lossOcclusion.append(lossOcclusionTriplet)
            lossDepth.append(lossDepthTriplet)
            lossColor.append(lossColorTriplet)
            lossNormal.append(lossNormalTriplet)

        # Average all loss terms across the sequence and combine into final loss
        lossOcclusion = torch.stack(lossOcclusion, dim=0).mean()
        lossDepth = torch.stack(lossDepth, dim=0).mean()
        lossColor = torch.stack(lossColor, dim=0).mean()
        lossNormal = torch.stack(lossNormal, dim=0).mean()
        loss = torch.stack([lossOcclusion, lossDepth, lossColor, lossNormal], dim=0).mean()

        # Update model parameters
        model.zero_grad()
        loss.backward(retain_graph=False)
        optimizer.step()

        # Plot loss
        summaryWriter.add_scalar('Losses/Loss', loss, iteration)
        summaryWriter.add_scalar('Losses/Loss Occlusion', lossOcclusion, iteration)
        summaryWriter.add_scalar('Losses/Loss Depth', lossDepth, iteration)
        summaryWriter.add_scalar('Losses/Loss Color', lossColor, iteration)
        summaryWriter.add_scalar('Losses/Loss Normal', lossNormal, iteration)

        if iteration % schedulerDecaySkip == (schedulerDecaySkip - 1):
            scheduler.step()
            print('Learning rate: ' + str(scheduler.get_last_lr()[0]))

        # Print progress, save checkpoints, create snapshots and plot loss graphs in certain intervals
        if iteration % snapshotSkip == (snapshotSkip - 1):
            snapshotIndex = (iteration // snapshotSkip)
            snapshotSampleIndex = snapshotIndex % batchSize
            snapshotFrameIndex = 1 + (snapshotIndex % (dataset.sequenceFrameCount - 2))

            progress = 'Epoch:\t' + str(epoch) + '\t' + str(int(100 * (batchIndex / batchCount))) + '%'
            print(progress)

            inputForeground = inputs[snapshotFrameIndex, snapshotSampleIndex, 0:1, :, :]
            inputDepth = inputs[snapshotFrameIndex, snapshotSampleIndex, 1:2, :, :]
            inputColor = inputs[snapshotFrameIndex, snapshotSampleIndex, 2:5, :, :]
            inputNormal = inputs[snapshotFrameIndex, snapshotSampleIndex, 5:8, :, :]

            outputOcclusion = outputs[snapshotFrameIndex, snapshotSampleIndex, 0:1, :, :]
            outputDepth = outputs[snapshotFrameIndex, snapshotSampleIndex, 1:2, :, :]
            outputColor = outputs[snapshotFrameIndex, snapshotSampleIndex, 2:5, :, :]
            outputNormal = outputs[snapshotFrameIndex, snapshotSampleIndex, 5:8, :, :]

            targetOcclusion = targets[snapshotFrameIndex, snapshotSampleIndex, 0:1, :, :]
            targetDepth = targets[snapshotFrameIndex, snapshotSampleIndex, 1:2, :, :]
            targetColor = targets[snapshotFrameIndex, snapshotSampleIndex, 2:5, :, :]
            targetNormal = targets[snapshotFrameIndex, snapshotSampleIndex, 5:8, :, :]

            splatsDepth = sequence['SplatsSparseDepth'][snapshotFrameIndex][snapshotSampleIndex]
            splatsColor = sequence['SplatsSparseColor'][snapshotFrameIndex][snapshotSampleIndex]
            splatsNormal = sequence['SplatsSparseNormalScreen'][snapshotFrameIndex][snapshotSampleIndex]

            pullPushDepth = sequence['PullPushDepth'][snapshotFrameIndex][snapshotSampleIndex]
            pullPushColor = sequence['PullPushColor'][snapshotFrameIndex][snapshotSampleIndex]
            pullPushNormal = sequence['PullPushNormalScreen'][snapshotFrameIndex][snapshotSampleIndex]

            fig = plt.figure(figsize=(4 * inputDepth.size(2), 5 * inputDepth.size(1)), dpi=1)
            fig.add_subplot(5, 4, 1).title#.set_text('Input Foreground')
            plt.imshow(TensorToImage(inputForeground))
            plt.axis('off')
            fig.add_subplot(5, 4, 2).title#.set_text('Input Color')
            plt.imshow(TensorToImage(inputColor))
            plt.axis('off')
            fig.add_subplot(5, 4, 3).title#.set_text('Input Depth')
            plt.imshow(TensorToImage(inputDepth))
            plt.axis('off')
            fig.add_subplot(5, 4, 4).title#.set_text('Input Normal')
            plt.imshow(TensorToImage(inputNormal))
            plt.axis('off')

            fig.add_subplot(5, 4, 5).title#.set_text('Output Occlusion')
            plt.imshow(TensorToImage(outputOcclusion))
            plt.axis('off')
            fig.add_subplot(5, 4, 6).title#.set_text('Output Color')
            plt.imshow(TensorToImage(outputColor))
            plt.axis('off')
            fig.add_subplot(5, 4, 7).title#.set_text('Output Depth')
            plt.imshow(TensorToImage(outputDepth))
            plt.axis('off')
            fig.add_subplot(5, 4, 8).title#.set_text('Output Normal')
            plt.imshow(TensorToImage(outputNormal))
            plt.axis('off')

            fig.add_subplot(5, 4, 9).title#.set_text('Target Occlusion')
            plt.imshow(TensorToImage(targetOcclusion))
            plt.axis('off')
            fig.add_subplot(5, 4, 10).title#.set_text('Target Color')
            plt.imshow(TensorToImage(targetColor))
            plt.axis('off')
            fig.add_subplot(5, 4, 11).title#.set_text('Target Depth')
            plt.imshow(TensorToImage(targetDepth))
            plt.axis('off')
            fig.add_subplot(5, 4, 12).title#.set_text('Target Normal')
            plt.imshow(TensorToImage(targetNormal))
            plt.axis('off')

            fig.add_subplot(5, 4, 14).title#.set_text('Splats Color')
            plt.imshow(TensorToImage(splatsColor))
            plt.axis('off')
            fig.add_subplot(5, 4, 15).title#.set_text('Splats Depth')
            plt.imshow(TensorToImage(splatsDepth))
            plt.axis('off')
            fig.add_subplot(5, 4, 16).title#.set_text('Splats Normal')
            plt.imshow(TensorToImage(splatsNormal))
            plt.axis('off')

            fig.add_subplot(5, 4, 18).title#.set_text('Pull Push Color')
            plt.imshow(TensorToImage(pullPushColor))
            plt.axis('off')
            fig.add_subplot(5, 4, 19).title#.set_text('Pull Push Depth')
            plt.imshow(TensorToImage(pullPushDepth))
            plt.axis('off')
            fig.add_subplot(5, 4, 20).title#.set_text('Pull Push Normal')
            plt.imshow(TensorToImage(pullPushNormal))
            plt.axis('off')

            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)

            summaryWriter.add_figure('Snapshots/Epoch' + str(epoch), plt.gcf(), iteration)

            framePrevious = sequence['MeshColor'][snapshotFrameIndex - 1][snapshotSampleIndex]
            frameCurrent = sequence['MeshColor'][snapshotFrameIndex][snapshotSampleIndex]
            frameNext = sequence['MeshColor'][snapshotFrameIndex + 1][snapshotSampleIndex]

            motionVectorPreviousToCurrent = sequence['MeshOpticalFlowForward'][snapshotFrameIndex][snapshotSampleIndex]
            motionVectorNextToCurrent = sequence['MeshOpticalFlowBackward'][snapshotFrameIndex + 1][snapshotSampleIndex]
            framePreviousWarpedToCurrent = WarpImage(framePrevious.unsqueeze(0), motionVectorPreviousToCurrent.unsqueeze(0)).squeeze(0)
            frameNextWarpedToCurrent = WarpImage(frameNext.unsqueeze(0), motionVectorNextToCurrent.unsqueeze(0)).squeeze(0)
            frameOverlay = (framePreviousWarpedToCurrent + frameCurrent + frameNextWarpedToCurrent) / 3.0

            occlusionPrevious = sequence['MeshOpticalFlowBackward'][snapshotFrameIndex][snapshotSampleIndex]
            occlusionPrevious = EstimateOcclusion(occlusionPrevious.unsqueeze(0)).squeeze(0)
            framePreviousWarpedToCurrentOccluded = occlusionPrevious * framePreviousWarpedToCurrent

            occlusionNext = sequence['MeshOpticalFlowForward'][snapshotFrameIndex + 1][snapshotSampleIndex]
            occlusionNext = EstimateOcclusion(occlusionNext.unsqueeze(0)).squeeze(0)
            frameNextWarpedToCurrentOccluded = occlusionNext * frameNextWarpedToCurrent
            frameOverlayOccluded = (framePreviousWarpedToCurrentOccluded + frameCurrent + frameNextWarpedToCurrentOccluded) / 3.0

            fig = plt.figure(figsize=(3 * frameCurrent.size(2), 3 * frameCurrent.size(1)), dpi=1)
            fig.add_subplot(3, 3, 1).title#.set_text('Frame Previous')
            plt.imshow(TensorToImage(framePrevious))
            plt.axis('off')
            fig.add_subplot(3, 3, 2).title#.set_text('Frame Current')
            plt.imshow(TensorToImage(frameCurrent))
            plt.axis('off')
            fig.add_subplot(3, 3, 3).title#.set_text('Frame Next')
            plt.imshow(TensorToImage(frameNext))
            plt.axis('off')

            fig.add_subplot(3, 3, 4).title#.set_text('Frame Previous Warped')
            plt.imshow(TensorToImage(framePreviousWarpedToCurrent))
            plt.axis('off')
            fig.add_subplot(3, 3, 5).title#.set_text('Frame Overlay')
            plt.imshow(TensorToImage(frameOverlay))
            plt.axis('off')
            fig.add_subplot(3, 3, 6).title#.set_text('Frame Next Warped')
            plt.imshow(TensorToImage(frameNextWarpedToCurrent))
            plt.axis('off')

            fig.add_subplot(3, 3, 7).title#.set_text('Frame Previous Warped Occluded')
            plt.imshow(TensorToImage(framePreviousWarpedToCurrentOccluded))
            plt.axis('off')
            fig.add_subplot(3, 3, 8).title#.set_text('Frame Overlay Occluded')
            plt.imshow(TensorToImage(frameOverlayOccluded))
            plt.axis('off')
            fig.add_subplot(3, 3, 9).title#.set_text('Frame Next Warped Occluded')
            plt.imshow(TensorToImage(frameNextWarpedToCurrentOccluded))
            plt.axis('off')

            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)

            summaryWriter.add_figure('SnapshotsFlow/Epoch' + str(epoch), plt.gcf(), iteration)

            # Save a checkpoint to file
            checkpoint = {
                'Epoch' : epoch,
                'BatchIndexStart' : batchIndex + 1,
                'Model' : model.state_dict(),
                'Optimizer' : optimizer.state_dict(),
                'Scheduler' : scheduler.state_dict()
            }

            checkpointFilename = checkpointDirectory + checkpointNameStart + checkpointNameEnd
            torch.save(checkpoint, checkpointFilename + '.tmp')
            time.sleep(0.01)
            os.replace(checkpointFilename + '.tmp', checkpointFilename)

    # Move to next epoch
    numpy.random.shuffle(randomIndices)
    torch.cuda.empty_cache()
    batchIndexStart = 0
    epoch += 1