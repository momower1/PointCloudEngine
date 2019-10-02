import random
import torch
import torchvision
import torchvision.transforms as transforms
import matplotlib.pyplot as plt
import numpy as np

# Function to show an image
def imshow(img):
    npimg = img.detach().numpy()
    npimg = np.transpose(npimg, axes=(1, 2, 0))
    plt.imshow(npimg)
    plt.show()

testset = torchvision.datasets.ImageFolder(root='./data/Testset/', transform=torchvision.transforms.ToTensor())
testloader = torch.utils.data.DataLoader(testset, batch_size=3, shuffle=False, num_workers=0)

iterator = iter(testloader)

print('Showing splats')
splats = iterator.next()
imshow(splats[0][0])
imshow(splats[0][1])
imshow(splats[0][2])

print('Showing sparse points')
sparsePoints = iterator.next()
imshow(sparsePoints[0][0])
imshow(sparsePoints[0][1])
imshow(sparsePoints[0][2])

# TESTING: Load neural network
# Input and Output: 3 Channel Color (RGB), 1 Channel Depth
model = torch.jit.load('./data/Pytorch_Jit_Model_Lucy.pt')
model = model.cuda()
print('Loaded model')

# Evaluate model with random input
input = torch.randn(1, 2, 1024, 1024)
input = input.cuda()
output = model(input)
print('Evaluated model')

# Show output image
print('Showing output')
output = output.cpu()
imshow(output[0])