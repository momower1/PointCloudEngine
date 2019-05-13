![Screenshot](https://lh4.googleusercontent.com/jqYd12Qn7Tz3pUkvr52mYfEJj0sThHIGqy_Sa1n6Lng_J7GcqW2EUZrqeht6lbBU4a8t9qS-FskpX12QTdlh=w1365-h937-rw)

## Features
- PointCloudEngine loads and renders point cloud datasets and generates an octree for level-of-detail
- Supports .ply files with _x,y,z,nx,ny,nz,red,green,blue_ format only (you can use e.g. [MeshLab](http://www.meshlab.net/) to export to this format)
- Generated octree is saved as .octree file in the Octrees folder for faster loading
- View the octree nodes in three different modes
  - Splats: circular overlapping billboards with weighted cluster colors and normals that approximate the surface of the point cloud
  - Bounding Cubes: inspect size and position of the octree nodes, the color is the average color of all the points assigned to this node
  - Normal Clusters: inspect color and weight of the four normal clusters that were computed for each node with the k-means algorithm
- GPU octree traversal using a compute shader
- View Frustum Culling and Visibility Culling
- Blending the colors of overlapping splats
- Octree level selection
- Phong Lighting

## Getting Started
- Download the latest release from [Releases](https://github.com/momower1/PointCloudEngine/releases)
- Extract the .zip file
- Adjust the settings.txt file (optional)
- Run PointCloudEngine.exe
- Open a .ply file with [O]
- After that, press [H] for help

## Configuring the rendering parameters
- Go into Splat view mode with [ENTER]
- Move the camera so close that the individual splats are visible (depending on the scale this might not happen)
- Press [Q/E] to adjust the sample rate in such a way that the splats overlap just a bit
- Enable Blending with [B], look at the point cloud from various distances and adjust the depth epsilon with [V/N] so that only close surfaces are blended together

## Remarks
- Settings are not stored seperately for each loaded file
- Octree files larger than ~4GB are not supported by the engine, lower the maxOctreeDepth parameter in the settings file to generate a smaller file
- When changing the maxOctreeDepth parameter in the settings.txt file you have to delete the old .octree files in the Octrees folder in order to generate a new octree. Otherwise the engine will just load the old file with the old octree depth.
- Examples for .ply files are attached in [Releases](https://github.com/momower1/PointCloudEngine/releases)

## Example for supported .ply file
```
ply
format ascii 1.0
element vertex 18
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
1 0 0 -1 0 0 255 0 0
1 0 0 0 1 0 255 0 0
1 0 0 0 -1 0 255 0 0
1 0 0 0 0 1 255 0 0
1 0 0 0 0 -1 255 0 0
0 1 0 1 0 0 0 255 0
0 1 0 -1 0 0 0 255 0
0 1 0 0 1 0 0 255 0
0 1 0 0 -1 0 0 255 0
0 1 0 0 0 1 0 255 0
0 1 0 0 0 -1 0 255 0
0 0 1 1 0 0 0 0 255
0 0 1 -1 0 0 0 0 255
0 0 1 0 1 0 0 0 255
0 0 1 0 -1 0 0 0 255
0 0 1 0 0 1 0 0 255
0 0 1 0 0 -1 0 0 255
```
