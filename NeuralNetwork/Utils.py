import math
import torch

# Use cuda if possible
if torch.cuda.is_available():
    device = torch.device('cuda')
else:
    device = torch.device('cpu')

def TensorToImage(tensor):
    return (tensor * 255.0).clamp(0, 255).permute(1, 2, 0).type(torch.uint8).cpu().numpy()

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

def EstimateOcclusion(motionVectorTensor, distance=1.0):
    n, c, h, w = motionVectorTensor.shape
            
    # Create a tensor with the pixel positions
    pixelsVertical = torch.arange(0, h, dtype=torch.float, device=device)
    pixelsHorizontal = torch.arange(0, w, dtype=torch.float, device=device)
    pixelsHorizontal = pixelsHorizontal.unsqueeze(0).repeat(h, 1)
    pixelsVertical = pixelsVertical.unsqueeze(0).reshape(-1, 1).repeat(1, w)

    # Pixel positions are at the center of the pixel, not the top left corner
    pixels = torch.stack([pixelsHorizontal, pixelsVertical], dim=0).unsqueeze(0)
    pixels += 0.5

    # Apply the motion vector to the pixel positions
    pixelsWarped = torch.round(pixels + distance * motionVectorTensor).long()
    pixelsWarped[:, 0, :, :] = pixelsWarped[:, 0, :, :].clamp(0, w - 1)
    pixelsWarped[:, 1, :, :] = pixelsWarped[:, 1, :, :].clamp(0, h - 1)

    # Create occlusion mask (should be 1 if a pixel is being warped to the coordinate)
    occlusion = torch.zeros((n, 1, h, w),dtype=torch.float, device=device)

    for sampleIndex in range(n):
        occlusion[sampleIndex, :, pixelsWarped[sampleIndex, 1, :, :], pixelsWarped[sampleIndex, 0, :, :]] = 1.0

    # Resolve artifacts due to sampling
    occlusion = torch.nn.functional.max_pool2d(occlusion, 3, 1, 1)
    occlusion = -torch.nn.functional.max_pool2d(-occlusion, 3, 1, 1)

    return occlusion

# Tensor (0, 1), Brightness (0, 2), Contrast (0, 2), Hue (-0.5, 0.5), Saturation (0, 2)
def ColorShift(tensor, brightness, contrast, hue, saturation):
    tensor = torchvision.transforms.functional.adjust_brightness(tensor, brightness)
    tensor = torchvision.transforms.functional.adjust_contrast(tensor, contrast)
    tensor = torchvision.transforms.functional.adjust_hue(tensor, hue)
    tensor = torchvision.transforms.functional.adjust_saturation(tensor, saturation)

    return tensor