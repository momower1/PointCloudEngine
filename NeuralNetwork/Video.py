from moviepy.editor import VideoClip, VideoFileClip
from Model import *
from Dataset import *

videoFilename = 'D:/Downloads/PointCloudEngineVideo'
checkpointFilename = 'D:/Google Drive/Informatik/Masterarbeit/Models/Walkman256/Checkpoint.pt'
dataset = Dataset('G:/PointCloudEngineDatasets/VideoWalkman256/', 3, False, False, False)

fps = 30.0
h = dataset.height
w = dataset.width
duration = dataset.trainingSequenceCount / fps

# Load model
checkpoint = torch.load(checkpointFilename)
VCN = UnetPullPush(5, 1, 16).to(device)
FIN = UnetPullPush(7, 2, 16).to(device)
SRN = UnetPullPush(16, 8, 16).to(device)
VCN.load_state_dict(checkpoint['SCM'])
FIN.load_state_dict(checkpoint['SFM'])
SRN.load_state_dict(checkpoint['SRM'])
VCN = VCN.eval()
FIN = FIN.eval()
SRN = SRN.eval()

previousOutputSRN = None

def CreateVideoFrame(t):
    global previousOutputSRN

    frameIndex = int(t * fps)
    tensors = dataset.GetFrame(dataset.trainingFrames, frameIndex)

    for tensorName in tensors:
        tensors[tensorName] = tensors[tensorName].unsqueeze(0)

    with torch.no_grad():
        # Evaluate the visibility classifiaction network
        inputVCN = torch.cat([tensors['PointsSparseForeground'], tensors['PointsSparseDepth'], tensors['PointsSparseNormalScreen']], dim=1)
        outputVCN = VCN(inputVCN)

        # Evaluate the flow inpainting network
        pointsVisibleMask = outputVCN.ge(0.5).float()
        pointsVisibleDepth = pointsVisibleMask * tensors['PointsSparseDepth']
        pointsVisibleNormal = pointsVisibleMask * tensors['PointsSparseNormalScreen']
        pointsVisibleFlowForward = pointsVisibleMask * tensors['PointsSparseOpticalFlowForward']
        depthMin, depthMax, pointsVisibleDepth = ConvertTensorIntoZeroToOneRange(pointsVisibleDepth)
        flowMin, flowMax, pointsVisibleFlowForward = ConvertTensorIntoZeroToOneRange(pointsVisibleFlowForward)
        inputFIN = torch.cat([pointsVisibleMask, pointsVisibleDepth, pointsVisibleNormal, pointsVisibleFlowForward], dim=1)
        outputFIN = FIN(inputFIN)
        flowForward = RevertTensorIntoFullRange(flowMin, flowMax, outputFIN)

        # Evaluate the surface reconstruction network
        pointsVisibleColor = pointsVisibleMask * tensors['PointsSparseColor']
        inputSRN = torch.cat([pointsVisibleMask, pointsVisibleDepth, pointsVisibleColor, pointsVisibleNormal], dim=1)

        if previousOutputSRN is None:
            previousOutputSRN = torch.zeros_like(inputSRN)

        previousOutputSRN = WarpImage(previousOutputSRN, flowForward)
        previousOutputSRN *= EstimateOcclusion(flowForward)
        inputSRN = torch.cat([inputSRN, previousOutputSRN], dim=1)
        outputSRN = SRN(inputSRN)

        # Create video frame
        frame = torch.zeros((1, 3, 3 * h, 5 * w), dtype=torch.float, device=device)

        frame[:, :, 0:h, 0:w] = tensors['PointsSparseDepth'].repeat(1, 3, 1, 1)
        frame[:, :, 0:h, w:2*w] = outputSRN[:, 1:2, :, :].repeat(1, 3, 1, 1)
        frame[:, :, 0:h, 2*w:3*w] = tensors['SplatsSparseDepth'].repeat(1, 3, 1, 1)
        frame[:, :, 0:h, 3*w:4*w] = tensors['PullPushDepth'].repeat(1, 3, 1, 1)
        frame[:, :, 0:h, 4*w:5*w] = tensors['MeshDepth'].repeat(1, 3, 1, 1)

        frame[:, :, h:2*h, 0:w] = tensors['PointsSparseColor']
        frame[:, :, h:2*h, w:2*w] = outputSRN[:, 2:5, :, :]
        frame[:, :, h:2*h, 2*w:3*w] = tensors['SplatsSparseColor']
        frame[:, :, h:2*h, 3*w:4*w] = tensors['PullPushColor']
        frame[:, :, h:2*h, 4*w:5*w] = tensors['MeshColor']

        frame[:, :, 2*h:3*h, 0:w] = tensors['PointsSparseNormalScreen']
        frame[:, :, 2*h:3*h, w:2*w] = outputSRN[:, 5:8, :, :]
        frame[:, :, 2*h:3*h, 2*w:3*w] = tensors['SplatsSparseNormalScreen']
        frame[:, :, 2*h:3*h, 3*w:4*w] = tensors['PullPushNormalScreen']
        frame[:, :, 2*h:3*h, 4*w:5*w] = tensors['MeshNormalScreen']

        frame = frame.squeeze(0)

    return TensorToImage(frame)

# Create a video
video = VideoClip(CreateVideoFrame, duration=duration)
video.write_videofile(videoFilename + '.avi', fps=fps, codec='png')
video.write_videofile(videoFilename + '.mp4', fps=fps)