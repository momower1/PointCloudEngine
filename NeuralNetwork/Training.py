import time
import threading
import matplotlib
import matplotlib.pyplot as plt
from torch.utils.tensorboard import SummaryWriter
from Dataset import *
from Model import *

# Use different matplotlib backend to avoid weird error
matplotlib.use('Agg')

dataset = Dataset(directory='G:/PointCloudEngineDatasetFlow/', sequenceFrameCount=3)

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
modelOcclusion = PullPushModel(5, 1, 16).to(device)
optimizerOcclusion = torch.optim.Adam(modelOcclusion.parameters(), lr=learningRate, betas=(0.9, 0.999))
schedulerOcclusion = torch.optim.lr_scheduler.ExponentialLR(optimizerOcclusion, gamma=schedulerDecayRate, verbose=False)

model = PullPushModel(8, 7, 16).to(device)
optimizer = torch.optim.Adam(model.parameters(), lr=learningRate, betas=(0.9, 0.999))
scheduler = torch.optim.lr_scheduler.ExponentialLR(optimizer, gamma=schedulerDecayRate, verbose=False)

factorDepth = 1.0
factorColor = 3.0
factorNormal = 1.0

# Use this directory for the visualization of loss graphs in the Tensorboard at http://localhost:6006/
checkpointDirectory += 'Sequence with Flow/'
summaryWriter = SummaryWriter(log_dir=checkpointDirectory)

