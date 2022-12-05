import numpy
import matplotlib
import matplotlib.pyplot as plt
from zipfile import ZipFile

# Use different matplotlib backend to avoid weird error
#matplotlib.use('Agg')

def LoadTextureFromBytes(bytes):
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
    depthNormalized = numpy.copy(depthTexture)

    # Replace background depth values with 0 for better visualization
    depthNormalized[maskBackground] = 0.0

    # Compute minimum and maximum masked depth values
    depthMin = depthTexture[maskForeground].min()
    depthMax = depthTexture[maskForeground].max()

    # Normalize values to [0, 1]
    depthNormalized[maskForeground] = (depthNormalized[maskForeground] - depthMin) / (depthMax - depthMin)

    return depthNormalized

archive = ZipFile('D:/Downloads/PointCloudEngineDataset/0.zip', 'r')

for filename in archive.namelist():
    print(filename)
    textureBytes = archive.read(filename)
    texture = LoadTextureFromBytes(textureBytes)

    #texture = numpy.transpose(texture, axes=(0, 1, 2))

    if filename.find('Depth') >= 0:
        foreground, background = GetForegroundBackgroundMasks(texture)
        texture = NormalizeDepthTexture(texture, foreground, background)

        # Visualize in grayscale with alpha channel for foreground/background mask
        texture = numpy.repeat(texture, 4, axis=2)
        texture[:, :, 3:4][foreground] = 1.0
        texture[:, :, 3:4][background] = 0.0

    plt.imshow(texture)
    plt.show()

exit(0)