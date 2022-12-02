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

textureMask = texture[:, :, 3] > 0
textureMask = numpy.reshape(textureMask, (height, width, 1))
textureMask = numpy.repeat(textureMask, 3, axis=2)

print(textureMask)
print(textureMask.shape)

texture = texture[:, :, 0:3]
textureMin = texture[textureMask].min()
textureMax = texture[textureMask].max()

print(textureMin)
print(textureMax)
print(textureMax - textureMin)

texture[textureMask] = (texture[textureMask] - textureMin) / (textureMax - textureMin)

print(rowCount)
print(texture.shape)
print(texture.min())
print(texture.max())

plt.imshow(texture)
plt.show()

exit(0)