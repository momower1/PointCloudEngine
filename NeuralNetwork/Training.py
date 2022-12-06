import time
import threading
import matplotlib
import matplotlib.pyplot as plt
from torch.utils.tensorboard import SummaryWriter
from Dataset import *
from Model import *

# Use different matplotlib backend to avoid weird error
matplotlib.use('Agg')

def SaveCheckpoint(filename, epoch, batchIndex, model, optimizer, scheduler):
    checkpoint = {
        'Epoch' : epoch,
        'BatchIndexStart' : batchIndex + 1,
        'Model' : model.state_dict(),
        'Optimizer' : optimizer.state_dict(),
        'Scheduler' : scheduler.state_dict()
    }

    torch.save(checkpoint, filename + '.tmp')
    time.sleep(0.01)
    os.replace(filename + '.tmp', filename)

dataset = Dataset('G:/PointCloudEngineDataset/')

checkpointDirectory = 'G:/PointCloudEngineCheckpoints/'
checkpointNameStart = 'Checkpoint'
checkpointNameEnd = '.pt'

epoch = 0
batchSize = 2
snapshotSkip = 256
batchIndexStart = 0
learningRate = 1e-3
schedulerDecayRate = 0.95
schedulerDecaySkip = 100000
batchCount = dataset.sequenceCount // batchSize

# Create model, optimizer and scheduler
model = Model(7, 7, 1).to(device)
optimizer = torch.optim.Adam(model.parameters(), lr=learningRate, betas=(0.9, 0.999))
scheduler = torch.optim.lr_scheduler.ExponentialLR(optimizer, gamma=schedulerDecayRate, verbose=False)

# Try to load the last checkpoint and continue training from there
if os.path.exists(checkpointDirectory + checkpointNameStart + checkpointNameEnd):
    print('Loading existing checkpoint from "' + checkpointDirectory + checkpointNameStart + checkpointNameEnd + '"')
    checkpoint = torch.load(checkpointDirectory + checkpointNameStart + checkpointNameEnd)

    epoch = checkpoint['Epoch']
    batchIndexStart = checkpoint['BatchIndexStart']
    model.load_state_dict(checkpoint['Model'])
    optimizer.load_state_dict(checkpoint['Optimizer'])
    scheduler.load_state_dict(checkpoint['Scheduler'])

# Use this directory for the visualization of loss graphs in the Tensorboard at http://localhost:6006/
checkpointDirectory += 'Test/'
summaryWriter = SummaryWriter(log_dir=checkpointDirectory)

# Make order of training sequences random but predictable
numpy.random.seed(0)
randomIndices = numpy.arange(dataset.sequenceCount)

# Restore state of random order
for i in range(epoch + 1):
    numpy.random.shuffle(randomIndices)

# Use threads to concurrently preload the next items in the dataset (slightly reduces sequence loading bottleneck)
preloadThreadCount = batchSize
preloadThreads = [None] * preloadThreadCount
batchNextTensors = [None] * batchSize