# Try to load the last checkpoint and continue training from there
if os.path.exists(checkpointDirectory + checkpointNameStart + checkpointNameEnd):
    print('Loading existing checkpoint from "' + checkpointDirectory + checkpointNameStart + checkpointNameEnd + '"')
    checkpoint = torch.load(checkpointDirectory + checkpointNameStart + checkpointNameEnd)

    epoch = checkpoint['Epoch']
    batchIndexStart = checkpoint['BatchIndexStart']
    modelOcclusion.load_state_dict(checkpoint['ModelOcclusion'])
    optimizerOcclusion.load_state_dict(checkpoint['OptimizerOcclusion'])
    schedulerOcclusion.load_state_dict(checkpoint['SchedulerOcclusion'])
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

        # TODO: Actually use whole sequence
        tensors = {}

        for renderMode in dataset.renderModes:
            tensors[renderMode] = sequence[renderMode][0]

        # Train the occlusion network
        inputOcclusion = torch.cat([tensors['PointsSparseForeground'], tensors['PointsSparseDepth'], tensors['PointsSparseNormalScreen']], dim=1)
        targetOcclusion = tensors['PointsSparseOcclusion']
        outputOcclusion = modelOcclusion(inputOcclusion)

        # There should be no values for non-foreground pixels
        outputOcclusion = outputOcclusion * tensors['PointsSparseForeground']

        lossOcclusion = torch.nn.functional.binary_cross_entropy(outputOcclusion, targetOcclusion, reduction='mean')
        modelOcclusion.zero_grad()
        lossOcclusion.backward(retain_graph=False)
        optimizerOcclusion.step()

        # Plot loss
        summaryWriter.add_scalar('Occlusion/Loss', lossOcclusion, iteration)

        # Train the inpainting network
        input = torch.cat([tensors['PointsSparseForeground'], tensors['PointsSparseDepth'], tensors['PointsSparseColor'], tensors['PointsSparseNormalScreen']], dim=1)

        outputOcclusionMask = (outputOcclusion.detach() > 0.5).repeat(1, model.inChannels, 1, 1)
        input[outputOcclusionMask] = 0.0

        target = torch.cat([tensors['MeshDepth'], tensors['MeshColor'], tensors['MeshNormalScreen']], dim=1)
        output = model(input)

        # Keep the input surface pixels (should fix issue that already "perfect" input does get blurred a lot)
        maskSurface = tensors['PointsSparseForeground'] * outputOcclusion.le(0.5).float()
        output = maskSurface * input[:, 1:8, :, :] + (1.0 - maskSurface) * output

        lossDepth = factorDepth * torch.nn.functional.mse_loss(output[:, 0:1, :, :], target[:, 0:1, :, :], reduction='mean')
        lossColor = factorColor * torch.nn.functional.mse_loss(output[:, 1:4, :, :], target[:, 1:4, :, :], reduction='mean')
        lossNormal = factorNormal * torch.nn.functional.mse_loss(output[:, 4:7, :, :], target[:, 4:7, :, :], reduction='mean')

        loss = torch.stack([lossDepth, lossColor, lossNormal], dim=0).mean()
        model.zero_grad()
        loss.backward(retain_graph=False)
        optimizer.step()

        # Plot loss
        summaryWriter.add_scalar('Inpainting/Loss', loss, iteration)
        summaryWriter.add_scalar('Inpainting/Loss Depth', lossDepth, iteration)
        summaryWriter.add_scalar('Inpainting/Loss Color', lossColor, iteration)
        summaryWriter.add_scalar('Inpainting/Loss Normal', lossNormal, iteration)

        if iteration % schedulerDecaySkip == (schedulerDecaySkip - 1):
            schedulerOcclusion.step()
            scheduler.step()
            print('Learning rate: ' + str(scheduler.get_last_lr()[0]))

        # Print progress, save checkpoints, create snapshots and plot loss graphs in certain intervals
        if iteration % snapshotSkip == (snapshotSkip - 1):
            snapshotIndex = (iteration // snapshotSkip)
            snapshotSampleIndex = snapshotIndex % batchSize

            progress = 'Epoch:\t' + str(epoch) + '\t' + str(int(100 * (batchIndex / batchCount))) + '%'
            print(progress)

            inputDepth = tensors['PointsSparseDepth'][snapshotSampleIndex]
            inputColor = tensors['PointsSparseColor'][snapshotSampleIndex]
            inputNormal = tensors['PointsSparseNormalScreen'][snapshotSampleIndex]

            inputSurfaceDepth = input[snapshotSampleIndex, 1:2, :, :]
            inputSurfaceColor = input[snapshotSampleIndex, 2:5, :, :]
            inputSurfaceNormal = input[snapshotSampleIndex, 5:8, :, :]

            targetDepth = target[snapshotSampleIndex, 0:1, :, :]
            targetColor = target[snapshotSampleIndex, 1:4, :, :]
            targetNormal = target[snapshotSampleIndex, 4:7, :, :]

            outputDepth = output[snapshotSampleIndex, 0:1, :, :]
            outputColor = output[snapshotSampleIndex, 1:4, :, :]
            outputNormal = output[snapshotSampleIndex, 4:7, :, :]

            splatsDepth = tensors['SplatsSparseDepth'][snapshotSampleIndex]
            splatsColor = tensors['SplatsSparseColor'][snapshotSampleIndex]
            splatsNormal = tensors['SplatsSparseNormalScreen'][snapshotSampleIndex]

            pullPushDepth = tensors['PullPushDepth'][snapshotSampleIndex]
            pullPushColor = tensors['PullPushColor'][snapshotSampleIndex]
            pullPushNormal = tensors['PullPushNormalScreen'][snapshotSampleIndex]

            fig = plt.figure(figsize=(3 * inputDepth.size(2), 6 * inputDepth.size(1)), dpi=1)
            fig.add_subplot(6, 3, 1).title#.set_text('Input Color')
            plt.imshow(TensorToImage(inputColor))
            plt.axis('off')
            fig.add_subplot(6, 3, 2).title#.set_text('Input Depth')
            plt.imshow(TensorToImage(inputDepth))
            plt.axis('off')
            fig.add_subplot(6, 3, 3).title#.set_text('Input Normal')
            plt.imshow(TensorToImage(inputNormal))
            plt.axis('off')

            fig.add_subplot(6, 3, 4).title#.set_text('Input Color')
            plt.imshow(TensorToImage(inputSurfaceColor))
            plt.axis('off')
            fig.add_subplot(6, 3, 5).title#.set_text('Input Depth')
            plt.imshow(TensorToImage(inputSurfaceDepth))
            plt.axis('off')
            fig.add_subplot(6, 3, 6).title#.set_text('Input Normal')
            plt.imshow(TensorToImage(inputSurfaceNormal))
            plt.axis('off')

            fig.add_subplot(6, 3, 7).title#.set_text('Output Color')
            plt.imshow(TensorToImage(outputColor))
            plt.axis('off')
            fig.add_subplot(6, 3, 8).title#.set_text('Output Depth')
            plt.imshow(TensorToImage(outputDepth))
            plt.axis('off')
            fig.add_subplot(6, 3, 9).title#.set_text('Output Normal')
            plt.imshow(TensorToImage(outputNormal))
            plt.axis('off')

            fig.add_subplot(6, 3, 10).title#.set_text('Target Color')
            plt.imshow(TensorToImage(targetColor))
            plt.axis('off')
            fig.add_subplot(6, 3, 11).title#.set_text('Target Depth')
            plt.imshow(TensorToImage(targetDepth))
            plt.axis('off')
            fig.add_subplot(6, 3, 12).title#.set_text('Target Normal')
            plt.imshow(TensorToImage(targetNormal))
            plt.axis('off')

            fig.add_subplot(6, 3, 13).title#.set_text('Splats Color')
            plt.imshow(TensorToImage(splatsColor))
            plt.axis('off')
            fig.add_subplot(6, 3, 14).title#.set_text('Splats Depth')
            plt.imshow(TensorToImage(splatsDepth))
            plt.axis('off')
            fig.add_subplot(6, 3, 15).title#.set_text('Splats Normal')
            plt.imshow(TensorToImage(splatsNormal))
            plt.axis('off')

            fig.add_subplot(6, 3, 16).title#.set_text('Pull Push Color')
            plt.imshow(TensorToImage(pullPushColor))
            plt.axis('off')
            fig.add_subplot(6, 3, 17).title#.set_text('Pull Push Depth')
            plt.imshow(TensorToImage(pullPushDepth))
            plt.axis('off')
            fig.add_subplot(6, 3, 18).title#.set_text('Pull Push Normal')
            plt.imshow(TensorToImage(pullPushNormal))
            plt.axis('off')

            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)

            summaryWriter.add_figure('Snapshots/Epoch' + str(epoch), plt.gcf(), iteration)

            inputDepth = inputOcclusion[snapshotSampleIndex, 1:2, :, :]
            inputNormal = inputOcclusion[snapshotSampleIndex, 2:5, :, :]

            targetOcclusion = targetOcclusion[snapshotSampleIndex, :, :, :]
            targetSurface = torch.clone(tensors['PointsSparseColor'][snapshotSampleIndex])
            targetSurface[(targetOcclusion > 0.5).repeat(3, 1, 1)] = 0.0

            outputOcclusion = outputOcclusion[snapshotSampleIndex, :, :, :]
            outputSurface = torch.clone(tensors['PointsSparseColor'][snapshotSampleIndex])
            outputSurface[(outputOcclusion > 0.5).repeat(3, 1, 1)] = 0.0

            fig = plt.figure(figsize=(2 * inputDepth.size(2), 3 * inputDepth.size(1)), dpi=1)
            fig.add_subplot(3, 2, 1).title#.set_text('Input Depth')
            plt.imshow(TensorToImage(inputDepth))
            plt.axis('off')
            fig.add_subplot(3, 2, 2).title#.set_text('Input Normal')
            plt.imshow(TensorToImage(inputNormal))
            plt.axis('off')

            fig.add_subplot(3, 2, 3).title#.set_text('Output Occlusion')
            plt.imshow(TensorToImage(outputOcclusion))
            plt.axis('off')
            fig.add_subplot(3, 2, 4).title#.set_text('Output Surface')
            plt.imshow(TensorToImage(outputSurface))
            plt.axis('off')

            fig.add_subplot(3, 2, 5).title#.set_text('Target Occlusion')
            plt.imshow(TensorToImage(targetOcclusion))
            plt.axis('off')
            fig.add_subplot(3, 2, 6).title#.set_text('Target Surface')
            plt.imshow(TensorToImage(targetSurface))
            plt.axis('off')

            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)

            summaryWriter.add_figure('SnapshotsOcclusion/Epoch' + str(epoch), plt.gcf(), iteration)

            # Save a checkpoint to file
            checkpoint = {
                'Epoch' : epoch,
                'BatchIndexStart' : batchIndex + 1,
                'ModelOcclusion' : modelOcclusion.state_dict(),
                'OptimizerOcclusion' : optimizerOcclusion.state_dict(),
                'SchedulerOcclusion' : schedulerOcclusion.state_dict(),
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