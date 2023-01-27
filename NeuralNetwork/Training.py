import time
import threading
import matplotlib
import matplotlib.pyplot as plt
from torch.utils.tensorboard import SummaryWriter
from Dataset import *
from Model import *

# Use different matplotlib backend to avoid weird error
matplotlib.use('Agg')

dataset = Dataset('G:/PointCloudEngineDataset128/', 3, False, True, False, 128)

checkpointDirectory = 'G:/PointCloudEngineCheckpoints/'
checkpointNameStart = 'Checkpoint'
checkpointNameEnd = '.pt'

epoch = 0
batchSize = 4
snapshotSkip = 256
batchIndexStart = 0
learningRate = 1e-4
schedulerDecayRate = 0.95
schedulerDecaySkip = 100000
occlusionArtifactFilterSize = 0
batchCount = dataset.trainingSequenceCount // batchSize

# Surface Classification Model
SCM = UnetPullPush(5, 1, 16).to(device)
optimizerSCM = torch.optim.Adam(SCM.parameters(), lr=learningRate, betas=(0.9, 0.999))
schedulerSCM = torch.optim.lr_scheduler.ExponentialLR(optimizerSCM, gamma=schedulerDecayRate, verbose=False)
factorLossSurfaceSCM = 1.0
factorLossTemporalSCM = 1.0

# Surface Flow Model
SFM = UnetPullPush(7, 2, 16).to(device)
optimizerSFM = torch.optim.Adam(SFM.parameters(), lr=learningRate, betas=(0.9, 0.999))
schedulerSFM = torch.optim.lr_scheduler.ExponentialLR(optimizerSFM, gamma=schedulerDecayRate, verbose=False)
factorLossOpticalFlowSFM = 1.0
factorLossTemporalSFM = 1.0

# Surface Reconstruction Model
trainingAdversarialSRM = False#True
SRM = UnetPullPush(16, 8, 16).to(device)
optimizerSRM = torch.optim.Adam(SRM.parameters(), lr=learningRate, betas=(0.5, 0.9) if trainingAdversarialSRM else (0.9, 0.999))
schedulerSRM = torch.optim.lr_scheduler.ExponentialLR(optimizerSRM, gamma=schedulerDecayRate, verbose=False)
previousOutputSRM = None
factorLossSurfaceSRM = 1.0
factorLossDepthSRM = 3.0
factorLossColorSRM = 10.0
factorLossNormalSRM = 1.0
factorLossTemporalSRM = 10.0

if trainingAdversarialSRM:
    criticSRM = Critic(48, 1, 48).to(device)
    optimizerCriticSRM = torch.optim.Adam(criticSRM.parameters(), lr=learningRate, betas=(0.5, 0.9))
    schedulerCriticSRM = torch.optim.lr_scheduler.ExponentialLR(optimizerCriticSRM, gamma=schedulerDecayRate, verbose=False)

    # Required for adaptive WGAN update strategy
    adaptiveUpdateCoefficientSRM = 1.0
    lossCriticWassersteinPreviousSRM = None
    lossWassersteinPreviousSRM = None
    stepsCriticSRM = 0
    stepsSRM = 0
    ratioCriticSRM = 1.0
    ratioSRM = 1.0

# Use this directory for the visualization of loss graphs in the Tensorboard at http://localhost:6006/
checkpointDirectory += 'UnetPullPush Tanh L1 1e-4 Supervised SRM 1 3 10 1 10/'
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

    if trainingAdversarialSRM:
        criticSRM.load_state_dict(checkpoint['CriticSRM'])
        optimizerCriticSRM.load_state_dict(checkpoint['OptimizerCriticSRM'])
        schedulerCriticSRM.load_state_dict(checkpoint['SchedulerCriticSRM'])
        stepsCriticSRM = checkpoint['StepsCriticSRM']
        stepsSRM = checkpoint['StepsSRM']

def PrintLearningRates():
    print('Learning rates: ', end='')
    print('SCM ' + str(schedulerSCM.get_last_lr()[0]), end='')
    print(', SFM ' + str(schedulerSFM.get_last_lr()[0]), end='')
    print(', SRM ' + str(schedulerSRM.get_last_lr()[0]), end='' if trainingAdversarialSRM else '\n')

    if trainingAdversarialSRM:
        print(', CriticSRM ' + str(schedulerCriticSRM.get_last_lr()[0]), end='\n')

# Make order of training sequences random but predictable
numpy.random.seed(0)
randomIndices = numpy.arange(dataset.trainingSequenceCount)

# Restore state of random order
for i in range(epoch + 1):
    numpy.random.shuffle(randomIndices)

