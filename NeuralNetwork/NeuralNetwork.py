import random
import torch
import torchvision
import torchvision.transforms as transforms
import matplotlib.pyplot as plt
import numpy as np

# Function to show an image from a tensor
def ShowImage(image):
    npimage = image.detach().numpy()
    npimage = np.transpose(npimage, axes=(1, 2, 0))
    plt.imshow(npimage)
    plt.show()

# Load rendered images from .png files in folders "/data/Testset/GroundTruth/" and "/data/Testset/Input/"
testset = torchvision.datasets.ImageFolder(root='./data/Testset/', transform=torchvision.transforms.ToTensor())
testloader = torch.utils.data.DataLoader(testset, batch_size=3, shuffle=False, num_workers=0)

# Show the images by using an iterator
iterator = iter(testloader)

print('Showing splats')
splats = iterator.next()
ShowImage(splats[0][0])
#ShowImage(splats[0][1])
#ShowImage(splats[0][2])

print('Showing sparse points')
sparsePoints = iterator.next()
ShowImage(sparsePoints[0][0])
#ShowImage(sparsePoints[0][1])
#ShowImage(sparsePoints[0][2])

# Load and evaluate neural network
model = torch.jit.load('./data/Pytorch_Jit_Model_Lucy.pt')
model = model.cuda()
print('Loaded model')

# Evaluate model with input from files
# Input: 1 Channel Color (R, G or B), 1 Channel Depth
# Output: 1 Channel Color, 1 Channel Depth, 1 Channel Visibility Mask
input = sparsePoints[0].narrow(1, 0, 2)
input = input.cuda()
output = model(input)
print('Evaluated model')

# Show output image from the neural network
print('Showing output')
output = output.cpu()
ShowImage(output[0])