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
batchSize = 2#8
snapshotSkip = 256
batchIndexStart = 0
learningRate = 1e-3
schedulerDecayRate = 0.95
schedulerDecaySkip = 100000
batchCount = dataset.trainingSequenceCount // batchSize

# Create model, optimizer and scheduler
model = PullPushModel(7, 3, 16, False).to(device)
optimizer = torch.optim.Adam(model.parameters(), lr=learningRate, betas=(0.9, 0.999))
scheduler = torch.optim.lr_scheduler.ExponentialLR(optimizer, gamma=schedulerDecayRate, verbose=False)

factorSurface = 1.0
factorOpticalFlow = 1.0
factorTemporal = 1.0

# Use this directory for the visualization of loss graphs in the Tensorboard at http://localhost:6006/
checkpointDirectory += 'Supervised/'
summaryWriter = SummaryWriter(log_dir=checkpointDirectory)

# Try to load the last checkpoint and continue training from there
if os.path.exists(checkpointDirectory + checkpointNameStart + checkpointNameEnd):
    print('Loading existing checkpoint from "' + checkpointDirectory + checkpointNameStart + checkpointNameEnd + '"')
    checkpoint = torch.load(checkpointDirectory + checkpointNameStart + checkpointNameEnd)
    epoch = checkpoint['Epoch']
    batchIndexStart = checkpoint['BatchIndexStart']
    model.load_state_dict(checkpoint['Model'])
    optimizerGenerator.load_state_dict(checkpoint['Optimizer'])
    schedulerGenerator.load_state_dict(checkpoint['Scheduler'])

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
            inputOpticalFlowForward = ConvertMotionVectorIntoZeroToOneRange(sequence['PointsSparseOpticalFlowForward'][frameIndex])
            input = torch.cat([sequence['PointsSparseForeground'][frameIndex], sequence['PointsSparseDepth'][frameIndex], sequence['PointsSparseNormalScreen'][frameIndex], inputOpticalFlowForward], dim=1)

            # Generate the output
            output = model(input, None)

            # Need to bring optical flow motion vectors from pixel space into [0, 1] range
            # TODO: Mabye use the grid that corresponds to the motion instead?
            targetOpticalFlowForward = ConvertMotionVectorIntoZeroToOneRange(sequence['MeshOpticalFlowForward'][frameIndex])
            target = torch.cat([sequence['PointsSparseSurface'][frameIndex], targetOpticalFlowForward], dim=1)

            inputs.append(input)
            outputs.append(output)
            targets.append(target)

        inputs = torch.stack(inputs, dim=0)
        outputs = torch.stack(outputs, dim=0)
        targets = torch.stack(targets, dim=0)

        # Accumulate loss terms over triplets in the sequence
        lossSurface = []
        lossOpticalFlow = []
        lossTemporal = []

        # TODO: Add temporal information using warped frames (e.g. inputPreviousWarpedForward, inputNextWarpedBackward)
        for tripletFrameIndex in range(1, dataset.sequenceFrameCount - 1):
            motionVectorPreviousToCurrent = sequence['MeshOpticalFlowForward'][tripletFrameIndex]
            motionVectorNextToCurrent = sequence['MeshOpticalFlowBackward'][tripletFrameIndex + 1]

            inputPrevious = inputs[tripletFrameIndex - 1]
            inputPreviousWarped = WarpImage(inputPrevious, motionVectorPreviousToCurrent)
            input = inputs[tripletFrameIndex]
            inputNext = inputs[tripletFrameIndex + 1]
            inputNextWarped = WarpImage(inputNext, motionVectorNextToCurrent)

            outputPrevious = outputs[tripletFrameIndex - 1]
            outputPreviousWarped = WarpImage(outputPrevious, motionVectorPreviousToCurrent)
            output = outputs[tripletFrameIndex]
            outputNext = outputs[tripletFrameIndex + 1]
            outputNextWarped = WarpImage(outputNext, motionVectorNextToCurrent)

            targetPrevious = targets[tripletFrameIndex - 1]
            targetPreviousWarped = WarpImage(targetPrevious, motionVectorPreviousToCurrent)
            target = targets[tripletFrameIndex]
            targetNext = targets[tripletFrameIndex + 1]
            targetNextWarped = WarpImage(targetNext, motionVectorNextToCurrent)

            sequenceInput = torch.cat([inputPreviousWarped, input, inputNextWarped], dim=1)
            sequenceOutput = torch.cat([outputPreviousWarped, output, outputNextWarped], dim=1)
            sequenceTarget = torch.cat([targetPreviousWarped, target, targetNextWarped], dim=1)

            # Add supervised generator loss terms
            # TODO: Temporal loss term should consider occlusion/invalid motion ouf warped output (remove these areas before computing the MSE)
            lossSurfaceTriplet = factorSurface * torch.nn.functional.binary_cross_entropy(output[:, 0:1, :, :], target[:, 0:1, :, :], reduction='mean')
            lossOpticalFlowTriplet = factorOpticalFlow * torch.nn.functional.mse_loss(output[:, 1:3, :, :], target[:, 1:3, :, :], reduction='mean')
            lossTemporalPrevious = factorTemporal * torch.nn.functional.mse_loss(outputPreviousWarped, output, reduction='mean')
            lossTemporalNext = factorTemporal * torch.nn.functional.mse_loss(outputNextWarped, output, reduction='mean')
            lossTemporalTriplet = lossTemporalPrevious + lossTemporalNext

            lossSurface.append(lossSurfaceTriplet)
            lossOpticalFlow.append(lossOpticalFlowTriplet)
            lossTemporal.append(lossTemporalTriplet)

        # Average all loss terms across the sequence and combine into final loss
        lossSurface = torch.stack(lossSurface, dim=0).mean()
        lossOpticalFlow = torch.stack(lossOpticalFlow, dim=0).mean()
        lossTemporal = torch.stack(lossTemporal, dim=0).mean()
        loss = torch.stack([lossSurface, lossOpticalFlow, lossTemporal], dim=0).mean()

        # Train the model
        model.zero_grad()
        loss.backward(retain_graph=False)
        optimizer.step()

        # Plot generator losses
        summaryWriter.add_scalar('Loss/Loss', loss, iteration)
        summaryWriter.add_scalar('Loss/Loss Surface', lossSurface, iteration)
        summaryWriter.add_scalar('Loss/Loss Optical Flow', lossOpticalFlow, iteration)
        summaryWriter.add_scalar('Loss/Loss Temporal', lossTemporal, iteration)

        # Need to detach previous output for next model iteration
        model.DetachPrevious()

        # Update learning rate using schedulers
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

            meshColor = sequence['MeshColor'][snapshotFrameIndex][snapshotSampleIndex]

            inputForeground = inputs[snapshotFrameIndex, snapshotSampleIndex, 0:1, :, :]
            inputDepth = inputs[snapshotFrameIndex, snapshotSampleIndex, 1:2, :, :]
            inputNormal = inputs[snapshotFrameIndex, snapshotSampleIndex, 2:5, :, :]
            inputOpticalFlowForward = inputs[snapshotFrameIndex, snapshotSampleIndex, 5:7, :, :]

            outputSurface = outputs[snapshotFrameIndex, snapshotSampleIndex, 0:1, :, :]
            outputOpticalFlowForward = outputs[snapshotFrameIndex, snapshotSampleIndex, 1:3, :, :]
            outputMotionVectorForward = ConvertMotionVectorIntoPixelRange(outputOpticalFlowForward.unsqueeze(0))
            outputMeshWarped = WarpImage(meshColor.unsqueeze(0), outputMotionVectorForward).squeeze(0)
            outputOcclusion = EstimateOcclusion(outputMotionVectorForward).squeeze(0)
            outputMeshWarped *= outputOcclusion

            targetSurface = targets[snapshotFrameIndex, snapshotSampleIndex, 0:1, :, :]
            targetOpticalFlowForward = targets[snapshotFrameIndex, snapshotSampleIndex, 1:3, :, :]
            targetMotionVectorForward = ConvertMotionVectorIntoPixelRange(targetOpticalFlowForward.unsqueeze(0))
            targetMeshWarped = WarpImage(meshColor.unsqueeze(0), targetMotionVectorForward).squeeze(0)
            targetOcclusion = EstimateOcclusion(targetMotionVectorForward).squeeze(0)
            targetMeshWarped *= targetOcclusion

            fig = plt.figure(figsize=(4 * inputDepth.size(2), 4 * inputDepth.size(1)), dpi=1)
            fig.add_subplot(4, 4, 1).title#.set_text('Input Foreground')
            plt.imshow(TensorToImage(inputForeground))
            plt.axis('off')
            fig.add_subplot(4, 4, 2).title#.set_text('Input Depth')
            plt.imshow(TensorToImage(inputDepth))
            plt.axis('off')
            fig.add_subplot(4, 4, 3).title#.set_text('Input Normal')
            plt.imshow(TensorToImage(inputNormal))
            plt.axis('off')
            fig.add_subplot(4, 4, 4).title#.set_text('Input Optical Flow')
            plt.imshow(FlowToImage(inputOpticalFlowForward))
            plt.axis('off')

            fig.add_subplot(4, 4, 5).title#.set_text('Output Surface')
            plt.imshow(TensorToImage(outputSurface))
            plt.axis('off')
            fig.add_subplot(4, 4, 8).title#.set_text('Output Optical Flow')
            plt.imshow(FlowToImage(outputOpticalFlowForward))
            plt.axis('off')
            fig.add_subplot(4, 4, 6).title#.set_text('Output Mesh Warped')
            plt.imshow(TensorToImage(outputMeshWarped))
            plt.axis('off')

            fig.add_subplot(4, 4, 9).title#.set_text('Target Surface')
            plt.imshow(TensorToImage(targetSurface))
            plt.axis('off')
            fig.add_subplot(4, 4, 12).title#.set_text('Target Optical Flow')
            plt.imshow(FlowToImage(targetOpticalFlowForward))
            plt.axis('off')
            fig.add_subplot(4, 4, 7).title#.set_text('Target Mesh Warped')
            plt.imshow(TensorToImage(targetMeshWarped))
            plt.axis('off')

            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)

            summaryWriter.add_figure('Snapshots/Epoch' + str(epoch), plt.gcf(), iteration)

            # Save a checkpoint to file
            checkpoint = {
                'Epoch' : epoch,
                'BatchIndexStart' : batchIndex + 1,
                'Model' : model.state_dict(),
                'Optimizer' : optimizer.state_dict(),
                'Scheduler' : scheduler.state_dict(),
            }

            torch.save(checkpoint, checkpointDirectory + checkpointNameStart + checkpointNameEnd)
            torch.save(checkpoint, checkpointDirectory + checkpointNameStart + str(epoch) + checkpointNameEnd)

    # Move to next epoch
    numpy.random.shuffle(randomIndices)
    torch.cuda.empty_cache()
    batchIndexStart = 0
    epoch += 1