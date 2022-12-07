from moviepy.editor import VideoClip, VideoFileClip
from Model import *
from Dataset import *

videoFilename = 'D:/Downloads/PointCloudEngineVideo.mp4'
checkpointFilename = 'G:/PointCloudEngineCheckpoints/Sparse Occlusion And Inpainting/Checkpoint.pt'
dataset = Dataset('G:/PointCloudEngineDatasetLarge/', 0)

fps = 30
duration = dataset.trainingSequenceCount / fps

# Load model
checkpoint = torch.load(checkpointFilename)
modelOcclusion = PullPushModel(5, 1, 16).to(device)
model = PullPushModel(8, 7, 16).to(device)
modelOcclusion.load_state_dict(checkpoint['ModelOcclusion'])
model.load_state_dict(checkpoint['Model'])
modelOcclusion = modelOcclusion.eval()
model = model.eval()

def CreateVideoFrame(t):
    sequenceIndex = int(t * fps)
    tensors = dataset.GetTrainingSequence(sequenceIndex)
    
    # Evaluate the models
    inputOcclusion = torch.cat([tensors['PointsSparseForeground'], tensors['PointsSparseDepth'], tensors['PointsSparseNormalScreen']], dim=0).unsqueeze(0)
    outputOcclusion = modelOcclusion(inputOcclusion)

    input = torch.cat([tensors['PointsSparseForeground'], tensors['PointsSparseDepth'], tensors['PointsSparseColor'], tensors['PointsSparseNormalScreen']], dim=0).unsqueeze(0)
    inputSurface = torch.clone(input)

    outputOcclusionMask = (outputOcclusion.detach() > 0.5).repeat(1, model.inChannels, 1, 1)
    inputSurface[outputOcclusionMask] = 0.0
    output = model(inputSurface)

    h = input.size(2)
    w = input.size(3)
    frame = torch.zeros((3, 2 * h, 3 * w), dtype=torch.float, device=device)

    frame[:, 0:h, 0:w] = input[0, 2:5, :, :]
    frame[:, 0:h, w:2*w] = input[0, 1:2, :, :].repeat(3, 1, 1)
    frame[:, 0:h, 2*w:3*w] = input[0, 5:8, :, :]

    frame[:, h:2*h, 0:w] = output[0, 1:4, :, :]
    frame[:, h:2*h, w:2*w] = output[0, 0:1, :, :].repeat(3, 1, 1)
    frame[:, h:2*h, 2*w:3*w] = output[0, 4:7, :, :]

    return TensorToImage(frame)

# Create a video
video = VideoClip(CreateVideoFrame, duration=duration)
video.write_videofile(videoFilename, fps=fps)