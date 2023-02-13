from Model import *

checkpointFilename = 'G:/PointCloudEngineCheckpoints/DragonSparse128 8 Frames Batch 6 SRM 100 500 1000 500 0/Checkpoint.pt'
scriptedFilenameSCM = 'Models/SCM.pt'
scriptedFilenameSFM = 'Models/SFM.pt'
scriptedFilenameSRM = 'Models/SRM.pt'

SCM = UnetPullPush(5, 1, 16).to(device)
SFM = UnetPullPush(7, 2, 16).to(device)
SRM = UnetPullPush(16, 8, 16).to(device)

# Load model parameters from checkpoint
checkpoint = torch.load(checkpointFilename)
SCM.load_state_dict(checkpoint['SCM'])
SFM.load_state_dict(checkpoint['SFM'])
SRM.load_state_dict(checkpoint['SRM'])

UndoWeightNormalization(SCM)
UndoWeightNormalization(SFM)
UndoWeightNormalization(SRM)

# Script models in order to use them in C++
scriptedSCM = torch.jit.script(SCM)
scriptedSFM = torch.jit.script(SFM)
scriptedSRM = torch.jit.script(SRM)
scriptedSCM.save(scriptedFilenameSCM)
scriptedSFM.save(scriptedFilenameSFM)
scriptedSRM.save(scriptedFilenameSRM)