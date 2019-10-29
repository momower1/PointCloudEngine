## Overview
- Two seperate point cloud renderers with support for splatting and phong lighing
- [Ground Truth Renderer](https://github.com/momower1/PointCloudEngine/wiki/Ground-Truth-Renderer) renders a subset of the point cloud and can input it into a neural network
- [Octree Renderer](https://github.com/momower1/PointCloudEngine/wiki/Octree-Renderer) builds an octree in a preprocessing step and renders the point cloud with LOD control
- [PlyToPointcloud](https://github.com/momower1/PointCloudEngine/wiki/PlyToPointcloud) tool for converting .ply files with _x,y,z,nx,ny,nz,red,green,blue_ format into the required .pointcloud format

## Getting Started
- Download and extract the latest Windows release from [Releases](https://github.com/momower1/PointCloudEngine/releases)
- Drag and drop your .ply files onto PlyToPointcloud.exe
- Adjust the settings.txt file (optional)
- Run PointCloudEngine.exe
- Open a generated .pointcloud file with [O]
- Press [H] for help and [R] to switch between the two renderers

## Configuring the rendering parameters
- Go into Splat view mode with [ENTER]
- Move the camera so close that the individual splats are visible (depending on the scale this might not happen)
- Press [Q/E] to adjust the sample rate in such a way that the splats overlap just a bit
- Enable Blending with [B], look at the point cloud from various distances and adjust the blend factor with [V/N] so that only close surfaces are blended together

## Developer Setup
- Install the following on your Windows machine at the default install locations
  - [HDF5](https://www.hdfgroup.org/downloads/hdf5)
  - [Cuda 10.0](https://developer.nvidia.com/cuda-10.0-download-archive?target_os=Windows&target_arch=x86_64&target_version=10)
  - [Anaconda 3.7](https://repo.anaconda.com/archive/Anaconda3-2019.07-Windows-x86_64.exe) for all users and add it to PATH
  - Python environment in Visual Studio Installer (no need to install Python 3.7)
- Run admin command line:
  - _conda install pytorch torchvision cudatoolkit=10.0 -c pytorch_

## Example for supported .ply file
```
ply
format ascii 1.0
element vertex 3
property float x
property float y
property float z
property float nx
property float ny
property float nz
property uchar red
property uchar green
property uchar blue
end_header
1 0 0 1 0 0 255 0 0
0 1 0 0 1 0 0 255 0
0 0 1 0 0 1 0 0 255
```

Copyright © 2019 Moritz Schöpf. All rights reserved. The use, distribution and change of this software and the source code without written permission of the author is prohibited.
