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
    def __init__(self, directory, testSetPercentage=0.2):
        self.directory = directory
        self.entries = []

        filenames = os.listdir(self.directory)

        for filename in filenames:
            if filename.lower().endswith('.zip'):
                self.entries.append(self.directory + filename)

        self.entryCount = len(self.entries)
        self.trainingFrames = self.entries[int(testSetPercentage * self.entryCount):self.entryCount]
        self.testFrames = self.entries[0:int(testSetPercentage * self.entryCount)]
        self.trainingSequenceCount = len(self.trainingFrames) - 2
        self.testSequenceCount = len(self.testFrames) - 2

        print('Initialized dataset from "' + directory + '"')
        print('\t- ' + str(self.entryCount) + ' entries')
        print('\t- ' + str(self.trainingSequenceCount) + ' training sequences')
        print('\t- ' + str(self.testSequenceCount) + ' test sequences')

    def __len__(self):
        return self.frameCount - 2

    def GetFrame(self, frames, index):
        archive = ZipFile(frames[index], 'r')

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

                texture = NormalizeDepthTexture(texture, foreground, background)

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

        depthEpsilon = 0.1
        pointsOcclusion = torch.abs(meshDepth - pointDepth) > depthEpsilon
        pointsSparseOcclusion = torch.abs(meshDepth - pointsSparseDepth) > depthEpsilon
        pointsOcclusion[tensors['PointsBackground'] > 0.5] = 0.0
        pointsSparseOcclusion[tensors['PointsBackground'] > 0.5] = 0.0
        pointsOcclusion = pointsOcclusion.float()
        pointsSparseOcclusion.float()

        tensors['PointsOcclusion'] = pointsOcclusion
        tensors['PointsSparseOcclusion'] = pointsSparseOcclusion

        return tensors

    def GetTrainingSequence(self, trainingSequenceIndex):
        # TODO: Return 3 frames, with optical flow warped previous/next frame onto current frame
        return self.GetFrame(self.trainingFrames, trainingSequenceIndex)

    def GetTestSequence(self, testSequenceIndex):
        return self.GetFrame(self.testFrames, testSequenceIndex)