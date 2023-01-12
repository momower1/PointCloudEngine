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
SRM = PullPushModel(16, 7, 16).to(device)
optimizerSRM = torch.optim.Adam(SRM.parameters(), lr=learningRate, betas=(0.9, 0.999))
schedulerSRM = torch.optim.lr_scheduler.ExponentialLR(optimizerSRM, gamma=schedulerDecayRate, verbose=False)
previousOutputSRM = None
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
    print('Learning rates: ')
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
            outputMotionVectorSFM = ConvertMotionVectorIntoPixelRange(inputFlowMinSFM, inputFlowMaxSFM, outputSFM)
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
            targetSRM = torch.cat([meshDepth, meshColor, meshNormal], dim=1)
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

            lossDepthTripletSRM = factorLossDepthSRM * torch.nn.functional.mse_loss(outputSRM[:, 0:1, :, :], targetSRM[:, 0:1, :, :], reduction='mean')
            lossColorTripletSRM = factorLossColorSRM * torch.nn.functional.mse_loss(outputSRM[:, 1:4, :, :], targetSRM[:, 1:4, :, :], reduction='mean')
            lossNormalTripletSRM = factorLossNormalSRM * torch.nn.functional.mse_loss(outputSRM[:, 4:7, :, :], targetSRM[:, 4:7, :, :], reduction='mean')
            lossTemporalTripletSRM = factorLossTemporalSRM * torch.nn.functional.mse_loss(outputPreviousWarpedSRM, outputNextWarpedSRM, reduction='mean')
            lossDepthSRM.append(lossDepthTripletSRM)
            lossColorSRM.append(lossColorTripletSRM)
            lossNormalSRM.append(lossNormalTripletSRM)
            lossTemporalSRM.append(lossTemporalTripletSRM)

        # Average all loss terms across the sequence
        lossSurfaceSCM = torch.stack(lossSurfaceSCM, dim=0).mean()
        lossTemporalSCM = torch.stack(lossTemporalSCM, dim=0).mean()
        lossOpticalFlowSFM = torch.stack(lossOpticalFlowSFM, dim=0).mean()
        lossTemporalSFM = torch.stack(lossTemporalSFM, dim=0).mean()
        lossDepthSRM = torch.stack(lossDepthSRM, dim=0).mean()
        lossColorSRM = torch.stack(lossColorSRM, dim=0).mean()
        lossNormalSRM = torch.stack(lossNormalSRM, dim=0).mean()
        lossTemporalSRM = torch.stack(lossTemporalSRM, dim=0).mean()

        # Combine partial losses into a single final loss term
        lossSCM = torch.stack([lossSurfaceSCM, lossTemporalSCM], dim=0).mean()
        lossSFM = torch.stack([lossOpticalFlowSFM, lossTemporalSFM], dim=0).mean()
        lossSRM = torch.stack([lossDepthSRM, lossColorSRM, lossNormalSRM, lossTemporalSRM], dim=0).mean()

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

        # Plot losses
        summaryWriter.add_scalar('Surface Classification Model/Loss SCM', lossSCM, iteration)
        summaryWriter.add_scalar('Surface Classification Model/Loss Surface SCM', lossSurfaceSCM, iteration)
        summaryWriter.add_scalar('Surface Classification Model/Loss Temporal SCM', lossTemporalSCM, iteration)
        summaryWriter.add_scalar('Surface Flow Model/Loss SFM', lossSFM, iteration)
        summaryWriter.add_scalar('Surface Flow Model/Loss Optical Flow SFM', lossOpticalFlowSFM, iteration)
        summaryWriter.add_scalar('Surface Flow Model/Loss Temporal SFM', lossTemporalSFM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss SRM', lossSRM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss Depth SRM', lossDepthSRM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss Color SRM', lossColorSRM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss Normal SRM', lossNormalSRM, iteration)
        summaryWriter.add_scalar('Surface Reconstruction Model/Loss Temporal SRM', lossTemporalSRM, iteration)

        # Need to detach previous output for next iteration (since using recurrent architecture)
        previousOutputSRM.detach()

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

            break

            meshColorPrevious = sequence['MeshColor'][snapshotFrameIndex - 1][snapshotSampleIndex]
            meshColor = sequence['MeshColor'][snapshotFrameIndex][snapshotSampleIndex]

            inputFlowMin = inputFlowMins[snapshotFrameIndex][snapshotSampleIndex]
            inputFlowMax = inputFlowMaxs[snapshotFrameIndex][snapshotSampleIndex]
            inputForeground = inputs[snapshotFrameIndex, snapshotSampleIndex, 0:1, :, :]
            inputDepth = inputs[snapshotFrameIndex, snapshotSampleIndex, 1:2, :, :]
            inputNormal = inputs[snapshotFrameIndex, snapshotSampleIndex, 2:5, :, :]
            inputOpticalFlowForward = inputs[snapshotFrameIndex, snapshotSampleIndex, 5:7, :, :]

            outputSurface = outputs[snapshotFrameIndex, snapshotSampleIndex, 0:1, :, :]
            outputOpticalFlowForward = outputs[snapshotFrameIndex, snapshotSampleIndex, 1:3, :, :]
            outputMotionVectorForward = ConvertMotionVectorIntoPixelRange(inputFlowMin, inputFlowMax, outputOpticalFlowForward.unsqueeze(0))
            outputMeshWarped = WarpImage(meshColorPrevious.unsqueeze(0), outputMotionVectorForward).squeeze(0)
            outputOcclusion = EstimateOcclusion(outputMotionVectorForward).squeeze(0)
            outputMeshWarped *= outputOcclusion
            outputMeshWarped = (meshColor + outputMeshWarped) / 2.0

            targetFlowMin = targetFlowMins[snapshotFrameIndex][snapshotSampleIndex]
            targetFlowMax = targetFlowMaxs[snapshotFrameIndex][snapshotSampleIndex]
            targetSurface = targets[snapshotFrameIndex, snapshotSampleIndex, 0:1, :, :]
            targetOpticalFlowForward = targets[snapshotFrameIndex, snapshotSampleIndex, 1:3, :, :]
            targetMotionVectorForward = ConvertMotionVectorIntoPixelRange(targetFlowMin, targetFlowMax, targetOpticalFlowForward.unsqueeze(0))
            targetMeshWarped = WarpImage(meshColorPrevious.unsqueeze(0), targetMotionVectorForward).squeeze(0)
            targetOcclusion = EstimateOcclusion(targetMotionVectorForward).squeeze(0)
            targetMeshWarped *= targetOcclusion
            targetMeshWarped = (meshColor + targetMeshWarped) / 2.0

            targetMeshWarpedGT = WarpImage(meshColorPrevious.unsqueeze(0), sequence['MeshOpticalFlowForward'][snapshotFrameIndex][snapshotSampleIndex].unsqueeze(0)).squeeze(0)

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
            fig.add_subplot(4, 4, 6).title#.set_text('Mesh Color Previous')
            plt.imshow(TensorToImage(meshColorPrevious))
            plt.axis('off')
            fig.add_subplot(4, 4, 7).title#.set_text('Output Mesh Warped')
            plt.imshow(TensorToImage(outputMeshWarped))
            plt.axis('off')
            fig.add_subplot(4, 4, 8).title#.set_text('Output Optical Flow')
            plt.imshow(FlowToImage(outputOpticalFlowForward))
            plt.axis('off')

            fig.add_subplot(4, 4, 9).title#.set_text('Target Surface')
            plt.imshow(TensorToImage(targetSurface))
            plt.axis('off')
            fig.add_subplot(4, 4, 10).title#.set_text('Mesh Color')
            plt.imshow(TensorToImage(meshColor))
            plt.axis('off')
            fig.add_subplot(4, 4, 11).title#.set_text('Target Mesh Warped')
            plt.imshow(TensorToImage(targetMeshWarped))
            plt.axis('off')
            fig.add_subplot(4, 4, 12).title#.set_text('Target Optical Flow')
            plt.imshow(FlowToImage(targetOpticalFlowForward))
            plt.axis('off')

            fig.add_subplot(4, 4, 15).title#.set_text('Target Mesh Warped GT')
            plt.imshow(TensorToImage(targetMeshWarpedGT))
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