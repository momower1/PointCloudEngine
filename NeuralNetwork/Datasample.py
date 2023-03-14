import numpy
import tkinter
from tkinter import filedialog
from matplotlib import pyplot as plt

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
        textures[renderMode] = texture

    return textures

# This script loads all textures from a .textures file into numpy arrays and displays them
tk = tkinter.Tk()
tk.withdraw()

texturesFilename = filedialog.askopenfile(title='Choose .textures file').name
texturesBytes = open(texturesFilename, 'rb').read()
textures = LoadTexturesFromBytes(texturesBytes)

for renderMode in textures:
    texture = textures[renderMode]
    print(renderMode + ': ' + str(texture.shape))

    plt.title(renderMode)
    plt.imshow(texture)
    plt.show()
