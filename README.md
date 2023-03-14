## Overview
- Two seperate point cloud renderers with support for splatting and phong lighting
- [Ground Truth Renderer](https://github.com/momower1/PointCloudEngine/wiki/Ground-Truth-Renderer) renders a subset of the point cloud and can input it into a neural network
- [Octree Renderer](https://github.com/momower1/PointCloudEngine/wiki/Octree-Renderer) builds an octree in a preprocessing step and renders the point cloud with LOD control
- [PlyToPointcloud](https://github.com/momower1/PointCloudEngine/wiki/PlyToPointcloud) tool for converting .ply files with _x,y,z,nx,ny,nz,red,green,blue_ format into the required .pointcloud format

## Getting Started
- Follow install guide for latest Windows 10 64-Bit release from [Releases](https://github.com/momower1/PointCloudEngine/releases)
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
  - Visual Studio 2019 Extension _Microsoft Visual Studio Installer Projects_
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

# Ground Truth Renderer
## Features
- Loads and renders different subsets of point clouds
- View the point cloud in different modes
  - Splats: high quality splat rendering of the whole point cloud
  - Points: high quality point rendering of the whole point cloud
  - Neural Network: renders neural network output and loss channel comparison
- Compare to a sparse subset of the point cloud with a different sampling rate
- HDF5 dataset generation containing color, normal and depth images of the point cloud
- Blending the colors of overlapping splats
- Phong Lighting

## Neural Network View Mode:
- Neural network should output the splat rendering from a sparse point rendering input
- The neural network must be loaded from a selected .pt file with _Load Model_
- A description of the input/output channels of the network must be loaded from a .txt file with _Load Description_
- Each entry consists of:
    - String: Name of the channel (render mode)
    - Int: Dimension of channel
    - String: Identifying if the channel is input (inp) or output (tar)
    - String: Transformation keywords e.g. normalization
    - Int: Offset of this channel from the start channel
- Example for a simple description file:
```
[['PointsSparseColor', 3, 'inp', 'normalize', 0], ['SplatsColor', 3, 'tar', 'normalize', 0]]
```
- When a _Loss Function_ is selected
    - Loss between two channels (_Loss Self_ and _Loss Target_) is computed
    - Screen area between these channel renderings can be controlled _Loss Area_

## HDF5 Dataset Generation
- Rendering resolution must currently be a power of 2
- There are two modes for the dataset generation with parameters in the _HDF5 Dataset_ tab
  - Waypoint dataset:
    - Interpolates the camera between the user set waypoints
    - Use the _Advanced_ tab to add/remove a waypoint for the current camera perspective
    - _Preview Waypoints_ shows a preview of the interpolation
    - _Step Size_ controls the interpolation value between two waypoints
  - Sphere dataset:
    - Sweeps the camera along a sphere around the point cloud
    - _Step Size_ influences the amount of viewing angles (0.2 results in ~1000 angles)
    - Theta and Phi Min/Max values define the subset of the sphere to be sweeped
    - Move the camera to the desired distance from the center of the point cloud before generation
- Start the generation process with _Generate Waypoint HDF5 Dataset_ or _Generate Sphere HDF5 Dataset_
    - Make sure that the density and rendering parameters for the _Sparse Splats_ view mode are set according to [Configure the rendering parameters](https://github.com/momower1/PointCloudEngine#configuring-the-rendering-parameters) 
    - The generated file will be stored in the HDF5 directory
    - After generation all the view points can be selected with the _Camera Recording_ slider

# Octree Renderer
## Features
- Loads and renders point cloud datasets and generates an octree for level-of-detail
- Generated octree is saved as .octree file in the Octrees folder for faster loading
- View the octree nodes in three different modes
  - Splats: circular overlapping billboards with weighted cluster colors and normals that approximate the surface of the point cloud
  - Bounding Cubes: inspect size and position of the octree nodes, the color is the average color of all the points assigned to this node
  - Normal Clusters: inspect color and weight of the four normal clusters that were computed for each node with the k-means algorithm
- GPU octree traversal using a compute shader
- View Frustum Culling and Backface Culling
- Blending the colors of overlapping splats
- Octree level selection
- Phong Lighting

## Remarks
- Octree files larger than ~4GB are not supported by the engine, lower the maxOctreeDepth parameter in the _Settings.txt_ file to generate a smaller file
- When changing the maxOctreeDepth parameter in the _Settings.txt_ file you have to delete the old .octree files in the Octrees folder in order to generate a new octree. Otherwise the engine will just load the old file with the old octree depth.

# PlyToPointcloud
## Features
- Converts between .ply and .pointcloud file format
- Supports .ply files with _x,y,z,nx,ny,nz,red,green,blue_ format only (you can use e.g. [MeshLab](http://www.meshlab.net/) to export to this format)
- Drag and drop .ply files to generate the corresponding .pointcloud files
- Drag and drop .pointcloud files to generate the original .ply file

## Pointcloud file format
- A .pointcloud file stores the following binary data
  - Vector3 - position of the bounding cube
  - float - size of the bounding cube
  - uint - length of the vertex array
  - vector - list of vertices
- Each vertex consists of
  - Vector3 - position
  - char[3] - normalized normal
  - uchar[3] - rgb color

Copyright © Moritz Schöpf. All rights reserved. The use, distribution and change of this software and the source code without written permission of the author is prohibited.