for threadIndex in range(preloadThreadCount):
    tensors = dataset[randomIndices[batchIndexStart * batchSize + threadIndex]]
    batchNextTensors[threadIndex] = tensors

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

        # Get all the data and stack them into batches
        tensorNames = batchNextTensors[0].keys()
        tensors = {}

        for tensorName in tensorNames:
            tensorsToStack = []

            for sampleIndex in range(batchSize):
                tensorsToStack.append(batchNextTensors[sampleIndex][tensorName])

            tensors[tensorName] = torch.stack(tensorsToStack, dim=0)

        # Reset for next iteration
        batchNextTensors = [None] * batchSize

        def preload_thread_load_next(batchNextTensors, threadIndex, sequenceIndex):
            tensors = dataset[sequenceIndex]
            batchNextTensors[threadIndex] = tensors

        for threadIndex in range(preloadThreadCount):
            if preloadThreads[threadIndex] is None:
                sequenceIndex = randomIndices[batchIndex * batchSize + threadIndex]
                preloadThreads[threadIndex] = threading.Thread(target=preload_thread_load_next, args=(batchNextTensors, threadIndex, sequenceIndex))
                preloadThreads[threadIndex].start()

        # Train the neural network
        input = torch.cat([tensors['PointsColor'], tensors['PointsDepth'], tensors['PointsNormalScreen']], dim=1)
        target = torch.cat([tensors['MeshColor'], tensors['MeshDepth'], tensors['MeshNormalScreen']], dim=1)

        output = model(input)

        loss = torch.nn.functional.mse_loss(output, target, reduction='mean')

        model.zero_grad()
        loss.backward(retain_graph=False)
        optimizer.step()

        # Plot critic losses
        summaryWriter.add_scalar('Loss', loss, iteration)

        if iteration % schedulerDecaySkip == (schedulerDecaySkip - 1):
            scheduler.step()
            print('Learning rate: ' + str(scheduler.get_last_lr()[0]))

        # Print progress, save checkpoints, create snapshots and plot loss graphs in certain intervals
        if iteration % snapshotSkip == (snapshotSkip - 1):
            snapshotIndex = (iteration // snapshotSkip)
            snapshotSampleIndex = snapshotIndex % batchSize

            progress = 'Epoch:\t' + str(epoch) + '\t' + str(int(100 * (batchIndex / batchCount))) + '%'
            print(progress)

            inputColor = input[snapshotSampleIndex, 0:3, :, :]
            inputDepth = input[snapshotSampleIndex, 3:4, :, :]
            inputNormal = input[snapshotSampleIndex, 4:7, :, :]

            targetColor = target[snapshotSampleIndex, 0:3, :, :]
            targetDepth = target[snapshotSampleIndex, 3:4, :, :]
            targetNormal = target[snapshotSampleIndex, 4:7, :, :]

            outputColor = output[snapshotSampleIndex, 0:3, :, :]
            outputDepth = output[snapshotSampleIndex, 3:4, :, :]
            outputNormal = output[snapshotSampleIndex, 4:7, :, :]

            fig = plt.figure(figsize=(3, 3), dpi=inputColor.size(2))
            fig.add_subplot(3, 3, 1).title#.set_text('Input Color')
            plt.imshow(TensorToImage(inputColor))
            plt.axis('off')
            fig.add_subplot(3, 3, 2).title#.set_text('Input Depth')
            plt.imshow(TensorToImage(inputDepth))
            plt.axis('off')
            fig.add_subplot(3, 3, 3).title#.set_text('Input Normal')
            plt.imshow(TensorToImage(inputNormal))
            plt.axis('off')

            fig.add_subplot(3, 3, 4).title#.set_text('Output Color')
            plt.imshow(TensorToImage(outputColor))
            plt.axis('off')
            fig.add_subplot(3, 3, 5).title#.set_text('Output Depth')
            plt.imshow(TensorToImage(outputDepth))
            plt.axis('off')
            fig.add_subplot(3, 3, 6).title#.set_text('Output Normal')
            plt.imshow(TensorToImage(outputNormal))
            plt.axis('off')

            fig.add_subplot(3, 3, 7).title#.set_text('Target Color')
            plt.imshow(TensorToImage(targetColor))
            plt.axis('off')
            fig.add_subplot(3, 3, 8).title#.set_text('Target Depth')
            plt.imshow(TensorToImage(targetDepth))
            plt.axis('off')
            fig.add_subplot(3, 3, 9).title#.set_text('Target Normal')
            plt.imshow(TensorToImage(targetNormal))
            plt.axis('off')

            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)

            summaryWriter.add_figure('Snapshots/Epoch' + str(epoch), plt.gcf(), iteration)

            SaveCheckpoint(checkpointDirectory + checkpointNameStart + checkpointNameEnd, epoch, batchIndex, model, optimizer, scheduler)

    # Move to next epoch
    numpy.random.shuffle(randomIndices)
    torch.cuda.empty_cache()
    batchIndexStart = 0
    epoch += 1