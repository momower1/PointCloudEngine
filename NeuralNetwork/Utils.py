import torch

# Use cuda if possible
if torch.cuda.is_available():
    device = torch.device('cuda')
else:
    device = torch.device('cpu')

def TensorToImage(tensor):
    return (tensor * 255.0).clamp(0, 255).permute(1, 2, 0).type(torch.uint8).cpu().numpy()