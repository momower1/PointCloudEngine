from Utils import *

def ApplyWeightNormalization(model):
    for module in model.modules():
        if type(module) is torch.nn.Conv2d or type(module) is torch.nn.PReLU:
            module = torch.nn.utils.weight_norm(module)

def InitializeParameters(model):
    for module in model.modules():
        if type(module) is torch.nn.Conv2d or type(module) is torch.nn.ConvTranspose2d:
            torch.nn.init.xavier_uniform_(module.weight, torch.nn.init.calculate_gain('leaky_relu', 0.25))
            #torch.nn.init.xavier_normal_(module.weight, 0.1)

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

class PullPushLayer(torch.nn.Module):
    def __init__(self, inoutChannels=16):
        super(PullPushLayer, self).__init__()
        self.inoutChannels = inoutChannels
        self.pullBlock = PullBlock(inoutChannels)
        self.pushBlock = PushBlock(inoutChannels)
        self.fuseBlock = FuseBlock(2 * inoutChannels, inoutChannels)

    def forward(self, input):
        n, c, h, w = input.shape

        paddings = []
        residuals = [input]
        resolutionCount = int(math.ceil(math.log2(max(h, w))))

        act = input

        # Pull phase
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

        # Push phase
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

        return act

class PullPushModel(torch.nn.Module):
    def __init__(self, inChannels=3, outChannels=3, innerChannels=16, recurrent=False):
        super(PullPushModel, self).__init__()
        self.inChannels = inChannels
        self.outChannels = outChannels
        self.recurrent = recurrent
        self.previous = None
        self.convStart = torch.nn.Conv2d(2 * inChannels if self.recurrent else inChannels, innerChannels, 3, 1, 1)
        self.preluStart = torch.nn.PReLU(1, 0.25)
        self.pullPush = PullPushLayer(innerChannels)
        self.convEnd = torch.nn.Conv2d(innerChannels, outChannels, 3, 1, 1)
        self.sigmoid = torch.nn.Sigmoid()

        InitializeParameters(self)
        ApplyWeightNormalization(self)

    def forward(self, input, motionVector):
        n, c, h, w = input.shape

        if self.recurrent:
            # Need to use a zero tensor for the previous output (if there was no frame before this one in a sequence)
            if self.previous is None:
                self.previous = torch.zeros_like(input)

            # Warp the previous output onto the current frame using the motion vector
            self.previous = WarpImage(self.previous, motionVector)

            # Concatenate input and previous output for a recurrent architecture
            act = torch.cat([input, self.previous], dim=1)
        else:
            act = input

        act = self.preluStart(self.convStart(act))
        act = self.pullPush(act)
        act = self.sigmoid(self.convEnd(act))

        if self.recurrent:
            # Store the current output for next iteration
            self.previous = act

        return act

    def DetachPrevious(self):
        if self.previous is not None:
            self.previous = self.previous.detach()

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

class Unet(torch.nn.Module):
    def __init__(self, inChannels=8, outChannels=8, innerChannels=8, layerCount=4):
        super(Unet, self).__init__()
        self.layerCount = layerCount
        self.encoderConvs = torch.nn.ModuleList()
        self.encoderPrelus = torch.nn.ModuleList()
        self.decoderConvs = torch.nn.ModuleList()
        self.decoderPrelus = torch.nn.ModuleList()
        self.convStart = torch.nn.Conv2d(inChannels, innerChannels, 3, 1, 1)
        self.preluStart = torch.nn.PReLU(1, 0.25)
        self.convEnd = torch.nn.Conv2d(innerChannels, outChannels, 3, 1, 1)
        self.sigmoid = torch.nn.Sigmoid()

        for layerIndex in range(self.layerCount):
            self.encoderConvs.append(torch.nn.Conv2d(pow(2, layerIndex) * innerChannels, pow(2, layerIndex + 1) * innerChannels, 4, 2, 1))
            self.encoderPrelus.append(torch.nn.PReLU(1, 0.25))

        for layerIndex in range(self.layerCount - 1, -1, -1):
            self.decoderConvs.append(torch.nn.ConvTranspose2d(pow(2, layerIndex + 1) * innerChannels, pow(2, layerIndex) * innerChannels, 4, 2, 1))
            self.decoderPrelus.append(torch.nn.PReLU(1, 0.25))

        InitializeParameters(self)
        ApplyWeightNormalization(self)

    def forward(self, input):
        n, c, h, w = input.shape

        # Calculate padding
        multiple = pow(2, self.layerCount)
        widthPadding = (multiple - (w % multiple)) % multiple
        heightPadding = (multiple - (h % multiple)) % multiple

        # Pad so that the strided convolution operations are valid
        input = torch.nn.functional.pad(input, pad=(0, widthPadding, 0, heightPadding), mode='constant', value=0)

        # Perform start convolution
        act = self.preluStart(self.convStart(input))

        # Store encoder embeddings for residual connection during decoder phase
        residuals = [act]

        # Encoder layers
        for layerIndex in range(self.layerCount):
            act = self.encoderPrelus[layerIndex](self.encoderConvs[layerIndex](act))
            residuals.append(act)

        residuals.pop()

        # Decoder layers
        for layerIndex in range(self.layerCount):
            act = self.decoderPrelus[layerIndex](self.decoderConvs[layerIndex](act))

            # Residual connection
            act = act + residuals.pop()

        # Perform end convolution
        act = self.sigmoid(self.convEnd(act))

        return act

