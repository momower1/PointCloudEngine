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
batchSize = 8
snapshotSkip = 256
batchIndexStart = 0
learningRate = 1e-3
schedulerDecayRate = 0.95
schedulerDecaySkip = 100000
batchCount = dataset.trainingSequenceCount // batchSize

# Surface Classification Model
SCM = PullPushModel(5, 1, 16).to(device)
optimizerSCM = torch.optim.Adam(SCM.parameters(), lr=learningRate, betas=(0.9, 0.999))
schedulerSCM = torch.optim.lr_scheduler.ExponentialLR(optimizerSCM, gamma=schedulerDecayRate, verbose=False)
factorLossSurfaceSCM = 1.0
factorLossTemporalSCM = 0.0

# Surface Flow Model
SFM = PullPushModel(7, 2, 16).to(device)
optimizerSFM = torch.optim.Adam(SFM.parameters(), lr=learningRate, betas=(0.9, 0.999))
schedulerSFM = torch.optim.lr_scheduler.ExponentialLR(optimizerSFM, gamma=schedulerDecayRate, verbose=False)
factorLossOpticalFlowSFM = 1.0
factorLossTemporalSFM = 0.0

# Surface Reconstruction Model
SRM = PullPushModel(16, 8, 16).to(device)
optimizerSRM = torch.optim.Adam(SRM.parameters(), lr=learningRate, betas=(0.9, 0.999))
schedulerSRM = torch.optim.lr_scheduler.ExponentialLR(optimizerSRM, gamma=schedulerDecayRate, verbose=False)
previousOutputSRM = None
factorLossSurfaceSRM = 1.0
factorLossDepthSRM = 1.0
factorLossColorSRM = 1.0
factorLossNormalSRM = 1.0
factorLossTemporalSRM = 0.0

# Use this directory for the visualization of loss graphs in the Tensorboard at http://localhost:6006/
checkpointDirectory += 'Supervised/'
summaryWriter = SummaryWriter(log_dir=checkpointDirectory)

# Try to load the last checkpoint and continue training from there
if os.path.exists(checkpointDirectory + checkpointNameStart + checkpointNameEnd):
    print('Loading existing checkpoint from "' + checkpointDirectory + checkpointNameStart + checkpointNameEnd + '"')
    checkpoint = torch.load(checkpointDirectory + checkpointNameStart + checkpointNameEnd)
    epoch = checkpoint['Epoch']
    batchIndexStart = checkpoint['BatchIndexStart']
    SCM.load_state_dict(checkpoint['SCM'])
    SFM.load_state_dict(checkpoint['SFM'])
    SRM.load_state_dict(checkpoint['SRM'])
    optimizerSCM.load_state_dict(checkpoint['OptimizerSCM'])
    optimizerSFM.load_state_dict(checkpoint['OptimizerSFM'])
    optimizerSRM.load_state_dict(checkpoint['OptimizerSRM'])
    schedulerSCM.load_state_dict(checkpoint['SchedulerSCM'])
    schedulerSFM.load_state_dict(checkpoint['SchedulerSFM'])
    schedulerSRM.load_state_dict(checkpoint['SchedulerSRM'])