# Use threads to concurrently preload the next items in the dataset (slightly reduces sequence loading bottleneck)
dataset.CreateNextRandomCropRect()
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
        frameWidth = dataset.cropWidth
        frameHeight = dataset.cropHeight
        dataset.CreateNextRandomCropRect()
        batchNextSequence = [None] * batchSize

        def preload_thread_load_next(batchNextSequence, threadIndex, sequenceIndex):
            batchNextSequence[threadIndex] = dataset.GetTrainingSequence(sequenceIndex)

        for threadIndex in range(preloadThreadCount):
            if preloadThreads[threadIndex] is None:
                sequenceIndex = randomIndices[batchIndex * batchSize + threadIndex]
                preloadThreads[threadIndex] = threading.Thread(target=preload_thread_load_next, args=(batchNextSequence, threadIndex, sequenceIndex))
                preloadThreads[threadIndex].start()

        # Surface Classification Model
        inputsSCM = []
        outputsSCM = []
        targetsSCM = []
        accuracySCM = []

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
        for frameIndex in range(dataset.sequenceFrameCount):
            meshForeground = sequence['MeshForeground'][frameIndex]
            meshDepth = sequence['MeshDepth'][frameIndex]
            meshColor = sequence['MeshColor'][frameIndex]
            meshNormal = sequence['MeshNormalScreen'][frameIndex]
            meshOpticalFlowForward = sequence['MeshOpticalFlowForward'][frameIndex]
            meshOpticalFlowBackward = sequence['MeshOpticalFlowBackward'][frameIndex]

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

            # Calculate SCM batchwise accuracy for plotting
            for sampleIndex in range(batchSize):
                accuracySampleMaskSCM = pointsSparseForeground[sampleIndex, :, :, :].ge(0.5)
                accuracyOutputMaskedSCM = outputSCM[sampleIndex, :, :, :][accuracySampleMaskSCM]
                accuracyTargetMaskedSCM = targetSCM[sampleIndex, :, :, :][accuracySampleMaskSCM]

                if accuracyOutputMaskedSCM.numel() > 0 and accuracyTargetMaskedSCM.numel() > 0:
                    accuracySampleSCM = torch.eq(accuracyOutputMaskedSCM.ge(0.5), accuracyTargetMaskedSCM.ge(0.5)).float().sum() / accuracySampleMaskSCM.float().sum()
                    accuracySCM.append(accuracySampleSCM)

            # Renormalize depth for the sparse surface (since non-surface pixel depth values are now gone)
            tmpMin, tmpMax, pointsSparseSurfaceDepthPredicted = ConvertTensorIntoZeroToOneRange(pointsSparseSurfaceDepthPredicted)

            # TODO: Also inpaint backward optical flow?

            # Surface Flow Model
            inputFlowMinSFM, inputFlowMaxSFM, inputFlowSFM = ConvertTensorIntoZeroToOneRange(pointsSparseSurfaceOpticalFlowForwardPredicted)
            inputSFM = torch.cat([pointsSparseSurfacePredicted, pointsSparseSurfaceDepthPredicted, pointsSparseSurfaceNormalPredicted, inputFlowSFM], dim=1)
            outputSFM = SFM(inputSFM)
            outputMotionVectorSFM = RevertTensorIntoFullRange(inputFlowMinSFM, inputFlowMaxSFM, outputSFM.detach())
            targetFlowMinSFM, targetFlowMaxSFM, targetFlowSFM = ConvertTensorIntoZeroToOneRange(meshOpticalFlowForward)
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

            if previousOutputSRM is None or previousOutputSRM.shape != inputSRM.shape:
                previousOutputSRM = torch.zeros_like(inputSRM)

            # TODO: Only keep warped pixels if they are not occluded by both forward and backward motion
            # But this requires the network to also inpaint the sparse backward motion (which is not considered right now)
            #previousOutputSRM *= EstimateOcclusion(outputMotionVectorBackwardSFM, distance=1, artifactFilterSize=occlusionArtifactFilterSize)
            previousOutputWarpedSRM = WarpImage(previousOutputSRM, outputMotionVectorSFM)
            previousOutputWarpedSRM *= EstimateOcclusion(outputMotionVectorSFM, distance=1, artifactFilterSize=occlusionArtifactFilterSize)

            inputSRM = torch.cat([inputSRM, previousOutputWarpedSRM], dim=1)
            outputSRM = SRM(inputSRM)
            previousOutputSRM = outputSRM
            targetSRM = torch.cat([meshForeground, meshDepth, meshColor, meshNormal], dim=1)
            inputsSRM.append(inputSRM)
            outputsSRM.append(outputSRM)
            targetsSRM.append(targetSRM)

        # Average SCM accuracy
        if len(accuracySCM) > 0:
            accuracySCM = torch.stack(accuracySCM, dim=0).mean()
            summaryWriter.add_scalar('Surface Classification Model/Accuracy SCM', accuracySCM, iteration)

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

        if trainingAdversarialSRM:
            lossWassersteinSRM = []
            lossCriticWassersteinSRM = []
            lossCriticGradientPenaltySRM = []

            # Adaptively either update critic or generator depending on the wasserstein loss ratios
            updateCriticSRM = ratioCriticSRM >= adaptiveUpdateCoefficientSRM * ratioSRM

            # Randomly reverse triplet frame order to improve recurrent WGAN temporal stability
            reverseCriticSequence = random.randint(0, 1) == 1

        # Accumulate loss terms over triplets in the sequence
        # TODO: Add temporal information using warped frames (e.g. inputPreviousWarpedForward, inputNextWarpedBackward)
        for tripletFrameIndex in range(1, dataset.sequenceFrameCount - 1):
            motionVectorPreviousToCurrent = sequence['MeshOpticalFlowForward'][tripletFrameIndex]
            motionVectorCurrentToPrevious = sequence['MeshOpticalFlowBackward'][tripletFrameIndex]
            motionVectorNextToCurrent = sequence['MeshOpticalFlowBackward'][tripletFrameIndex + 1]
            motionVectorCurrentToNext = sequence['MeshOpticalFlowForward'][tripletFrameIndex + 1]

            # Calculate occlusion masks for the motion vectors
            occlusionPreviousToCurrent = EstimateOcclusion(motionVectorPreviousToCurrent, 1, occlusionArtifactFilterSize)
            occlusionCurrentToPrevious = EstimateOcclusion(motionVectorCurrentToPrevious, 1, occlusionArtifactFilterSize)
            occlusionNextToCurrent = EstimateOcclusion(motionVectorNextToCurrent, 1, occlusionArtifactFilterSize)
            occlusionCurrentToNext = EstimateOcclusion(motionVectorCurrentToNext, 1, occlusionArtifactFilterSize)

            # Temporal mask for temporal loss term includes forward and backward occlusion
            temporalMask = WarpImage(occlusionCurrentToPrevious, motionVectorPreviousToCurrent) * occlusionPreviousToCurrent
            temporalMask *= WarpImage(occlusionCurrentToNext, motionVectorNextToCurrent) * occlusionNextToCurrent

            # Use sparse point flow (for the surface points) for sparse point warping
            sparseMotionVectorPreviousToCurrent = sequence['PointsSparseOpticalFlowForward'][tripletFrameIndex] * sequence['PointsSparseSurface'][tripletFrameIndex]
            sparseMotionVectorCurrentToPrevious = sequence['PointsSparseOpticalFlowBackward'][tripletFrameIndex] * sequence['PointsSparseSurface'][tripletFrameIndex - 1]
            sparseMotionVectorNextToCurrent = sequence['PointsSparseOpticalFlowBackward'][tripletFrameIndex + 1] * sequence['PointsSparseSurface'][tripletFrameIndex]
            sparseMotionVectorCurrentToNext = sequence['PointsSparseOpticalFlowForward'][tripletFrameIndex + 1] * sequence['PointsSparseSurface'][tripletFrameIndex + 1]

            # Calculate occlusion masks for the sparse surface motion vectors
            sparseOcclusionPreviousToCurrent = EstimateOcclusion(sparseMotionVectorPreviousToCurrent, 1, occlusionArtifactFilterSize)
            sparseOcclusionCurrentToPrevious = EstimateOcclusion(sparseMotionVectorCurrentToPrevious, 1, occlusionArtifactFilterSize)
            sparseOcclusionNextToCurrent = EstimateOcclusion(sparseMotionVectorNextToCurrent, 1, occlusionArtifactFilterSize)
            sparseOcclusionCurrentToNext = EstimateOcclusion(sparseMotionVectorCurrentToNext, 1, occlusionArtifactFilterSize)

            # Sparse temporal mask
            sparseTemporalMask = WarpImage(sparseOcclusionCurrentToPrevious, sparseMotionVectorPreviousToCurrent) * sparseOcclusionPreviousToCurrent
            sparseTemporalMask *= WarpImage(sparseOcclusionCurrentToNext, sparseMotionVectorNextToCurrent) * sparseOcclusionNextToCurrent

            # Surface Classification Model
            outputPreviousSCM = outputsSCM[tripletFrameIndex - 1]
            outputPreviousWarpedSCM = outputPreviousSCM * sparseOcclusionCurrentToPrevious
            outputPreviousWarpedSCM = WarpImage(outputPreviousWarpedSCM, sparseMotionVectorPreviousToCurrent)
            outputPreviousWarpedSCM *= sparseOcclusionPreviousToCurrent
            outputSCM = outputsSCM[tripletFrameIndex]
            outputNextSCM = outputsSCM[tripletFrameIndex + 1]
            outputNextWarpedSCM = outputNextSCM * sparseOcclusionCurrentToNext
            outputNextWarpedSCM = WarpImage(outputNextWarpedSCM, sparseMotionVectorNextToCurrent)
            outputNextWarpedSCM *= sparseOcclusionNextToCurrent
            targetSCM = targetsSCM[tripletFrameIndex]

            lossSurfaceTripletSCM = factorLossSurfaceSCM * torch.nn.functional.binary_cross_entropy(outputSCM, targetSCM, reduction='mean')
            lossTemporalTripletPreviousSCM = factorLossTemporalSCM * torch.nn.functional.l1_loss(sparseTemporalMask * outputPreviousWarpedSCM, sparseTemporalMask * outputSCM, reduction='mean')
            lossTemporalTripletNextSCM = factorLossTemporalSCM * torch.nn.functional.l1_loss(sparseTemporalMask * outputNextWarpedSCM, sparseTemporalMask * outputSCM, reduction='mean')
            lossTemporalTripletSCM = lossTemporalTripletPreviousSCM + lossTemporalTripletNextSCM
            lossSurfaceSCM.append(lossSurfaceTripletSCM)
            lossTemporalSCM.append(lossTemporalTripletSCM)

            # Surface Flow Model
            outputPreviousSFM = outputsSFM[tripletFrameIndex - 1]
            outputPreviousWarpedSFM = outputPreviousSFM * occlusionCurrentToPrevious
            outputPreviousWarpedSFM = WarpImage(outputPreviousWarpedSFM, motionVectorPreviousToCurrent)
            outputPreviousWarpedSFM *= occlusionPreviousToCurrent
            outputSFM = outputsSFM[tripletFrameIndex]
            outputNextSFM = outputsSFM[tripletFrameIndex + 1]
            outputNextWarpedSFM = outputNextSFM * occlusionCurrentToNext
            outputNextWarpedSFM = WarpImage(outputNextWarpedSFM, motionVectorNextToCurrent)
            outputNextWarpedSFM *= occlusionNextToCurrent
            targetSFM = targetsSFM[tripletFrameIndex]

            lossOpticalFlowTripletSFM = factorLossOpticalFlowSFM * torch.nn.functional.l1_loss(outputSFM, targetSFM, reduction='mean')
            lossTemporalTripletPreviousSFM = factorLossTemporalSFM * torch.nn.functional.l1_loss(temporalMask * outputPreviousWarpedSFM, temporalMask * outputSFM, reduction='mean')
            lossTemporalTripletNextSFM = factorLossTemporalSFM * torch.nn.functional.l1_loss(temporalMask * outputNextWarpedSFM, temporalMask * outputSFM, reduction='mean')
            lossTemporalTripletSFM = lossTemporalTripletPreviousSFM + lossTemporalTripletNextSFM
            lossOpticalFlowSFM.append(lossOpticalFlowTripletSFM)
            lossTemporalSFM.append(lossTemporalTripletSFM)

            # Surface Reconstruction Model
            outputPreviousSRM = outputsSRM[tripletFrameIndex - 1]
            outputPreviousWarpedSRM = outputPreviousSRM * occlusionCurrentToPrevious
            outputPreviousWarpedSRM = WarpImage(outputPreviousWarpedSRM, motionVectorPreviousToCurrent)
            outputPreviousWarpedSRM *= occlusionPreviousToCurrent
            outputSRM = outputsSRM[tripletFrameIndex]
            outputNextSRM = outputsSRM[tripletFrameIndex + 1]
            outputNextWarpedSRM = outputNextSRM * occlusionCurrentToNext
            outputNextWarpedSRM = WarpImage(outputNextWarpedSRM, motionVectorNextToCurrent)
            outputNextWarpedSRM *= occlusionNextToCurrent
            targetSRM = targetsSRM[tripletFrameIndex]

            lossSurfaceTripletSRM = factorLossSurfaceSRM * torch.nn.functional.binary_cross_entropy(outputSRM[:, 0:1, :, :], targetSRM[:, 0:1, :, :], reduction='mean')
            lossDepthTripletSRM = factorLossDepthSRM * torch.nn.functional.l1_loss(outputSRM[:, 1:2, :, :], targetSRM[:, 1:2, :, :], reduction='mean')
            lossColorTripletSRM = factorLossColorSRM * torch.nn.functional.l1_loss(outputSRM[:, 2:5, :, :], targetSRM[:, 2:5, :, :], reduction='mean')
            lossNormalTripletSRM = factorLossNormalSRM * torch.nn.functional.l1_loss(outputSRM[:, 5:8, :, :], targetSRM[:, 5:8, :, :], reduction='mean')
            lossTemporalTripletPreviousSRM = factorLossTemporalSRM * torch.nn.functional.l1_loss(temporalMask * outputPreviousWarpedSRM, temporalMask * outputSRM, reduction='mean')
            lossTemporalTripletNextSRM = factorLossTemporalSRM * torch.nn.functional.l1_loss(temporalMask * outputNextWarpedSRM, temporalMask * outputSRM, reduction='mean')
            lossTemporalTripletSRM = lossTemporalTripletPreviousSRM + lossTemporalTripletNextSRM
            lossSurfaceSRM.append(lossSurfaceTripletSRM)
            lossDepthSRM.append(lossDepthTripletSRM)
            lossColorSRM.append(lossColorTripletSRM)
            lossNormalSRM.append(lossNormalTripletSRM)
            lossTemporalSRM.append(lossTemporalTripletSRM)

            if trainingAdversarialSRM:
                # Do not consider the previous frame output from SRM here (slice it away)
                inputPreviousSRM = inputsSRM[tripletFrameIndex - 1][:, 0:8, :, :]
                inputPreviousWarpedSRM = inputPreviousSRM * sparseOcclusionCurrentToPrevious
                inputPreviousWarpedSRM = WarpImage(inputPreviousWarpedSRM, sparseMotionVectorPreviousToCurrent)
                inputPreviousWarpedSRM *= sparseOcclusionPreviousToCurrent
                inputSRM = inputsSRM[tripletFrameIndex][:, 0:8, :, :]
                inputNextSRM = inputsSRM[tripletFrameIndex + 1][:, 0:8, :, :]
                inputNextWarpedSRM = inputNextSRM * sparseOcclusionCurrentToNext
                inputNextWarpedSRM = WarpImage(inputNextWarpedSRM, sparseMotionVectorNextToCurrent)
                inputNextWarpedSRM *= sparseOcclusionNextToCurrent

                targetPreviousSRM = targetsSRM[tripletFrameIndex - 1]
                targetPreviousWarpedSRM = targetPreviousSRM * occlusionCurrentToPrevious
                targetPreviousWarpedSRM = WarpImage(targetPreviousWarpedSRM, motionVectorPreviousToCurrent)
                targetPreviousWarpedSRM *= occlusionPreviousToCurrent
                targetNextSRM = targetsSRM[tripletFrameIndex + 1]
                targetNextWarpedSRM = targetNextSRM * occlusionCurrentToNext
                targetNextWarpedSRM = WarpImage(targetNextWarpedSRM, motionVectorNextToCurrent)
                targetNextWarpedSRM *= occlusionNextToCurrent

                sequenceInputSRM = [inputPreviousWarpedSRM, inputSRM, inputNextWarpedSRM]
                sequenceOutputSRM = [outputPreviousWarpedSRM, outputSRM, outputNextWarpedSRM]
                sequenceTargetSRM = [targetPreviousWarpedSRM, targetSRM, targetNextWarpedSRM]

                if reverseCriticSequence:
                    sequenceInputSRM.reverse()
                    sequenceOutputSRM.reverse()
                    sequenceTargetSRM.reverse()

                sequenceInputSRM = torch.cat(sequenceInputSRM, dim=1)
                sequenceOutputSRM = torch.cat(sequenceOutputSRM, dim=1)
                sequenceTargetSRM = torch.cat(sequenceTargetSRM, dim=1)

                sequenceRealSRM = torch.cat([sequenceInputSRM, sequenceTargetSRM], dim=1)
                sequenceFakeSRM = torch.cat([sequenceInputSRM, sequenceOutputSRM], dim=1)

                criticRealSRM = criticSRM(sequenceRealSRM)
                criticFakeSRM = criticSRM(sequenceFakeSRM)

                lossWassersteinTripletSRM = -criticFakeSRM
                lossCriticWassersteinTripletSRM = criticFakeSRM - criticRealSRM

                lossWassersteinSRM.append(lossWassersteinTripletSRM)
                lossCriticWassersteinSRM.append(lossCriticWassersteinTripletSRM)

                # Only add computationally expensive gradient penalty term for improved wasserstein GAN training if it is necessary
                if updateCriticSRM:
                    lossCriticGradientPenaltyTripletSRM = GradientPenalty(criticSRM, sequenceRealSRM, sequenceFakeSRM)
                    lossCriticGradientPenaltySRM.append(lossCriticGradientPenaltyTripletSRM)

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

        if trainingAdversarialSRM:
            lossWassersteinSRM = torch.stack(lossWassersteinSRM, dim=0).mean()
            lossCriticWassersteinSRM = torch.stack(lossCriticWassersteinSRM, dim=0).mean()

            if updateCriticSRM:
                lossCriticGradientPenaltySRM = torch.stack(lossCriticGradientPenaltySRM, dim=0).mean()

        # Combine partial losses into a single final loss term
        lossSCM = lossSurfaceSCM + lossTemporalSCM
        lossSFM = lossOpticalFlowSFM + lossTemporalSFM
        lossSRM = lossSurfaceSRM + lossDepthSRM + lossColorSRM + lossNormalSRM + lossTemporalSRM

        if trainingAdversarialSRM:
            lossSRM += lossWassersteinSRM

            if updateCriticSRM:
                lossCriticSRM = lossCriticWassersteinSRM + lossCriticGradientPenaltySRM

        # TODO: Pretrain SCM, then pretrain SFM and lastly train SRM

        # Train Surface Classification Model
        SCM.zero_grad()
        lossSCM.backward(retain_graph=True)
        optimizerSCM.step()

        summaryWriter.add_scalar('Surface Classification Model/_Loss SCM', lossSCM, iteration)
        summaryWriter.add_scalar('Surface Classification Model/Loss Surface SCM', lossSurfaceSCM, iteration)
        summaryWriter.add_scalar('Surface Classification Model/Loss Temporal SCM', lossTemporalSCM, iteration)

        # Train Surface Flow Model
        SFM.zero_grad()
        lossSFM.backward(retain_graph=True)
        optimizerSFM.step()

        summaryWriter.add_scalar('Surface Flow Model/_Loss SFM', lossSFM, iteration)
        summaryWriter.add_scalar('Surface Flow Model/Loss Optical Flow SFM', lossOpticalFlowSFM, iteration)
        summaryWriter.add_scalar('Surface Flow Model/Loss Temporal SFM', lossTemporalSFM, iteration)

        # Train Surface Reconstruction Model
        updateSRM = True

        if trainingAdversarialSRM:
            if updateCriticSRM:
                updateSRM = False
                criticSRM.zero_grad()
                lossCriticSRM.backward(retain_graph=False)
                optimizerCriticSRM.step()
                stepsCriticSRM += 1

                summaryWriter.add_scalar('Surface Reconstruction Critic/_Loss Critic SRM', lossCriticSRM, iteration)
                summaryWriter.add_scalar('Surface Reconstruction Critic/Loss Critic Wasserstein SRM', lossCriticWassersteinSRM, iteration)
                summaryWriter.add_scalar('Surface Reconstruction Critic/Loss Critic Gradient Penalty SRM', lossCriticGradientPenaltySRM, iteration)

        if updateSRM:
            SRM.zero_grad()
            lossSRM.backward(retain_graph=False)
            optimizerSRM.step()

            summaryWriter.add_scalar('Surface Reconstruction Model/_Loss SRM', lossSRM, iteration)
            summaryWriter.add_scalar('Surface Reconstruction Model/Loss Surface SRM', lossSurfaceSRM, iteration)
            summaryWriter.add_scalar('Surface Reconstruction Model/Loss Depth SRM', lossDepthSRM, iteration)
            summaryWriter.add_scalar('Surface Reconstruction Model/Loss Color SRM', lossColorSRM, iteration)
            summaryWriter.add_scalar('Surface Reconstruction Model/Loss Normal SRM', lossNormalSRM, iteration)
            summaryWriter.add_scalar('Surface Reconstruction Model/Loss Temporal SRM', lossTemporalSRM, iteration)

            if trainingAdversarialSRM:
                stepsSRM += 1
                summaryWriter.add_scalar('Surface Reconstruction Model/Loss Wasserstein SRM', lossWassersteinSRM, iteration)

        # Need to detach previous output for next iteration (since using recurrent architecture)
        previousOutputSRM = previousOutputSRM.detach()

        # Update wasserstein loss ratios for adaptive WGAN training
        if trainingAdversarialSRM:
            if lossCriticWassersteinPreviousSRM is not None and lossWassersteinPreviousSRM is not None:
                ratioCriticSRM = torch.abs((lossCriticWassersteinSRM - lossCriticWassersteinPreviousSRM) / (lossCriticWassersteinPreviousSRM + 1e-8))
                ratioSRM = torch.abs((lossWassersteinSRM - lossWassersteinPreviousSRM) / (lossWassersteinPreviousSRM + 1e-8))

            lossCriticWassersteinPreviousSRM = lossCriticWassersteinSRM
            lossWassersteinPreviousSRM = lossWassersteinSRM

        # Update learning rate using schedulers
        if iteration % schedulerDecaySkip == (schedulerDecaySkip - 1):
            schedulerSCM.step()
            schedulerSFM.step()
            schedulerSRM.step()
            
            if trainingAdversarialSRM:
                schedulerCriticSRM.step()

            PrintLearningRates()

        # Print progress, save checkpoints, create snapshots and plot loss graphs in certain intervals
        if iteration % snapshotSkip == (snapshotSkip - 1):
            snapshotIndex = (iteration // snapshotSkip)
            snapshotSampleIndex = snapshotIndex % batchSize
            snapshotFrameIndex = 1 + (snapshotIndex % (dataset.sequenceFrameCount - 2))

            progress = 'Epoch:\t' + str(epoch) + '\t' + str(int(100 * (batchIndex / batchCount))) + '%'

            if trainingAdversarialSRM:
                progress += '\tStepsCriticSRM: ' + str(stepsCriticSRM) + '\tStepsSRM: ' + str(stepsSRM)

            print(progress)

            # Temporal Masks
            fig = plt.figure(figsize=(2 * frameWidth, 1 * frameHeight), dpi=1)
            fig.add_subplot(1, 2, 1).title#.set_text('Temporal Mask')
            plt.imshow(TensorToImage(temporalMask[snapshotSampleIndex]))
            plt.axis('off')
            fig.add_subplot(1, 2, 2).title#.set_text('Sparse Temporal Mask')
            plt.imshow(TensorToImage(sparseTemporalMask[snapshotSampleIndex]))
            plt.axis('off')
            
            plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
            plt.margins(0, 0)
            summaryWriter.add_figure('Temporal Mask/Epoch' + str(epoch), plt.gcf(), iteration)

            # Surface Classification Model
            inputPointsSparseForegroundSCM = inputsSCM[snapshotFrameIndex][snapshotSampleIndex, 0:1, :, :]
            inputPointsSparseDepthSCM = inputsSCM[snapshotFrameIndex][snapshotSampleIndex, 1:2, :, :]
            inputPointsSparseNormalSCM = inputsSCM[snapshotFrameIndex][snapshotSampleIndex, 2:5, :, :]
            outputPointsSparseSurface = outputsSCM[snapshotFrameIndex][snapshotSampleIndex, :, :, :]
            outputPointsSparseSurfaceMask = outputPointsSparseSurface.ge(0.5).float()
            targetPointsSparseSurface = targetsSCM[snapshotFrameIndex][snapshotSampleIndex, :, :, :]

            fig = plt.figure(figsize=(3 * frameWidth, 2 * frameHeight), dpi=1)
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

            fig = plt.figure(figsize=(3 * frameWidth, 2 * frameHeight), dpi=1)
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

            fig = plt.figure(figsize=(4 * frameWidth, 4 * frameHeight), dpi=1)
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

            if trainingAdversarialSRM:
                sequenceInputPreviousWarpedSRM = sequenceInputSRM[snapshotSampleIndex, 0:8, :, :]
                sequenceInputCurrentSRM = sequenceInputSRM[snapshotSampleIndex, 8:16, :, :]
                sequenceInputNextWarpedSRM = sequenceInputSRM[snapshotSampleIndex, 16:24, :, :]
                sequenceOutputPreviousWarpedSRM = sequenceOutputSRM[snapshotSampleIndex, 0:8, :, :]
                sequenceOutputCurrentSRM = sequenceOutputSRM[snapshotSampleIndex, 8:16, :, :]
                sequenceOutputNextWarpedSRM = sequenceOutputSRM[snapshotSampleIndex, 16:24, :, :]
                sequenceTargetPreviousWarpedSRM = sequenceTargetSRM[snapshotSampleIndex, 0:8, :, :]
                sequenceTargetCurrentSRM = sequenceTargetSRM[snapshotSampleIndex, 8:16, :, :]
                sequenceTargetNextWarpedSRM = sequenceTargetSRM[snapshotSampleIndex, 16:24, :, :]

                fig = plt.figure(figsize=(6 * frameWidth, 6 * frameHeight), dpi=1)
                fig.add_subplot(6, 6, 1).title#.set_text('Sequence Input Surface Previous Warped')
                plt.imshow(TensorToImage(sequenceInputPreviousWarpedSRM[0:1, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 2).title#.set_text('Sequence Input Current Surface')
                plt.imshow(TensorToImage(sequenceInputCurrentSRM[0:1, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 3).title#.set_text('Sequence Input Surface Next Warped')
                plt.imshow(TensorToImage(sequenceInputNextWarpedSRM[0:1, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 7).title#.set_text('Sequence Output Surface Previous Warped')
                plt.imshow(TensorToImage(sequenceOutputPreviousWarpedSRM[0:1, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 8).title#.set_text('Sequence Output Current Surface')
                plt.imshow(TensorToImage(sequenceOutputCurrentSRM[0:1, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 9).title#.set_text('Sequence Output Surface Next Warped')
                plt.imshow(TensorToImage(sequenceOutputNextWarpedSRM[0:1, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 13).title#.set_text('Sequence Target Surface Previous Warped')
                plt.imshow(TensorToImage(sequenceTargetPreviousWarpedSRM[0:1, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 14).title#.set_text('Sequence Target Current Surface')
                plt.imshow(TensorToImage(sequenceTargetCurrentSRM[0:1, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 15).title#.set_text('Sequence Target Surface Next Warped')
                plt.imshow(TensorToImage(sequenceTargetNextWarpedSRM[0:1, :, :]))
                plt.axis('off')

                fig.add_subplot(6, 6, 4).title#.set_text('Sequence Input Depth Previous Warped')
                plt.imshow(TensorToImage(sequenceInputPreviousWarpedSRM[1:2, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 5).title#.set_text('Sequence Input Current Depth')
                plt.imshow(TensorToImage(sequenceInputCurrentSRM[1:2, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 6).title#.set_text('Sequence Input Depth Next Warped')
                plt.imshow(TensorToImage(sequenceInputNextWarpedSRM[1:2, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 10).title#.set_text('Sequence Output Depth Previous Warped')
                plt.imshow(TensorToImage(sequenceOutputPreviousWarpedSRM[1:2, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 11).title#.set_text('Sequence Output Current Depth')
                plt.imshow(TensorToImage(sequenceOutputCurrentSRM[1:2, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 12).title#.set_text('Sequence Output Depth Next Warped')
                plt.imshow(TensorToImage(sequenceOutputNextWarpedSRM[1:2, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 16).title#.set_text('Sequence Target Depth Previous Warped')
                plt.imshow(TensorToImage(sequenceTargetPreviousWarpedSRM[1:2, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 17).title#.set_text('Sequence Target Current Depth')
                plt.imshow(TensorToImage(sequenceTargetCurrentSRM[1:2, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 18).title#.set_text('Sequence Target Depth Next Warped')
                plt.imshow(TensorToImage(sequenceTargetNextWarpedSRM[1:2, :, :]))
                plt.axis('off')

                fig.add_subplot(6, 6, 19).title#.set_text('Sequence Input Color Previous Warped')
                plt.imshow(TensorToImage(sequenceInputPreviousWarpedSRM[2:5, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 20).title#.set_text('Sequence Input Current Color')
                plt.imshow(TensorToImage(sequenceInputCurrentSRM[2:5, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 21).title#.set_text('Sequence Input Color Next Warped')
                plt.imshow(TensorToImage(sequenceInputNextWarpedSRM[2:5, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 25).title#.set_text('Sequence Output Color Previous Warped')
                plt.imshow(TensorToImage(sequenceOutputPreviousWarpedSRM[2:5, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 26).title#.set_text('Sequence Output Current Color')
                plt.imshow(TensorToImage(sequenceOutputCurrentSRM[2:5, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 27).title#.set_text('Sequence Output Color Next Warped')
                plt.imshow(TensorToImage(sequenceOutputNextWarpedSRM[2:5, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 31).title#.set_text('Sequence Target Color Previous Warped')
                plt.imshow(TensorToImage(sequenceTargetPreviousWarpedSRM[2:5, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 32).title#.set_text('Sequence Target Current Color')
                plt.imshow(TensorToImage(sequenceTargetCurrentSRM[2:5, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 33).title#.set_text('Sequence Target Color Next Warped')
                plt.imshow(TensorToImage(sequenceTargetNextWarpedSRM[2:5, :, :]))
                plt.axis('off')

                fig.add_subplot(6, 6, 22).title#.set_text('Sequence Input Normal Previous Warped')
                plt.imshow(TensorToImage(sequenceInputPreviousWarpedSRM[5:8, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 23).title#.set_text('Sequence Input Current Normal')
                plt.imshow(TensorToImage(sequenceInputCurrentSRM[5:8, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 24).title#.set_text('Sequence Input Normal Next Warped')
                plt.imshow(TensorToImage(sequenceInputNextWarpedSRM[5:8, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 28).title#.set_text('Sequence Output Normal Previous Warped')
                plt.imshow(TensorToImage(sequenceOutputPreviousWarpedSRM[5:8, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 29).title#.set_text('Sequence Output Current Normal')
                plt.imshow(TensorToImage(sequenceOutputCurrentSRM[5:8, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 30).title#.set_text('Sequence Output Normal Next Warped')
                plt.imshow(TensorToImage(sequenceOutputNextWarpedSRM[5:8, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 34).title#.set_text('Sequence Target Normal Previous Warped')
                plt.imshow(TensorToImage(sequenceTargetPreviousWarpedSRM[5:8, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 35).title#.set_text('Sequence Target Current Normal')
                plt.imshow(TensorToImage(sequenceTargetCurrentSRM[5:8, :, :]))
                plt.axis('off')
                fig.add_subplot(6, 6, 36).title#.set_text('Sequence Target Normal Next Warped')
                plt.imshow(TensorToImage(sequenceTargetNextWarpedSRM[5:8, :, :]))
                plt.axis('off')

                plt.subplots_adjust(top=1, bottom=0, right=1, left=0, hspace=0, wspace=0)
                plt.margins(0, 0)
                summaryWriter.add_figure('Surface Reconstruction Critic/Epoch' + str(epoch), plt.gcf(), iteration)

            # Save an animated gif with input, output and target (quality is worse due to compression)
            videoTensor = torch.zeros((1, dataset.sequenceFrameCount, 3, 3 * frameHeight, 4 * frameWidth), dtype=torch.float, device=device)

            for frameIndex in range(dataset.sequenceFrameCount):
                # Surface
                videoTensor[0, frameIndex, :, 0:frameHeight, 0:frameWidth] = inputsSRM[frameIndex][snapshotSampleIndex, 0:1, :, :].repeat(3, 1, 1)
                videoTensor[0, frameIndex, :, frameHeight:2*frameHeight, 0:frameWidth] = outputsSRM[frameIndex][snapshotSampleIndex, 0:1, :, :].repeat(3, 1, 1)
                videoTensor[0, frameIndex, :, 2*frameHeight:3*frameHeight, 0:frameWidth] = targetsSRM[frameIndex][snapshotSampleIndex, 0:1, :, :].repeat(3, 1, 1)
                
                # Depth
                videoTensor[0, frameIndex, :, 0:frameHeight, frameWidth:2*frameWidth] = inputsSRM[frameIndex][snapshotSampleIndex, 1:2, :, :].repeat(3, 1, 1)
                videoTensor[0, frameIndex, :, frameHeight:2*frameHeight, frameWidth:2*frameWidth] = outputsSRM[frameIndex][snapshotSampleIndex, 1:2, :, :].repeat(3, 1, 1)
                videoTensor[0, frameIndex, :, 2*frameHeight:3*frameHeight, frameWidth:2*frameWidth] = targetsSRM[frameIndex][snapshotSampleIndex, 1:2, :, :].repeat(3, 1, 1)

                # Color
                videoTensor[0, frameIndex, :, 0:frameHeight, 2*frameWidth:3*frameWidth] = inputsSRM[frameIndex][snapshotSampleIndex, 2:5, :, :]
                videoTensor[0, frameIndex, :, frameHeight:2*frameHeight, 2*frameWidth:3*frameWidth] = outputsSRM[frameIndex][snapshotSampleIndex, 2:5, :, :]
                videoTensor[0, frameIndex, :, 2*frameHeight:3*frameHeight, 2*frameWidth:3*frameWidth] = targetsSRM[frameIndex][snapshotSampleIndex, 2:5, :, :]

                # Normal
                videoTensor[0, frameIndex, :, 0:frameHeight, 3*frameWidth:4*frameWidth] = inputsSRM[frameIndex][snapshotSampleIndex, 5:8, :, :]
                videoTensor[0, frameIndex, :, frameHeight:2*frameHeight, 3*frameWidth:4*frameWidth] = outputsSRM[frameIndex][snapshotSampleIndex, 5:8, :, :]
                videoTensor[0, frameIndex, :, 2*frameHeight:3*frameHeight, 3*frameWidth:4*frameWidth] = targetsSRM[frameIndex][snapshotSampleIndex, 5:8, :, :]

            videoTensor = torch.clamp(videoTensor, 0, 1)
            summaryWriter.add_video('Videos/Epoch' + str(epoch), videoTensor, iteration, fps=10)

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

            if trainingAdversarialSRM:
                checkpoint['CriticSRM'] = criticSRM.state_dict()
                checkpoint['OptimizerCriticSRM'] = optimizerCriticSRM.state_dict()
                checkpoint['SchedulerCriticSRM'] = schedulerCriticSRM.state_dict()
                checkpoint['StepsCriticSRM'] = stepsCriticSRM
                checkpoint['StepsSRM'] = stepsSRM

            torch.save(checkpoint, checkpointDirectory + checkpointNameStart + checkpointNameEnd)
            torch.save(checkpoint, checkpointDirectory + checkpointNameStart + str(epoch) + checkpointNameEnd)

    # Move to next epoch
    numpy.random.shuffle(randomIndices)
    torch.cuda.empty_cache()
    batchIndexStart = 0
    epoch += 1