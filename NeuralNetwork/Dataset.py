import os
import torch
import numpy
from zipfile import ZipFile
from Utils import *

def LoadTextureFromBytes(textureBytes):
    dataTypeMap = { 2 : 'float16', 4 : 'float32' }
    width = numpy.frombuffer(buffer=textureBytes, dtype='int32', count=1, offset=0).item()
    height = numpy.frombuffer(buffer=textureBytes, dtype='int32', count=1, offset=4).item()
    channels = numpy.frombuffer(buffer=textureBytes, dtype='int32', count=1, offset=8).item()
    elementSizeInBytes = numpy.frombuffer(buffer=textureBytes, dtype='int32', count=1, offset=12).item()
    texture = numpy.frombuffer(buffer=textureBytes, dtype=dataTypeMap[elementSizeInBytes], count=-1, offset=16)
    texture = numpy.reshape(texture, (height, width, channels))
    texture = texture.astype('float32')

    return texture

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
    def __init__(self, directory, sequenceFrameCount=3, testSetPercentage=0.0):
        self.directory = directory
        self.sequenceFrameCount = sequenceFrameCount
        self.testSetPercentage = testSetPercentage
        
        if self.sequenceFrameCount < 3:
            print('Error: Sequence frame count must at least be 3!')
            exit(0)

        frameIndices = []
        filenames = os.listdir(self.directory)

        for filename in filenames:
            if filename.lower().endswith('.zip'):
                frameIndex = int(filename.lower().split('.zip')[0])
                frameIndices.append(frameIndex)

        frameIndices.sort()
        frameCount = len(frameIndices)

        self.trainingFrames = frameIndices[int(testSetPercentage * frameCount):frameCount]
        self.testFrames = frameIndices[0:int(testSetPercentage * frameCount)]
        self.trainingSequenceCount = max(0, len(self.trainingFrames) - (self.sequenceFrameCount - 1))
        self.testSequenceCount = max(0, len(self.testFrames) - (self.sequenceFrameCount - 1))
        self.renderModes = self.GetFrame(frameIndices, 0).keys()

        print('Initialized dataset from "' + directory + '"')
        print('\t- ' + str(frameCount) + ' total frames')
        print('\t- ' + str(self.sequenceFrameCount) + ' frames per sequence')
        print('\t- ' + str(self.trainingSequenceCount) + ' training sequences')
        print('\t- ' + str(self.testSequenceCount) + ' test sequences')
        print('\t- ' + str(self.renderModes) + ' render modes ')

    def GetFrame(self, frames, index):
        archive = ZipFile(self.directory + str(frames[index]) + '.zip', 'r')

        tensors = {}

        for filename in archive.namelist():
            renderMode = filename.split('_')[0]

            textureBytes = archive.read(filename)
            texture = LoadTextureFromBytes(textureBytes)
            texture = torch.from_numpy(texture)
            texture = texture.to(device)
            texture = torch.permute(texture, dims=(2, 0, 1))

            # Normalize depth into [0, 1] range and add a foreground/background mask
            if renderMode.find('Depth') >= 0:
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

            tensors[renderMode] = texture

        # Add point occlusion mask
        meshDepth = tensors['MeshDepth']
        pointDepth = tensors['PointsDepth']
        pointsSparseDepth = tensors['PointsSparseDepth']

        depthEpsilon = 1e-3
        pointsOcclusion = torch.abs(meshDepth - pointDepth) > depthEpsilon
        pointsSparseOcclusion = torch.abs(meshDepth - pointsSparseDepth) > depthEpsilon
        pointsOcclusion[tensors['PointsBackground'] > 0.5] = 0.0
        pointsSparseOcclusion[tensors['PointsSparseBackground'] > 0.5] = 0.0
        pointsOcclusion = pointsOcclusion.float()
        pointsSparseOcclusion = pointsSparseOcclusion.float()

        tensors['PointsOcclusion'] = pointsOcclusion
        tensors['PointsSparseOcclusion'] = pointsSparseOcclusion

        # Normalize depth after occlusion mask calculation
        # Be careful: normalized depth can be quite different between points and mesh due to occluded pixel depth
        tensorNames = tensors.keys()

        for tensorName in tensorNames:
            if tensorName.find('Depth') >= 0:
                viewMode = tensorName.split('Depth')[0]

                tensors[tensorName] = NormalizeDepthTexture(tensors[tensorName], tensors[viewMode + 'Foreground'].bool(), tensors[viewMode + 'Background'].bool())

        return tensors

    def GetSequence(self, frames, sequenceIndex):
        sequence = []

        for frameIndex in range(self.sequenceFrameCount):
            tensors = self.GetFrame(frames, sequenceIndex + frameIndex)
            sequence.append(tensors)

        return sequence

    def GetTrainingSequenceCount(self):
        return self.trainingSequenceCount

    def GetTestSequenceCount(self):
        return self.testSequenceCount

    def GetTrainingSequence(self, trainingSequenceIndex):
        return self.GetSequence(self.trainingFrames, trainingSequenceIndex)

    def GetTestSequence(self, testSequenceIndex):
        return self.GetSequence(self.testFrames, testSequenceIndex)