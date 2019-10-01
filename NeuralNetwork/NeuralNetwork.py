import random
import torch
import torchvision
import torchvision.transforms as transforms
import matplotlib.pyplot as plt
import numpy as np

# Function to show an image
def imshow(img):
    npimg = img.numpy()
    npimg = np.transpose(npimg, axes=(1, 2, 0))
    plt.imshow(npimg)
    plt.show()

testset = torchvision.datasets.ImageFolder(root='./data/Testset/', transform=torchvision.transforms.ToTensor())
testloader = torch.utils.data.DataLoader(testset, batch_size=3, shuffle=False, num_workers=0)

iterator = iter(testloader)

print('Showing ground truth')
groundTruth = iterator.next()
imshow(groundTruth[0][0])
imshow(groundTruth[0][1])
imshow(groundTruth[0][2])

print('Showing input')
input = iterator.next()
imshow(input[0][0])
imshow(input[0][1])
imshow(input[0][2])

# TESTING: Load neural network
# Input and Output: 3 Channel Color (RGB), 1 Channel Depth
model = torch.jit.load('./data/Pytorch_Jit_Model_Lucy.pt')
print('Loaded pytorch model')

# Evaluate model
#output = model(input)