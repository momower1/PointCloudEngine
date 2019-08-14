## Overview
- Two seperate point cloud renderers with support for splatting and phong lighing
- [Ground Truth Renderer](https://github.com/momower1/PointCloudEngine/wiki/Ground-Truth-Renderer) directly renders a subset of the point cloud without preprocessing
- [Octree Renderer](https://github.com/momower1/PointCloudEngine/wiki/Octree-Renderer) builds an octree in a preprocessing step and renders the point cloud with LOD control
- [PlyToPointcloud](https://github.com/momower1/PointCloudEngine/wiki/PlyToPointcloud) tool for converting .ply files with _x,y,z,nx,ny,nz,red,green,blue_ format into the required .pointcloud format

## Getting Started
- Install DirectX11 and [HDF5](https://www.hdfgroup.org/downloads/hdf5)
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