class UnetPullPushEncoderBlock(torch.nn.Module):
    def __init__(self, inChannels=8, outChannels=16):
        super(UnetPullPushEncoderBlock, self).__init__()
        self.conv = torch.nn.Conv2d(inChannels, outChannels, 4, 2, 1)
        self.prelu = torch.nn.PReLU(1, 0.25)
        self.pullPush = PullPushLayer(outChannels)

    def forward(self, input):
        return self.pullPush(self.prelu(self.conv(input)))

class UnetPullPushDecoderBlock(torch.nn.Module):
    def __init__(self, inChannels=16, outChannels=8):
        super(UnetPullPushDecoderBlock, self).__init__()
        self.conv = torch.nn.ConvTranspose2d(inChannels, outChannels, 4, 2, 1)
        self.prelu = torch.nn.PReLU(1, 0.25)

    def forward(self, input):
        return self.prelu(self.conv(input))

class UnetPullPush(torch.nn.Module):
    def __init__(self, inChannels=8, outChannels=8, innerChannels=8, layerCount=4):
        super(UnetPullPush, self).__init__()
        self.layerCount = layerCount
        self.encoderBlocks = torch.nn.ModuleList()
        self.decoderBlocks = torch.nn.ModuleList()
        self.convStart = torch.nn.Conv2d(inChannels, innerChannels, 3, 1, 1)
        self.preluStart = torch.nn.PReLU(1, 0.25)
        self.convEnd = torch.nn.Conv2d(innerChannels, outChannels, 3, 1, 1)
        self.sigmoid = torch.nn.Sigmoid()

        for layerIndex in range(self.layerCount):
            self.encoderBlocks.append(UnetPullPushEncoderBlock(pow(2, layerIndex) * innerChannels, pow(2, layerIndex + 1) * innerChannels))

        for layerIndex in range(self.layerCount - 1, -1, -1):
            self.decoderBlocks.append(UnetPullPushDecoderBlock(pow(2, layerIndex + 1) * innerChannels, pow(2, layerIndex) * innerChannels))

        InitializeParameters(self)
        ApplyWeightNormalization(self)

    def forward(self, input):
        n, c, h, w = input.shape

        # Calculate padding
        multiple = pow(2, self.layerCount)
        widthPadding = (multiple - (w % multiple)) % multiple
        heightPadding = (multiple - (h % multiple)) % multiple

        # Pad so that the strided convolution operations are valid
        input = torch.nn.functional.pad(input, pad=(0, widthPadding, 0, heightPadding), mode='constant', value=0)

        # Perform start convolution
        act = self.preluStart(self.convStart(input))

        # Store encoder embeddings for residual connection during decoder phase
        residuals = [act]

        # Encoder layers
        for layerIndex in range(self.layerCount):
            act = self.encoderBlocks[layerIndex](act)
            residuals.append(act)

        residuals.pop()

        # Decoder layers
        for layerIndex in range(self.layerCount):
            act = self.decoderBlocks[layerIndex](act)

            # Residual connection
            act = act + residuals.pop()

        # Perform end convolution
        act = self.sigmoid(self.convEnd(act))

        return act