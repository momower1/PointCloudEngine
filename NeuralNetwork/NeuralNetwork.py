import numpy
import matplotlib
import matplotlib.pyplot as plt

# Use different matplotlib backend to avoid weird error
#matplotlib.use('Agg')

textureFile = open('D:/Downloads/PointCloudEngineDataset/MeshDepth.texture', 'rb')
textureBytes = textureFile.read()

rowCount = numpy.frombuffer(buffer=textureBytes, dtype='int32', count=1, offset=0)
texture = numpy.frombuffer(buffer=textureBytes, dtype='float16', count=-1, offset=4)
texture = numpy.reshape(texture, (rowCount.item(), -1, 4))
texture = texture.astype('float32')
height = texture.shape[0]
width = texture.shape[1]

#texture = numpy.transpose(texture, axes=(0, 1, 2))

# Create a mask that only selects elements from the non-alpha channels where the alpha channel is non-zero
textureMask = texture[:, :, 3] > 0
textureMask = numpy.reshape(textureMask, (height, width, 1))
textureMask = numpy.repeat(textureMask, 4, axis=2)
textureMask[:, :, 3] = False

# Compute minimum and maximum depth values (mask only selects among the RGB elements)
textureMin = texture[textureMask].min()
textureMax = texture[textureMask].max()

# Normalize RGB values to [0, 1] but keep the alpha channel the same
texture[textureMask] = (texture[textureMask] - textureMin) / (textureMax - textureMin)

plt.imshow(texture)
plt.show()

exit(0)