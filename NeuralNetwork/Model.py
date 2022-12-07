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

class PullBlock(torch.nn.Module):
    def __init__(self, inoutChannels=16):
        super(PullBlock, self).__init__()
        self.moduleList = torch.nn.ModuleList()
        self.moduleList.append(torch.nn.Conv2d(inoutChannels, inoutChannels, 3, 1, 1))
        self.moduleList.append(torch.nn.PReLU(1, 0.25))
        self.moduleList.append(torch.nn.Conv2d(inoutChannels, inoutChannels, 3, 1, 1))
        self.moduleList.append(torch.nn.PReLU(1, 0.25))
        self.moduleList.append(torch.nn.MaxPool2d(2))

        # Initialize parameters
        for module in self.modules():
            if isinstance(module, torch.nn.Conv2d):
                torch.nn.init.xavier_uniform_(module.weight, torch.nn.init.calculate_gain('leaky_relu', 0.25))

    def forward(self, input):
        act = input

        for module in self.moduleList:
            act = module(act)

        return act

class PushBlock(torch.nn.Module):
    def __init__(self, inoutChannels=16):
        super(PushBlock, self).__init__()
        self.moduleList = torch.nn.ModuleList()
        self.moduleList.append(torch.nn.Conv2d(inoutChannels, inoutChannels, 3, 1, 1))
        self.moduleList.append(torch.nn.PReLU(1, 0.25))
        self.moduleList.append(torch.nn.ConvTranspose2d(inoutChannels, inoutChannels, 4, 2, 1))
        self.moduleList.append(torch.nn.PReLU(1, 0.25))

    def forward(self, input):
        act = input

        for module in self.moduleList:
            act = module(act)

        return act

class FuseBlock(torch.nn.Module):
    def __init__(self, inChannels=32, outChannels=16):
        super(FuseBlock, self).__init__()
        self.moduleList = torch.nn.ModuleList()
        self.moduleList.append(torch.nn.Conv2d(inChannels, inChannels, 3, 1, 1))
        self.moduleList.append(torch.nn.PReLU(1, 0.25))
        self.moduleList.append(torch.nn.Conv2d(inChannels, outChannels, 3, 1, 1))
        self.moduleList.append(torch.nn.PReLU(1, 0.25))

    def forward(self, input):
        act = input

        for module in self.moduleList:
            act = module(act)

        return act

class PullPushModel(torch.nn.Module):
    def __init__(self, inChannels=3, outChannels=3, innerChannels=16):
        super(PullPushModel, self).__init__()
        self.inChannels = inChannels
        self.outChannels = outChannels
        self.convStart = torch.nn.Conv2d(inChannels, innerChannels, 3, 1, 1)
        self.preluStart = torch.nn.PReLU(1, 0.25)
        self.convEnd = torch.nn.Conv2d(innerChannels, outChannels, 3, 1, 1)
        self.sigmoid = torch.nn.Sigmoid()
        self.pullBlock = PullBlock(innerChannels)
        self.pushBlock = PushBlock(innerChannels)
        self.fuseBlock = FuseBlock(2 * innerChannels, innerChannels)

    def forward(self, input):
        n, c, h, w = input.shape

        # Calculate padding to a power of 2
        exponent = int(math.log2(max(h, w)))
        size = int(pow(2, exponent))
        widthPadding = (size - (w % size)) % size
        heightPadding = (size - (h % size)) % size

        # Pad so that the downscale and shuffle operation is valid
        input = torch.nn.functional.pad(input, pad=(0, widthPadding, 0, heightPadding), mode='constant', value=0)

        act = self.preluStart(self.convStart(input))

        residuals = [act]

        for i in range(exponent):
            act = self.pullBlock(act)
            residuals.append(act)

        residuals.pop()

        for i in range(exponent):
            residual = residuals.pop()
            act = self.pushBlock(act)
            act = torch.cat([act, residual], dim=1)
            act = self.fuseBlock(act)
            act = act + residual

        act = self.sigmoid(self.convEnd(act))

        # Slice away the padding
        act = act[:, :, 0:h, 0:w]

        return act