import numpy
import matplotlib
import matplotlib.pyplot as plt

# Use different matplotlib backend to avoid weird error
#matplotlib.use('Agg')

def NormalizeDepthTexture(texture):
    h, w, c = texture.shape

    # Create a mask that only selects elements from the non-alpha channels where the alpha channel is non-zero
    mask = texture[:, :, 3] > 0
    mask = numpy.reshape(mask, (h, w, 1))
    mask = numpy.repeat(mask, 4, axis=2)
    mask[:, :, 3] = False

    # Compute minimum and maximum depth values (mask only selects among the RGB elements)
    depthMin = texture[mask].min()
    depthMax = texture[mask].max()

    # Normalize RGB values to [0, 1] but keep the alpha channel the same
    textureNormalized = numpy.copy(texture)
    textureNormalized[mask] = (texture[mask] - depthMin) / (depthMax - depthMin)

    return textureNormalized

textureFile = open('D:/Downloads/PointCloudEngineDataset/MeshDepth.texture', 'rb')
textureBytes = textureFile.read()

width = numpy.frombuffer(buffer=textureBytes, dtype='int32', count=1, offset=0).item()
height = numpy.frombuffer(buffer=textureBytes, dtype='int32', count=1, offset=4).item()
channels = numpy.frombuffer(buffer=textureBytes, dtype='int32', count=1, offset=8).item()
elementSizeInBytes = numpy.frombuffer(buffer=textureBytes, dtype='int32', count=1, offset=12).item()
texture = numpy.frombuffer(buffer=textureBytes, dtype='float16', count=-1, offset=16)
texture = numpy.reshape(texture, (height, width, channels))
texture = texture.astype('float32')

#texture = numpy.transpose(texture, axes=(0, 1, 2))

texture = NormalizeDepthTexture(texture)

plt.imshow(texture)
plt.show()

exit(0)