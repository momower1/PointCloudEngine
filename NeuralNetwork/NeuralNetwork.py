import random
import torch
import torchvision
import torchvision.transforms as transforms
import matplotlib.pyplot as plt
import numpy as np

# Function to show an image from a tensor
def ShowImage(image, title='Figure'):
    npimage = image.detach().numpy()

    # Transpose for the correct shape
    if npimage.shape[0] == 3:
        npimage = np.transpose(npimage, axes=(1, 2, 0))
    
    if npimage.shape[0] == 2:
        npimage = np.transpose(npimage)
    
    plt.imshow(npimage)
    plt.title(title)
    plt.show()

# Load rendered images from .png files in folders "/data/Testset/GroundTruth/" and "/data/Testset/Input/"
testset = torchvision.datasets.ImageFolder(root='./data/Testset/', transform=torchvision.transforms.ToTensor())
testloader = torch.utils.data.DataLoader(testset, batch_size=3, shuffle=False, num_workers=0)

# Show the images by using an iterator
iterator = iter(testloader)

splats = iterator.next()
#ShowImage(splats[0][0], 'Splats Color')
#ShowImage(splats[0][1], 'Splats Depth')
#ShowImage(splats[0][2], 'Splats Normals')

sparsePoints = iterator.next()
#ShowImage(sparsePoints[0][0], 'Sparse Points Color')
#ShowImage(sparsePoints[0][1], 'Sparse Points Depth')
#ShowImage(sparsePoints[0][2], 'Sparse Points Normals')

# Load and evaluate neural network
model = torch.jit.load('./data/Pytorch_Jit_Model_Lucy.pt')
model = model.cuda()
print('Loaded model')

# Evaluate model with input from files
# Input: 1 Channel Color (R, G or B), 1 Channel Depth
# Output: 1 Channel Color, 1 Channel Depth, 1 Channel Visibility Mask
input = torch.zeros(1, 2, 1024, 1024)

# First channel is color
input[0][0] = sparsePoints[0][0][0]

# Second channel is depth (average from RGB values)
input[0][1] = torch.mean(sparsePoints[0][1], 0)

# Show input images
ShowImage(input[0][0], 'Input Color R')
ShowImage(input[0][1], 'Input Depth')

# Evaluate model on gpu
input = input.cuda()
output = model(input)
print('Evaluated model')

# Show output image from the neural network
output = output.cpu()
ShowImage(output[0][0], 'Output Color R')
ShowImage(output[0][1], 'Output Depth')
ShowImage(output[0][2], 'Output Visibility Mask')