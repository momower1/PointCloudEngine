## Overview
- Two seperate point cloud renderers with support for splatting and phong lighing
- [Ground Truth Renderer](https://github.com/momower1/PointCloudEngine/wiki/Ground-Truth-Renderer) renders a subset of the point cloud and can input it into a neural network
- [Octree Renderer](https://github.com/momower1/PointCloudEngine/wiki/Octree-Renderer) builds an octree in a preprocessing step and renders the point cloud with LOD control
- [PlyToPointcloud](https://github.com/momower1/PointCloudEngine/wiki/PlyToPointcloud) tool for converting .ply files with _x,y,z,nx,ny,nz,red,green,blue_ format into the required .pointcloud format

## Getting Started
- Follow install guide for latest Windows release from [Releases](https://github.com/momower1/PointCloudEngine/releases)
- Drag and drop your .ply files onto _PlyToPointcloud.exe_
- Adjust the _Settings.txt_ file (optional)
- Run _PointCloudEngine.exe_
- Open a generated .pointcloud file with File->Open
- Use the File menu to switch between the two renderers
- Move the camera with WASD, holding the right mouse button rotates the camera

## Configuring the rendering parameters
- Select the _Splat_ view mode
- Move the camera so close that the individual splats are visible (depending on the scale this might not happen)
- Adjust the _Sampling Rate_ in such a way that the splats overlap just a bit
- Enable _Blending_, look at the point cloud from various distances and adjust the blend factor with so that only close surfaces are blended together

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
