import os
import torch
import numpy
import random
from zipfile import ZipFile
from Utils import *

def LoadTexturesFromBytes(texturesBytes):
    dataTypeMap = { 2 : 'float16', 4 : 'float32' }

    totalBytesToRead = len(texturesBytes)
    bytesRead = 0
    textures = {}

    while bytesRead < totalBytesToRead:
        renderModeNameSize = numpy.frombuffer(buffer=texturesBytes, dtype='int32', count=1, offset=bytesRead).item()
        bytesRead += 4
        renderMode = texturesBytes[bytesRead:bytesRead+renderModeNameSize].decode('utf-16')
        bytesRead += renderModeNameSize
        width = numpy.frombuffer(buffer=texturesBytes, dtype='int32', count=1, offset=bytesRead).item()
        bytesRead += 4
        height = numpy.frombuffer(buffer=texturesBytes, dtype='int32', count=1, offset=bytesRead).item()
        bytesRead += 4
        channels = numpy.frombuffer(buffer=texturesBytes, dtype='int32', count=1, offset=bytesRead).item()
        bytesRead += 4
        elementSizeInBytes = numpy.frombuffer(buffer=texturesBytes, dtype='int32', count=1, offset=bytesRead).item()
        bytesRead += 4
        textureElementCount = width * height * channels
        texture = numpy.frombuffer(buffer=texturesBytes, dtype=dataTypeMap[elementSizeInBytes], count=textureElementCount, offset=bytesRead)
        bytesRead += textureElementCount * elementSizeInBytes
        texture = numpy.reshape(texture, (height, width, channels))
        texture = texture.astype('float32')
        texture = torch.from_numpy(texture)
        texture = texture.to(device)
        texture = torch.permute(texture, dims=(2, 0, 1))

        textures[renderMode] = texture

    return textures

def GetForegroundBackgroundMasks(depthTexture):
    maskForeground = depthTexture < 1.0
    maskBackground = depthTexture >= 1.0

    return maskForeground, maskBackground

def NormalizeDepthTexture(depthTexture, maskForeground, maskBackground):
    depthNormalized = torch.clone(depthTexture)

    # Replace background depth values with 0 for better visualization
    depthNormalized[maskBackground] = 0.0

    # Compute minimum and maximum masked depth values
    depthMin = depthTexture[maskForeground].min()
    depthMax = depthTexture[maskForeground].max()

    # Normalize values to [0, 1]
    depthNormalized[maskForeground] = (depthNormalized[maskForeground] - depthMin) / (depthMax - depthMin)

    return depthNormalized

