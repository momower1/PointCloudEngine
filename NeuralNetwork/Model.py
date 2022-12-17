from Utils import *

def ApplyWeightNormalization(model):
    for module in model.modules():
        if type(module) is torch.nn.Conv2d or type(module) is torch.nn.PReLU:
            module = torch.nn.utils.weight_norm(module)

def InitializeParameters(model):
    for module in model.modules():
        if type(module) is torch.nn.Conv2d or type(module) is torch.nn.ConvTranspose2d:
            torch.nn.init.xavier_uniform_(module.weight, torch.nn.init.calculate_gain('leaky_relu', 0.25))

class Model(torch.nn.Module):
    def __init__(self, inChannels=3, outChannels=3, innerLayers=1):
        super(Model, self).__init__()
        self.inChannels = inChannels
        self.outChannels = outChannels
        self.moduleList = torch.nn.ModuleList()

        innerChannels = max(inChannels, outChannels)

        self.moduleList.append(torch.nn.Conv2d(inChannels, innerChannels, 3, 1, 1))
        self.moduleList.append(torch.nn.PReLU(1, 0.25))

        for innerLayerIndex in range(innerLayers):
            self.moduleList.append(torch.nn.Conv2d(innerChannels, innerChannels, 3, 1, 1))
            self.moduleList.append(torch.nn.PReLU(1, 0.25))

        self.moduleList.append(torch.nn.Conv2d(innerChannels, outChannels, 3, 1, 1))
        self.moduleList.append(torch.nn.Sigmoid())

        InitializeParameters(self)
        ApplyWeightNormalization(self)

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

class Critic(torch.nn.Module):
    def __init__(self, inChannels=3, outChannels=3, innerChannels=16):
        super(Critic, self).__init__()
        self.inChannels = inChannels
        self.outChannels = outChannels
        self.convStart = torch.nn.Conv2d(inChannels, innerChannels, 3, 1, 1)
        self.preluStart = torch.nn.PReLU(1, 0.25)
        self.convEnd = torch.nn.Conv2d(innerChannels, outChannels, 3, 1, 1)
        self.pullBlock = PullBlock(innerChannels)

        InitializeParameters(self)
        ApplyWeightNormalization(self)

    def forward(self, input):
        n, c, h, w = input.shape

        act = self.preluStart(self.convStart(input))

        resolutionCount = int(math.ceil(math.log2(max(h, w))))

        for i in range(resolutionCount):
            # Pad such that width and height are dividable by 2
            multiple = 2
            height = act.size(2)
            width = act.size(3)
            heightPadding = (multiple - (height % multiple)) % multiple
            widthPadding = (multiple - (width % multiple)) % multiple

            if heightPadding > 0 or widthPadding > 0:
                act = torch.nn.functional.pad(act, pad=(0, widthPadding, 0, heightPadding), mode='constant', value=0)

            act = self.pullBlock(act)

        act = self.convEnd(act)
        act = act.view(n)

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

        InitializeParameters(self)
        ApplyWeightNormalization(self)

    def forward(self, input):
        n, c, h, w = input.shape

        act = self.preluStart(self.convStart(input))

        paddings = []
        residuals = [act]
        resolutionCount = int(math.ceil(math.log2(max(h, w))))

        for i in range(resolutionCount):
            # Pad such that width and height are dividable by 2
            multiple = 2
            height = act.size(2)
            width = act.size(3)
            heightPadding = (multiple - (height % multiple)) % multiple
            widthPadding = (multiple - (width % multiple)) % multiple
            paddings.append([heightPadding, widthPadding])

            if heightPadding > 0 or widthPadding > 0:
                act = torch.nn.functional.pad(act, pad=(0, widthPadding, 0, heightPadding), mode='constant', value=0)

            act = self.pullBlock(act)
            residuals.append(act)

        residuals.pop()

        for i in range(resolutionCount):
            act = self.pushBlock(act)

            # Slice away the padding
            height = act.size(2)
            width = act.size(3)
            heightPadding, widthPadding = paddings.pop()

            if heightPadding > 0 or widthPadding > 0:
                act = act[:, :, 0:height-heightPadding, 0:width-widthPadding]

            # Fuse and add residual from pull phase
            residual = residuals.pop()
            act = torch.cat([act, residual], dim=1)
            act = self.fuseBlock(act)
            act = act + residual

        act = self.sigmoid(self.convEnd(act))

        return act

class CriticDeep(torch.nn.Module):
    def __init__(self, frameSize=128, inChannels=48):
        super(CriticDeep, self).__init__()
        self.moduleList = torch.nn.ModuleList()
        self.layerCount = int(math.log2(frameSize)) - 2

        # Create a critic with variable layer count depending on the input frame size (must be a power of 2)
        for layerIndex in range(self.layerCount):
            self.moduleList.append(torch.nn.Conv2d(pow(2, layerIndex) * inChannels, pow(2, layerIndex + 1) * inChannels, 4, 2, 1))
            self.moduleList.append(torch.nn.PReLU(1, 0.25))

        self.moduleList.append(torch.nn.Conv2d(pow(2, self.layerCount) * inChannels, 1, int(frameSize / pow(2, self.layerCount)), 1))

        InitializeParameters(self)
        ApplyWeightNormalization(self)

    def forward(self, input):
        n, c, h, w = input.shape
        act = input

        for moduleIndex in range(len(self.moduleList)):
            act = self.moduleList[moduleIndex](act)

        act = act.view(n)

        return act