import math
import torch
import torchvision

# Use cuda if possible
if torch.cuda.is_available():
    device = torch.device('cuda')
else:
    device = torch.device('cpu')

def TensorToImage(tensor):
    return (tensor * 255.0).clamp(0, 255).permute(1, 2, 0).type(torch.uint8).cpu().numpy()

def PrintIfTensorHasNaN(tensor, message):
    if torch.isnan(tensor).float().sum() > 0:
        print(message + ' is NaN!')

# Assume flow is a [0, 1] range tensor with shape (2, H, W)
def FlowToImage(flow):
    c, h, w = flow.shape
    flowImage = torch.zeros((3, h, w), dtype=flow.dtype, device=flow.device)
    flowImage[0:2, :, :] = flow[0:2, :, :]

    return TensorToImage(flowImage)

# Required for warping images
pixelGrid = None

def PixelGrid(width, height):
    pixelPositionsVertical = torch.arange(0, height, dtype=torch.float, device=device)
    pixelPositionsHorizontal = torch.arange(0, width, dtype=torch.float, device=device)
    pixelPositionsHorizontal = pixelPositionsHorizontal.unsqueeze(0).repeat(height, 1)
    pixelPositionsVertical = pixelPositionsVertical.unsqueeze(0).reshape(-1, 1).repeat(1, width)

    pixelGrid = torch.zeros((1, 2, height, width), dtype=torch.float, device=device)
    pixelGrid[:, 0, :, :] = pixelPositionsHorizontal
    pixelGrid[:, 1, :, :] = pixelPositionsVertical

    # Pixel positions are at the center of the pixel, not the top left corner
    pixelGrid += 0.5

    return pixelGrid

def WarpImage(imageTensor, motionVectorTensor, distance=1.0):
    global pixelGrid

    # Create a warp grid with the optical flow
    # First create a tensor that holds its image pixel coordinates (only if it doesn't exist yet or needs to change in size)
    if pixelGrid is None or pixelGrid.size(2) != imageTensor.size(2) or pixelGrid.size(3) != imageTensor.size(3):
        pixelGrid = PixelGrid(imageTensor.size(3), imageTensor.size(2))

    # Apply the motion vector
    warpGrid = pixelGrid + distance * motionVectorTensor

    # Normalize into (-1, 1) range for grid_sample
    warpGrid[:, 0, :, :] /= imageTensor.shape[3]
    warpGrid[:, 1, :, :] /= imageTensor.shape[2]
    warpGrid = (warpGrid * 2.0) - 1.0
    warpGrid = warpGrid.permute(0, 2, 3, 1)

    imageWarpedTensor = torch.nn.functional.grid_sample(imageTensor, warpGrid, mode='bilinear', padding_mode='zeros')

    return imageWarpedTensor

def GetValidMotionMask(motionVectorTensor, distance=1.0):
    n, c, h, w = motionVectorTensor.shape
    image = torch.ones((n, 1, h, w), dtype=torch.float, device=device)
    mask = WarpImage(image, motionVectorTensor, distance)
    return mask

def EndPointError(motionVectorTarget, motionVectorOutput):
    n, c, h, w = motionVectorTarget.shape
    pixels = pixel_grid(w, h).repeat(n, 1, 1, 1)
    pixelsWarpedTarget = pixels + motionVectorTarget
    pixelsWarpedOutput = pixels + motionVectorOutput
    pixelsWarpedDifference = pixelsWarpedOutput - pixelsWarpedTarget

    return torch.linalg.vector_norm(pixelsWarpedDifference, ord=2, dim=1).mean()

def EstimateOcclusion(motionVectorTensor, distance=1.0, artifactFilterSize=1):
    n, c, h, w = motionVectorTensor.shape
    
    # Apply the motion vector to the pixel positions
    pixels = PixelGrid(w, h).repeat(n, 1, 1, 1)
    pixelsWarped = WarpImage(pixels, motionVectorTensor, distance).long()
    pixelsWarped[:, 0, :, :] = pixelsWarped[:, 0, :, :].clamp(0, w - 1)
    pixelsWarped[:, 1, :, :] = pixelsWarped[:, 1, :, :].clamp(0, h - 1)

    # Create occlusion mask (should be 1 if a pixel is being warped to the coordinate)
    occlusion = torch.zeros((n, 1, h, w),dtype=torch.float, device=device)

    # Need to perform indexing per sample in the batch (otherwise too many values will be filled)
    for sampleIndex in range(n):
        occlusion[sampleIndex, :, pixelsWarped[sampleIndex, 1, :, :], pixelsWarped[sampleIndex, 0, :, :]] = 1.0

    # Resolve artifacts due to sampling
    if artifactFilterSize > 0:
        k = (2 * artifactFilterSize) - 1
        occlusion = torch.nn.functional.max_pool2d(occlusion, k, 1, k // 2)
        occlusion = -torch.nn.functional.max_pool2d(-occlusion, k, 1, k // 2)

    return occlusion

# Add this term to enforce the 1-Lipshitz constraint that is required for improved WGAN training
def GradientPenalty(critic, batchReal, batchFake, gamma=27.0):
    n, c, h, w = batchReal.size()
    alpha = torch.rand((n, c, h, w), dtype=torch.float, device=device)
    batchMixed = alpha * batchReal + (1.0 - alpha) * batchFake
    batchMixed = torch.autograd.Variable(batchMixed, requires_grad=True)

    criticMixed = critic(batchMixed)

    gradOutputs = torch.ones(n, dtype=torch.float, device=device)
    gradients = torch.autograd.grad(outputs=criticMixed, inputs=batchMixed, grad_outputs=gradOutputs, retain_graph=True, create_graph=True)
    gradients = gradients[0].view(n, -1)

    gradientNorm = torch.sqrt(torch.sum(torch.square(gradients), dim=1, keepdim=True) + 1e-12)
    gradientPenalty = gamma * torch.square(gradientNorm - 1.0)
    gradientPenalty = gradientPenalty.view(n)

    return gradientPenalty

# Tensor (0, 1), Brightness (0, 2), Contrast (0, 2), Hue (-0.5, 0.5), Saturation (0, 2)
def ColorShift(tensor, brightness, contrast, hue, saturation):
    tensor = torchvision.transforms.functional.adjust_brightness(tensor, brightness)
    tensor = torchvision.transforms.functional.adjust_contrast(tensor, contrast)
    tensor = torchvision.transforms.functional.adjust_hue(tensor, hue)
    tensor = torchvision.transforms.functional.adjust_saturation(tensor, saturation)

    return tensor

def ConvertTensorIntoZeroToOneRange(tensorFull):
    n, c, h, w = tensorFull.shape

    # Perform Min-Max normalization batchwise
    tensorMin, tensorMinIndices = torch.min(tensorFull.view(n, -1), dim=1)
    tensorZeroOne = tensorFull - tensorMin.view(n, 1, 1, 1)
    tensorMax, tensorMaxIndices = torch.max(tensorZeroOne.view(n, -1), dim=1)
    tensorZeroOne /= tensorMax.view(n, 1, 1, 1) + 1e-12

    return tensorMin, tensorMax, tensorZeroOne

def RevertTensorIntoFullRange(tensorMin, tensorMax, tensorZeroOne):
    n, c, h, w = tensorZeroOne.shape
    return tensorMin.view(n, 1, 1, 1) + (tensorMax.view(n, 1, 1, 1) * tensorZeroOne)