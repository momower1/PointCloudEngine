import matplotlib
import matplotlib.pyplot as plt
from Dataset import *

dataset = Dataset('D:/Downloads/PointCloudEngineDataset/')

tensors = dataset[0]

for tensorName in tensors:
    tensor = tensors[tensorName]
    
    print(tensorName)
    print(tensor.shape)

    plt.imshow(TensorToImage(tensor))
    plt.show()

exit(0)