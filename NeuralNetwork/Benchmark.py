import time
from Model import *

model = PullPushModel(8, 7, 16).to(device).eval()
print(model.parameters)
totalParameters = sum(p.numel() for p in model.parameters())
print('Sum parameters: ' + str(totalParameters))

width = 1920
height = 1080
count = 100
input = torch.rand(1, 8, height, width, dtype=torch.float, device=device)

timeStart = time.time()

startEvent = torch.cuda.Event(enable_timing=True)
endEvent = torch.cuda.Event(enable_timing=True)
startEvent.record()

with torch.no_grad():
    for i in range(count):
        output = model(input)

endEvent.record()

torch.cuda.synchronize()

timeAverage = startEvent.elapsed_time(endEvent) / count

print('Average time per iteration:\t' + str(timeAverage))