def PrintLearningRates():
    print('Learning rates: ', end='')
    print('SCM ' + str(schedulerSCM.get_last_lr()[0]), end='')
    print(', SFM ' + str(schedulerSFM.get_last_lr()[0]), end='')
    print(', SRM ' + str(schedulerSRM.get_last_lr()[0]))

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
    PrintLearningRates()

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
        frameGenerationOrder = range(dataset.sequenceFrameCount) if (iteration % 2 == 0) else range(dataset.sequenceFrameCount - 1, -1, -1)

        # Surface Classification Model
        inputsSCM = []
        outputsSCM = []
        targetsSCM = []

        # Surface Flow Model
        inputsSFM = []
        outputsSFM = []
        targetsSFM = []
        inputFlowMinsSFM = []
        inputFlowMaxsSFM = []
        targetFlowMinsSFM = []
        targetFlowMaxsSFM = []

        # Surface Reconstruction Model
        inputsSRM = []
        outputsSRM = []
        targetsSRM = []

        # Create input, output and target tensors for each frame in the sequence
        for frameIndex in frameGenerationOrder:
            meshForeground = sequence['MeshForeground'][frameIndex]
            meshDepth = sequence['MeshDepth'][frameIndex]
            meshColor = sequence['MeshColor'][frameIndex]
            meshNormal = sequence['MeshNormalScreen'][frameIndex]
            meshOpticalFlowForward = sequence['MeshOpticalFlowForward'][frameIndex]

            pointsSparseSurface = sequence['PointsSparseSurface'][frameIndex]
            pointsSparseForeground = sequence['PointsSparseForeground'][frameIndex]
            pointsSparseDepth = sequence['PointsSparseDepth'][frameIndex]
            pointsSparseColor = sequence['PointsSparseColor'][frameIndex]
            pointsSparseNormal = sequence['PointsSparseNormalScreen'][frameIndex]
            pointsSparseOpticalFlowForward = sequence['PointsSparseOpticalFlowForward'][frameIndex]

            # Surface Classification Model
            inputSCM = torch.cat([pointsSparseForeground, pointsSparseDepth, pointsSparseNormal], dim=1)
            outputSCM = SCM(inputSCM)
            targetSCM = pointsSparseSurface
            inputsSCM.append(inputSCM)
            outputsSCM.append(outputSCM)
            targetsSCM.append(targetSCM)

            # Only keep pixels if they are part of the predicted surface
            pointsSparseSurfacePredicted = outputSCM.ge(0.5).float()
            pointsSparseSurfaceDepthPredicted = pointsSparseSurfacePredicted * pointsSparseDepth
            pointsSparseSurfaceColorPredicted = pointsSparseSurfacePredicted * pointsSparseColor
            pointsSparseSurfaceNormalPredicted = pointsSparseSurfacePredicted * pointsSparseNormal
            pointsSparseSurfaceOpticalFlowForwardPredicted = pointsSparseSurfacePredicted * pointsSparseOpticalFlowForward

            # Surface Flow Model
            inputFlowMinSFM, inputFlowMaxSFM, inputFlowSFM = ConvertMotionVectorIntoZeroToOneRange(pointsSparseSurfaceOpticalFlowForwardPredicted)
            inputSFM = torch.cat([pointsSparseSurfacePredicted, pointsSparseSurfaceDepthPredicted, pointsSparseSurfaceNormalPredicted, inputFlowSFM], dim=1)
            outputSFM = SFM(inputSFM)
            outputMotionVectorSFM = ConvertMotionVectorIntoPixelRange(inputFlowMinSFM, inputFlowMaxSFM, outputSFM.detach())
            targetFlowMinSFM, targetFlowMaxSFM, targetFlowSFM = ConvertMotionVectorIntoZeroToOneRange(meshOpticalFlowForward)
            targetSFM = targetFlowSFM
            inputsSFM.append(inputSFM)
            outputsSFM.append(outputSFM)
            targetsSFM.append(targetSFM)
            inputFlowMinsSFM.append(inputFlowMinSFM)
            inputFlowMaxsSFM.append(inputFlowMaxSFM)
            targetFlowMinsSFM.append(targetFlowMinSFM)
            targetFlowMaxsSFM.append(targetFlowMaxSFM)

            # Surface Reconstruction Model
            inputSRM = torch.cat([pointsSparseSurfacePredicted, pointsSparseSurfaceDepthPredicted, pointsSparseSurfaceColorPredicted, pointsSparseSurfaceNormalPredicted], dim=1)

            if previousOutputSRM is None:
                previousOutputSRM = torch.zeros_like(inputSRM)

            previousOutputWarpedSRM = WarpImage(previousOutputSRM, outputMotionVectorSFM)
            inputSRM = torch.cat([inputSRM, previousOutputWarpedSRM], dim=1)
            outputSRM = SRM(inputSRM)
            previousOutputSRM = outputSRM
            targetSRM = torch.cat([meshForeground, meshDepth, meshColor, meshNormal], dim=1)
            inputsSRM.append(inputSRM)
            outputsSRM.append(outputSRM)
            targetsSRM.append(targetSRM)

        # TODO?: Stack into tensors of shape [dataset.sequenceFrameCount, N, C, H, W]

        # Surface Classification Model Loss Terms
        lossSurfaceSCM = []
        lossTemporalSCM = []

        # Surface Flow Model Loss Terms
        lossOpticalFlowSFM = []
        lossTemporalSFM = []

        # Surface Reconstruction Model Loss Terms
        lossSurfaceSRM = []
        lossDepthSRM = []
        lossColorSRM = []
        lossNormalSRM = []
        lossTemporalSRM = []

        # Accumulate loss terms over triplets in the sequence
        # TODO: Add temporal information using warped frames (e.g. inputPreviousWarpedForward, inputNextWarpedBackward)
        for tripletFrameIndex in range(1, dataset.sequenceFrameCount - 1):
            motionVectorPreviousToCurrent = sequence['MeshOpticalFlowForward'][tripletFrameIndex]
            motionVectorNextToCurrent = sequence['MeshOpticalFlowBackward'][tripletFrameIndex + 1]

            # TODO: Temporal loss term should probably include occlusion and also current output

            # Surface Classification Model
            outputPreviousSCM = outputsSCM[tripletFrameIndex - 1]
            outputPreviousWarpedSCM = WarpImage(outputPreviousSCM, motionVectorPreviousToCurrent)
            outputSCM = outputsSCM[tripletFrameIndex]
            outputNextSCM = outputsSCM[tripletFrameIndex + 1]
            outputNextWarpedSCM = WarpImage(outputNextSCM, motionVectorNextToCurrent)
            targetSCM = targetsSCM[tripletFrameIndex]

            lossSurfaceTripletSCM = factorLossSurfaceSCM * torch.nn.functional.binary_cross_entropy(outputSCM, targetSCM, reduction='mean')
            lossTemporalTripletSCM = factorLossTemporalSCM * torch.nn.functional.mse_loss(outputPreviousWarpedSCM, outputNextWarpedSCM, reduction='mean')
            lossSurfaceSCM.append(lossSurfaceTripletSCM)
            lossTemporalSCM.append(lossTemporalTripletSCM)

            # TODO: Re-normalize depth for the sparse surface!

            # Surface Flow Model
            outputPreviousSFM = outputsSFM[tripletFrameIndex - 1]
            outputPreviousWarpedSFM = WarpImage(outputPreviousSFM, motionVectorPreviousToCurrent)
            outputSFM = outputsSFM[tripletFrameIndex]
            outputNextSFM = outputsSFM[tripletFrameIndex + 1]
            outputNextWarpedSFM = WarpImage(outputNextSFM, motionVectorNextToCurrent)
            targetSFM = targetsSFM[tripletFrameIndex]

            lossOpticalFlowTripletSFM = factorLossOpticalFlowSFM * torch.nn.functional.mse_loss(outputSFM, targetSFM, reduction='mean')
            lossTemporalTripletSFM = factorLossTemporalSFM * torch.nn.functional.mse_loss(outputPreviousWarpedSFM, outputNextWarpedSFM, reduction='mean')
            lossOpticalFlowSFM.append(lossOpticalFlowTripletSFM)
            lossTemporalSFM.append(lossTemporalTripletSFM)

            # Surface Reconstruction Model
            # TODO: WGAN
            outputPreviousSRM = outputsSRM[tripletFrameIndex - 1]
            outputPreviousWarpedSRM = WarpImage(outputPreviousSRM, motionVectorPreviousToCurrent)
            outputSRM = outputsSRM[tripletFrameIndex]
            outputNextSRM = outputsSRM[tripletFrameIndex + 1]
            outputNextWarpedSRM = WarpImage(outputNextSRM, motionVectorNextToCurrent)
            targetSRM = targetsSRM[tripletFrameIndex]

            lossSurfaceTripletSRM = factorLossSurfaceSRM * torch.nn.functional.binary_cross_entropy(outputSRM[:, 0:1, :, :], targetSRM[:, 0:1, :, :], reduction='mean')
            lossDepthTripletSRM = factorLossDepthSRM * torch.nn.functional.mse_loss(outputSRM[:, 1:2, :, :], targetSRM[:, 1:2, :, :], reduction='mean')
            lossColorTripletSRM = factorLossColorSRM * torch.nn.functional.mse_loss(outputSRM[:, 2:5, :, :], targetSRM[:, 2:5, :, :], reduction='mean')
            lossNormalTripletSRM = factorLossNormalSRM * torch.nn.functional.mse_loss(outputSRM[:, 5:8, :, :], targetSRM[:, 5:8, :, :], reduction='mean')
            lossTemporalTripletSRM = factorLossTemporalSRM * torch.nn.functional.mse_loss(outputPreviousWarpedSRM, outputNextWarpedSRM, reduction='mean')
            lossSurfaceSRM.append(lossSurfaceTripletSRM)
            lossDepthSRM.append(lossDepthTripletSRM)
            lossColorSRM.append(lossColorTripletSRM)
            lossNormalSRM.append(lossNormalTripletSRM)
            lossTemporalSRM.append(lossTemporalTripletSRM)

        # Average all loss terms across the sequence
        lossSurfaceSCM = torch.stack(lossSurfaceSCM, dim=0).mean()
        lossTemporalSCM = torch.stack(lossTemporalSCM, dim=0).mean()
        lossOpticalFlowSFM = torch.stack(lossOpticalFlowSFM, dim=0).mean()
        lossTemporalSFM = torch.stack(lossTemporalSFM, dim=0).mean()
        lossSurfaceSRM = torch.stack(lossSurfaceSRM, dim=0).mean()
        lossDepthSRM = torch.stack(lossDepthSRM, dim=0).mean()
        lossColorSRM = torch.stack(lossColorSRM, dim=0).mean()
        lossNormalSRM = torch.stack(lossNormalSRM, dim=0).mean()
        lossTemporalSRM = torch.stack(lossTemporalSRM, dim=0).mean()

        # Combine partial losses into a single final loss term
        lossSCM = torch.stack([lossSurfaceSCM, lossTemporalSCM], dim=0).mean()
        lossSFM = torch.stack([lossOpticalFlowSFM, lossTemporalSFM], dim=0).mean()
        lossSRM = torch.stack([lossSurfaceSRM, lossDepthSRM, lossColorSRM, lossNormalSRM, lossTemporalSRM], dim=0).mean()

        # TODO: Pretrain SCM, then pretrain SFM and lastly train SRM

        # Train Surface Classification Model
        SCM.zero_grad()
        lossSCM.backward(retain_graph=True)
        optimizerSCM.step()

        # Train Surface Flow Model
        SFM.zero_grad()
        lossSFM.backward(retain_graph=True)
        optimizerSFM.step()

        # Train Surface Reconstruction Model
        SRM.zero_grad()
        lossSRM.backward(retain_graph=False)
        optimizerSRM.step()

        # Need to detach previous output for next iteration (since using recurrent architecture)
        previousOutputSRM = previousOutputSRM.detach()

        # Plot losses
        summaryWriter.add_scalar('Surface Classification Model/Loss SCM', lossSCM, iteration)
        summaryWriter.add_scalar('Surface Classification Model/Loss Surface SCM', lossSurfaceSCM, iteration)
        summaryWriter.add_scalar('Surface Classification Model/Loss Temporal SCM', lossTemporalSCM, iteration)
        summaryWriter.add_scalar('Surface Flow Model/Loss SFM', lossSFM, iteration)
        summaryWriter.add_scalar('Surface Flow Model/Loss Optical Flow SFM', lossOpticalFlowSFM, iteration)
        summaryWriter.add_scalar('Surface Flow Model/Loss Temporal SFM', lossTemporalSFM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss SRM', lossSRM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss Surface SRM', lossSurfaceSRM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss Depth SRM', lossDepthSRM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss Color SRM', lossColorSRM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss Normal SRM', lossNormalSRM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss Temporal SRM', lossTemporalSRM, iteration)

        # Update learning rate using schedulers
        if iteration % schedulerDecaySkip == (schedulerDecaySkip - 1):
            schedulerSCM.step()
            schedulerSFM.step()
            schedulerSRM.step()
            PrintLearningRates()

        # Print progress, save checkpoints, create snapshots and plot loss graphs in certain intervals
        if iteration % snapshotSkip == (snapshotSkip - 1):
            snapshotIndex = (iteration // snapshotSkip)
            snapshotSampleIndex = snapshotIndex % batchSize
            snapshotFrameIndex = 1 + (snapshotIndex % (dataset.sequenceFrameCount - 2))

            progress = 'Epoch:\t' + str(epoch) + '\t' + str(int(100 * (batchIndex / batchCount))) + '%'
            print(progress)

            # Surface Classification Model
            inputPointsSparseForegroundSCM = inputsSCM[snapshotFrameIndex][snapshotSampleIndex, 0:1, :, :]
            inputPointsSparseDepthSCM = inputsSCM[snapshotFrameIndex][snapshotSampleIndex, 1:2, :, :]
            inputPointsSparseNormalSCM = inputsSCM[snapshotFrameIndex][snapshotSampleIndex, 2:5, :, :]
            outputPointsSparseSurface = outputsSCM[snapshotFrameIndex][snapshotSampleIndex, :, :, :]
            outputPointsSparseSurfaceMask = outputPointsSparseSurface.ge(0.5).float()
            targetPointsSparseSurface = targetsSCM[snapshotFrameIndex][snapshotSampleIndex, :, :, :]

            fig = plt.figure(figsize=(3 * dataset.width, 2 * dataset.height), dpi=1)
            fig.add_subplot(2, 3, 1).title#.set_text('Input Points Sparse Foreground')
            plt.imshow(TensorToImage(inputPointsSparseForegroundSCM))
            plt.axis('off')
            fig.add_subplot(2, 3, 2).title#.set_text('Input Points Sparse Depth')
            plt.imshow(TensorToImage(inputPointsSparseDepthSCM))
            plt.axis('off')
            fig.add_subplot(2, 3, 3).title#.set_text('Input Points Sparse Normal')
            plt.imshow(TensorToImage(inputPointsSparseNormalSCM))
            plt.axis('off')
            fig.add_subplot(2, 3, 4).title#.set_text('Output Points Sparse Surface')
            plt.imshow(TensorToImage(outputPointsSparseSurface))
            plt.axis('off')
            fig.add_subplot(2, 3, 5).title#.set_text('Output Points Sparse Surface Mask')
            plt.imshow(TensorToImage(outputPointsSparseSurfaceMask))
            plt.axis('off')
            fig.add_subplot(2, 3, 6).title#.set_text('Target Points Sparse Surface')
            plt.imshow(TensorToImage(targetPointsSparseSurface))
            plt.axis('off')
            
            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)
            summaryWriter.add_figure('Surface Classification Model/Epoch' + str(epoch), plt.gcf(), iteration)

            # Surface Flow Model
            inputPointsSparseSurfacePredicted = inputsSFM[snapshotFrameIndex][snapshotSampleIndex, 0:1, :, :]
            inputPointsSparseSurfaceDepthPredicted = inputsSFM[snapshotFrameIndex][snapshotSampleIndex, 1:2, :, :]
            inputPointsSparseSurfaceNormalPredicted = inputsSFM[snapshotFrameIndex][snapshotSampleIndex, 2:5, :, :]
            inputPointsSparseSurfaceOpticalFlowForward = inputsSFM[snapshotFrameIndex][snapshotSampleIndex, 5:7, :, :]
            outputSurfaceOpticalFlowForward = outputsSFM[snapshotFrameIndex][snapshotSampleIndex, :, :, :]
            targetMeshOpticalFlowForward = targetsSFM[snapshotFrameIndex][snapshotSampleIndex, :, :, :]

            fig = plt.figure(figsize=(3 * dataset.width, 2 * dataset.height), dpi=1)
            fig.add_subplot(2, 3, 1).title#.set_text('Input Points Sparse Surface Predicted')
            plt.imshow(TensorToImage(inputPointsSparseSurfacePredicted))
            plt.axis('off')
            fig.add_subplot(2, 3, 2).title#.set_text('Input Points Sparse Surface Depth Predicted')
            plt.imshow(TensorToImage(inputPointsSparseSurfaceDepthPredicted))
            plt.axis('off')
            fig.add_subplot(2, 3, 3).title#.set_text('Input Points Sparse Surface Normal Predicted')
            plt.imshow(TensorToImage(inputPointsSparseSurfaceNormalPredicted))
            plt.axis('off')
            fig.add_subplot(2, 3, 4).title#.set_text('Input Points Sparse Surface Optical Flow Forward')
            plt.imshow(FlowToImage(inputPointsSparseSurfaceOpticalFlowForward))
            plt.axis('off')
            fig.add_subplot(2, 3, 5).title#.set_text('Output Surface Optical Flow Forward')
            plt.imshow(FlowToImage(outputSurfaceOpticalFlowForward))
            plt.axis('off')
            fig.add_subplot(2, 3, 6).title#.set_text('Target Mesh Optical Flow Forward')
            plt.imshow(FlowToImage(targetMeshOpticalFlowForward))
            plt.axis('off')
            
            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)
            summaryWriter.add_figure('Surface Flow Model/Epoch' + str(epoch), plt.gcf(), iteration)

            # Surface Reconstruction Model
            inputPointsSparseSurfacePredicted = inputsSRM[snapshotFrameIndex][snapshotSampleIndex, 0:1, :, :]
            inputPointsSparseSurfaceDepthPredicted = inputsSRM[snapshotFrameIndex][snapshotSampleIndex, 1:2, :, :]
            inputPointsSparseSurfaceColorPredicted = inputsSRM[snapshotFrameIndex][snapshotSampleIndex, 2:5, :, :]
            inputPointsSparseSurfaceNormalPredicted = inputsSRM[snapshotFrameIndex][snapshotSampleIndex, 5:8, :, :]
            inputPreviousWarpedSurfacePredicted = inputsSRM[snapshotFrameIndex][snapshotSampleIndex, 8:9, :, :]
            inputPreviousWarpedSurfaceDepthPredicted = inputsSRM[snapshotFrameIndex][snapshotSampleIndex, 9:10, :, :]
            inputPreviousWarpedSurfaceColorPredicted = inputsSRM[snapshotFrameIndex][snapshotSampleIndex, 10:13, :, :]
            inputPreviousWarpedSurfaceNormalPredicted = inputsSRM[snapshotFrameIndex][snapshotSampleIndex, 13:16, :, :]
            outputSurfaceForeground = outputsSRM[snapshotFrameIndex][snapshotSampleIndex, 0:1, :, :]
            outputSurfaceDepth = outputsSRM[snapshotFrameIndex][snapshotSampleIndex, 1:2, :, :]
            outputSurfaceColor = outputsSRM[snapshotFrameIndex][snapshotSampleIndex, 2:5, :, :]
            outputSurfaceNormal = outputsSRM[snapshotFrameIndex][snapshotSampleIndex, 5:8, :, :]
            targetMeshForeground = targetsSRM[snapshotFrameIndex][snapshotSampleIndex, 0:1, :, :]
            targetMeshDepth = targetsSRM[snapshotFrameIndex][snapshotSampleIndex, 1:2, :, :]
            targetMeshColor = targetsSRM[snapshotFrameIndex][snapshotSampleIndex, 2:5, :, :]
            targetMeshNormal = targetsSRM[snapshotFrameIndex][snapshotSampleIndex, 5:8, :, :]

            fig = plt.figure(figsize=(4 * dataset.width, 4 * dataset.height), dpi=1)
            fig.add_subplot(4, 4, 1).title#.set_text('Input Points Sparse Surface Predicted')
            plt.imshow(TensorToImage(inputPointsSparseSurfacePredicted))
            plt.axis('off')
            fig.add_subplot(4, 4, 2).title#.set_text('Input Points Sparse Surface Depth Predicted')
            plt.imshow(TensorToImage(inputPointsSparseSurfaceDepthPredicted))
            plt.axis('off')
            fig.add_subplot(4, 4, 3).title#.set_text('Input Points Sparse Surface Color Predicted')
            plt.imshow(TensorToImage(inputPointsSparseSurfaceColorPredicted))
            plt.axis('off')
            fig.add_subplot(4, 4, 4).title#.set_text('Input Points Sparse Surface Normal Predicted')
            plt.imshow(TensorToImage(inputPointsSparseSurfaceNormalPredicted))
            plt.axis('off')
            fig.add_subplot(4, 4, 5).title#.set_text('Input Previous Warped Points Sparse Surface Predicted')
            plt.imshow(TensorToImage(inputPreviousWarpedSurfacePredicted))
            plt.axis('off')
            fig.add_subplot(4, 4, 6).title#.set_text('inputPreviousWarpedPointsSparseSurfaceDepthPredicted')
            plt.imshow(TensorToImage(inputPreviousWarpedSurfaceDepthPredicted))
            plt.axis('off')
            fig.add_subplot(4, 4, 7).title#.set_text('Input Previous Warped Surface Color Predicted')
            plt.imshow(TensorToImage(inputPreviousWarpedSurfaceColorPredicted))
            plt.axis('off')
            fig.add_subplot(4, 4, 8).title#.set_text('Input Previous Warped Surface Normal Predicted')
            plt.imshow(TensorToImage(inputPreviousWarpedSurfaceNormalPredicted))
            plt.axis('off')
            fig.add_subplot(4, 4, 9).title#.set_text('Output Surface Foreground')
            plt.imshow(TensorToImage(outputSurfaceForeground))
            plt.axis('off')
            fig.add_subplot(4, 4, 10).title#.set_text('Output Surface Depth')
            plt.imshow(TensorToImage(outputSurfaceDepth))
            plt.axis('off')
            fig.add_subplot(4, 4, 11).title#.set_text('Output Surface Color')
            plt.imshow(TensorToImage(outputSurfaceColor))
            plt.axis('off')
            fig.add_subplot(4, 4, 12).title#.set_text('Output Surface Normal')
            plt.imshow(TensorToImage(outputSurfaceNormal))
            plt.axis('off')
            fig.add_subplot(4, 4, 13).title#.set_text('Target Mesh Foreground')
            plt.imshow(TensorToImage(targetMeshForeground))
            plt.axis('off')
            fig.add_subplot(4, 4, 14).title#.set_text('Target Mesh Depth')
            plt.imshow(TensorToImage(targetMeshDepth))
            plt.axis('off')
            fig.add_subplot(4, 4, 15).title#.set_text('Target Mesh Color')
            plt.imshow(TensorToImage(targetMeshColor))
            plt.axis('off')
            fig.add_subplot(4, 4, 16).title#.set_text('Target Mesh Normal')
            plt.imshow(TensorToImage(targetMeshNormal))
            plt.axis('off')
            
            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)
            summaryWriter.add_figure('Surface Reconstruction Model/Epoch' + str(epoch), plt.gcf(), iteration)

            # Save a checkpoint to file
            checkpoint = {
                'Epoch' : epoch,
                'BatchIndexStart' : batchIndex + 1,
                'SCM' : SCM.state_dict(),
                'SFM' : SFM.state_dict(),
                'SRM' : SRM.state_dict(),
                'OptimizerSCM' : optimizerSCM.state_dict(),
                'OptimizerSFM' : optimizerSFM.state_dict(),
                'OptimizerSRM' : optimizerSRM.state_dict(),
                'SchedulerSCM' : schedulerSCM.state_dict(),
                'SchedulerSFM' : schedulerSFM.state_dict(),
                'SchedulerSRM' : schedulerSRM.state_dict(),
            }

            torch.save(checkpoint, checkpointDirectory + checkpointNameStart + checkpointNameEnd)
            torch.save(checkpoint, checkpointDirectory + checkpointNameStart + str(epoch) + checkpointNameEnd)

    # Move to next epoch
    numpy.random.shuffle(randomIndices)
    torch.cuda.empty_cache()
    batchIndexStart = 0
    epoch += 1