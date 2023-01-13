from Model import *

checkpointFilename = 'G:/PointCloudEngineCheckpoints/Supervised/Checkpoint.pt'
tracedFilenameSCM = 'Models/SCM.pt'
tracedFilenameSFM = 'Models/SFM.pt'
tracedFilenameSRM = 'Models/SRM.pt'

SCM = PullPushModel(5, 1, 16).to(device)
SFM = PullPushModel(7, 2, 16).to(device)
SRM = PullPushModel(16, 8, 16).to(device)

# Load model parameters from checkpoint
checkpoint = torch.load(checkpointFilename)
SCM.load_state_dict(checkpoint['SCM'])
SFM.load_state_dict(checkpoint['SFM'])
SRM.load_state_dict(checkpoint['SRM'])

# Trace models
tracedSCM = torch.jit.trace(SCM, torch.rand((1, 5, 1920, 1080), dtype=torch.float, device=device))
tracedSFM = torch.jit.trace(SFM, torch.rand((1, 7, 1920, 1080), dtype=torch.float, device=device))
tracedSRM = torch.jit.trace(SRM, torch.rand((1, 16, 1920, 1080), dtype=torch.float, device=device))
torch.jit.save(tracedSCM, tracedFilenameSCM)
torch.jit.save(tracedSFM, tracedFilenameSFM)
torch.jit.save(tracedSRM, tracedFilenameSRM)