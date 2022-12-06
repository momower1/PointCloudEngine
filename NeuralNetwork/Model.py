from Utils import *

class Model(torch.nn.Module):
    def __init__(self, inChannels=3, outChannels=3, innerLayers=1):
        super(Model, self).__init__()
        self.moduleList = torch.nn.ModuleList()

        innerChannels = max(inChannels, outChannels)

        self.moduleList.append(torch.nn.Conv2d(inChannels, innerChannels, 3, 1, 1))
        self.moduleList.append(torch.nn.PReLU(1, 0.25))

        for innerLayerIndex in range(innerLayers):
            self.moduleList.append(torch.nn.Conv2d(innerChannels, innerChannels, 3, 1, 1))
            self.moduleList.append(torch.nn.PReLU(1, 0.25))

        self.moduleList.append(torch.nn.Conv2d(innerChannels, outChannels, 3, 1, 1))
        self.moduleList.append(torch.nn.Sigmoid())

        # Initialize parameters
        for module in self.modules():
            if isinstance(module, torch.nn.Conv2d):
                torch.nn.init.xavier_uniform_(module.weight, torch.nn.init.calculate_gain('leaky_relu', 0.25))

    def forward(self, input):
        act = input

        for module in self.moduleList:
            act = module(act)

        return act