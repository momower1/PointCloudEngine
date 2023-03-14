![Comparison](Demo/Comparison.png?raw=true)

## Overview
- Two seperate point cloud renderers with support for splatting and phong lighting
- Ground Truth Renderer renders a point cloud with splatting, pull push algorithm or neural rendering pipeline and can compare results against a mesh
- Octree Renderer builds an octree in a preprocessing step and renders the point cloud with LOD control and splatting
- PlyToPointcloud tool converts .ply files with _x,y,z,nx,ny,nz,red,green,blue_ format into the required .pointcloud format

## Getting Started
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
- Pull-Push algorithm can be configured in a similar way
- Neural Rendering Pipeline requires loading all three exported networks

## Developer Setup (Windows)
- The following libraries are required, make sure that the CUDA toolkit version and Pytorch version are exactly the same
  - [Cuda 11.7](https://developer.nvidia.com/cuda-11-7-0-download-archive?target_os=Windows&target_arch=x86_64&target_version=11&target_type=exe_local)
  - [Anaconda3 2022.10](https://www.anaconda.com/products/distribution#Downloads) for all users and add it to PATH
  - Python environment in Visual Studio Installer (no need to install Python 3.7)
  - [Pytorch 1.13.1](https://pytorch.org/get-started/locally/) _conda install pytorch torchvision torchaudio pytorch-cuda=11.7 -c pytorch -c nvidia_
- Update include directories, library directories and post build event paths in Visual Studio PointCloudEngine property pages according to the installation paths

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
  - Pull Push: fast screen-space inpainting algorithm applied to point rendering
  - Mesh: render .OBJ textured mesh for comparison
  - Neural Network: renders neural network output from the sparse point rendering
- Compare to a sparse subset of the point cloud with a different sampling rate
- Blending the colors of overlapping splats
- Phong Lighting

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

# Training
- The neural rendering pipeline is trained in Pytorch
- Datasets for training can be created within the Engine under the Dataset tab

Copyright © Moritz Schöpf. All rights reserved. The use, distribution and change of this software and the source code without written permission of the author is prohibited.
