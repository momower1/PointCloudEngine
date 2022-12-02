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

#texture = numpy.transpose(texture, axes=(0, 1, 2))

textureMin = texture[texture[:, :, 3] > 0].min()
textureMax = texture[texture[:, :, 3] > 0].max()

print(textureMin)
print(textureMax)

texture = (texture - textureMin) / (textureMax - textureMin)
texture = texture[:, :, 0:3]

print(rowCount)
print(texture.shape)
print(texture.min())
print(texture.max())

plt.imshow(texture)
plt.show()

exit(0)