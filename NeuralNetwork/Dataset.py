import os
import torch
import numpy
from zipfile import ZipFile
from Utils import *

# Use different matplotlib backend to avoid weird error
#matplotlib.use('Agg')

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
    def __init__(self, directory):
        self.directory = directory
        self.frames = []

        filenames = os.listdir(self.directory)

        for filename in filenames:
            if filename.lower().endswith('.zip'):
                self.frames.append(self.directory + filename)

        self.frameCount = len(self.frames)
        self.sequenceCount = self.frameCount - 2

        print('Initialized dataset from "' + directory + '"')
        print('\t- ' + str(self.frameCount) + ' frames')
        print('\t- ' + str(self.sequenceCount) + ' sequences')

    def __len__(self):
        return self.frameCount - 2

    def __getitem__(self, sequenceIndex):
        # TODO: Return 3 frames, with optical flow warped previous/next frame onto current frame
        archive = ZipFile(self.frames[sequenceIndex], 'r')

        tensors = {}

        for filename in archive.namelist():
            textureBytes = archive.read(filename)
            texture = LoadTextureFromBytes(textureBytes)
            texture = torch.from_numpy(texture)
            texture = texture.to(device)
            texture = torch.permute(texture, dims=(2, 0, 1))

            # Normalize depth into [0, 1] range and add a foreground/background mask
            if filename.find('Depth') >= 0:
                viewMode = filename.split('Depth')[0]

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

            tensors[filename.split('.')[0]] = texture

        return tensors