class Dataset:
    def __init__(self, directory, sequenceFrameCount=3, dataAugmentation=True, zipCompressed=False, testSetPercentage=0.0):
        self.directory = directory
        self.sequenceFrameCount = sequenceFrameCount
        self.dataAugmentation = dataAugmentation
        self.zipCompressed = zipCompressed
        self.testSetPercentage = testSetPercentage
        
        if self.sequenceFrameCount < 3:
            print('Error: Sequence frame count must at least be 3!')
            exit(0)

        # Either use compressed or uncompressed dataset files
        fileSuffix = '.zip' if zipCompressed else '.textures'

        frameIndices = []
        filenames = os.listdir(self.directory)

        for filename in filenames:
            if filename.lower().endswith(fileSuffix):
                frameIndex = int(filename.lower().split(fileSuffix)[0])
                frameIndices.append(frameIndex)

        frameIndices.sort()
        frameCount = len(frameIndices)

        self.trainingFrames = frameIndices[int(testSetPercentage * frameCount):frameCount]
        self.testFrames = frameIndices[0:int(testSetPercentage * frameCount)]
        self.trainingSequenceCount = max(0, len(self.trainingFrames) - (self.sequenceFrameCount - 1) - 1)
        self.testSequenceCount = max(0, len(self.testFrames) - (self.sequenceFrameCount - 1) - 1)
        self.renderModes = self.GetFrame(frameIndices, 0).keys()
        self.height = list(self.GetFrame(frameIndices, 0).values())[0].size(1)
        self.width = list(self.GetFrame(frameIndices, 0).values())[0].size(2)

        print('Initialized dataset from "' + directory + '"')
        print('\t- ' + str(frameCount) + ' total frames')
        print('\t- ' + str(self.sequenceFrameCount) + ' frames per sequence')
        print('\t- ' + str(self.trainingSequenceCount) + ' training sequences')
        print('\t- ' + str(self.testSequenceCount) + ' test sequences')
        print('\t- ' + str(len(self.renderModes)) + ' render modes')
        print('\t- ' + str(self.height) + ' height')
        print('\t- ' + str(self.width) + ' width')

    def GetFrame(self, frames, index):
        if self.zipCompressed:
            archive = ZipFile(self.directory + str(frames[index]) + '.zip', 'r')
            texturesBytes = archive.read(str(index) + '.textures')
        else:
            texturesBytes = open(self.directory + str(frames[index]) + '.textures', 'rb').read()

        textures = LoadTexturesFromBytes(texturesBytes)

        tensors = {}

        for renderMode in textures.keys():
            texture = textures[renderMode]

            # Normalize depth into [0, 1] range and add a foreground/background mask
            if 'Depth' in renderMode:
                viewMode = renderMode.split('Depth')[0]

                if viewMode == 'PullPush':
                    foreground = texture[3:4, :, :] >= 1.0
                    background = texture[3:4, :, :] <= 0.0
                    texture = texture[0:1, :, :]
                else:
                    foreground, background = GetForegroundBackgroundMasks(texture)

                # Add a foreground/background mask for each view mode
                tensors[viewMode + 'Foreground'] = foreground.float()
                tensors[viewMode + 'Background'] = background.float()
            else:
                # Slice away alpha channel
                texture = texture[0:3, :, :]

            # Slice away optical flow channels that are not needed
            if 'OpticalFlow' in renderMode:
                texture = texture[0:2, :, :]

            tensors[renderMode] = texture

        # Add point surface mask (only points that are on the surface have the value 1)
        meshDepth = tensors['MeshDepth']

        for renderMode in textures.keys():
            if 'Points' in renderMode and 'Depth' in renderMode:
                viewMode = renderMode.split('Depth')[0]
                pointsDepth = tensors[renderMode]

                depthEpsilon = 1e-2
                pointsSurface = torch.abs(meshDepth - pointsDepth) < depthEpsilon
                pointsSurface[tensors[viewMode + 'Background'] > 0.5] = False
                pointsSurface = pointsSurface.float()

                tensors[viewMode + 'Surface'] = pointsSurface

        # Normalize depth after surface mask calculation
        # Be careful: normalized depth can be quite different between points and mesh due to non-surface pixel depth
        tensorNames = tensors.keys()

        for tensorName in tensorNames:
            if 'Depth' in tensorName:
                viewMode = tensorName.split('Depth')[0]
                tensors[tensorName] = NormalizeDepthTexture(tensors[tensorName], tensors[viewMode + 'Foreground'].bool(), tensors[viewMode + 'Background'].bool())

        return tensors

    def GetSequence(self, frames, sequenceIndex):
        sequence = []

        # Get all the frames
        for frameIndex in range(sequenceIndex, sequenceIndex + self.sequenceFrameCount, 1):
            tensors = self.GetFrame(frames, frameIndex)
            sequence.append(tensors)

        # Possibly reverse the order of frames in the sequence
        # Then motion vectors need to be swapped with the one from the next frame (and also swap forward and backward motion)
        reverseSequence = self.dataAugmentation and random.randint(0, 1) == 0

        if reverseSequence:
            # Add another frame to the end that is required to swap the optical flow
            sequence.append(self.GetFrame(frames, sequenceIndex + self.sequenceFrameCount))

            # Swap motion vectors with the one from the next frame with swapped direction
            for frameIndex in range(self.sequenceFrameCount):
                tensors = sequence[frameIndex]
                tensorsNext = sequence[frameIndex + 1]

                for renderMode in self.renderModes:
                    if 'OpticalFlowForward' in renderMode:
                        viewMode = renderMode.split('OpticalFlowForward')[0]
                        tensors[viewMode + 'OpticalFlowForward'] = tensorsNext[viewMode + 'OpticalFlowBackward']
                        tensors[viewMode + 'OpticalFlowBackward'] = tensorsNext[viewMode + 'OpticalFlowForward']
                        sequence[frameIndex] = tensors

            # Remove previously added frame and actually reverse the frame order
            sequence.pop()
            sequence.reverse()

        if self.dataAugmentation:
            # Augment data by color shifting the frames according to a normal distribution with low variance
            brightness = max(0, min(2, random.normalvariate(1.0, 0.1)))
            contrast = max(0, min(2, random.normalvariate(1.0, 0.1)))
            hue = max(-0.5, min(0.5, random.normalvariate(0.0, 0.05)))
            saturation = max(0, min(2, random.normalvariate(1.0, 0.1)))

            # Apply the same color augmentation to all frames in the sequence
            for frameIndex in range(self.sequenceFrameCount):
                for renderMode in self.renderModes:
                    if 'Color' in renderMode:
                        sequence[frameIndex][renderMode] = ColorShift(sequence[frameIndex][renderMode], brightness, contrast, hue, saturation)

            # Randomly flip frames and motion vectors (either no flip or flip at x, y or x-y axis)
            flipDimIndex = random.randint(0, 3)

            if flipDimIndex > 0:
                flipDims = [[], [1], [2], [1, 2]]
                flipDim = flipDims[flipDimIndex]

                for frameIndex in range(self.sequenceFrameCount):
                    for renderMode in self.renderModes:
                        sequence[frameIndex][renderMode] = torch.flip(sequence[frameIndex][renderMode], flipDim)

                        # Need to also invert motion vector signs for the flipped axis
                        if 'OpticalFlow' in renderMode:
                            motionVector = sequence[frameIndex][renderMode]

                            if 1 in flipDim:
                                motionVector[1, :, :] *= -1.0

                            if 2 in flipDim:
                                motionVector[0, :, :] *= -1.0

                            sequence[frameIndex][renderMode] = motionVector

                        # Need to also invert normal vector sign in screen space for the flipped axis (also do this in [-1, 1] range)
                        if 'NormalScreen' in renderMode:
                            normalVector = (sequence[frameIndex][renderMode] * 2.0) - 1.0

                            if 1 in flipDim:
                                normalVector[1, :, :] *= -1.0

                            if 2 in flipDim:
                                normalVector[0, :, :] *= -1.0

                            sequence[frameIndex][renderMode] = (normalVector + 1.0) / 2.0

                    # Normal value can be (0, 0, 0) before flipping if there is no normal, need to handle this case to avoid error
                    for renderMode in self.renderModes:
                        if 'NormalScreen' in renderMode:
                            viewMode = renderMode.split('NormalScreen')[0]
                            sequence[frameIndex][renderMode] *= sequence[frameIndex][viewMode + 'Foreground']

            # Add a tiny bit of noise
            for frameIndex in range(self.sequenceFrameCount):
                for renderMode in self.renderModes:
                    sequence[frameIndex][renderMode] += torch.normal(mean=0.0, std=1.0 / 256.0, size=sequence[frameIndex][renderMode].shape, device=device)

        return sequence

    def GetTrainingSequenceCount(self):
        return self.trainingSequenceCount

    def GetTestSequenceCount(self):
        return self.testSequenceCount

    def GetTrainingSequence(self, trainingSequenceIndex):
        return self.GetSequence(self.trainingFrames, trainingSequenceIndex)

    def GetTestSequence(self, testSequenceIndex):
        return self.GetSequence(self.testFrames, testSequenceIndex)