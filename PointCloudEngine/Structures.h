#ifndef STRUCTURES_H
#define STRUCTURES_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    struct Color16
    {
        // 6 bits red, 6 bits green, 4 bits blue
        unsigned short data;

        Color16()
        {
            data = 0;
        }

        Color16(byte red, byte green, byte blue)
        {
            byte r = round(63 * (red / 255.0f));
            byte g = round(63 * (green / 255.0f));
            byte b = round(15 * (blue / 255.0f));

            data = r << 10;
            data = data | g << 4;
            data = data | b;
        }
    };

    struct ClusterNormal
    {
		// Compact representation of a cluster normal with polar coordinates using inclination theta and azimuth phi
		// 6 bits theta, 6 bits phi and 4 bits for the cone of the normal (largest angle to one of the normals assigned to this cluster)
		// The empty normal is represented with theta=0 and phi=0
		// Theta is in [0, pi] therefore 0=0, 63=pi
		// Phi is in [-pi, pi] therefore 0=-pi, 63=pi
		// Cone is in [0, pi] therefore 0=0, 15=pi
		USHORT thetaPhiCone;

		ClusterNormal()
        {
			// Set to empty normal
			thetaPhiCone = 0;
        }

		// Cone is in radians from 0 to pi
		ClusterNormal(Vector3 clusterNormal, float clusterCone)
        {
			clusterNormal.Normalize();

            USHORT theta = 63.0f * (acos(clusterNormal.z) / XM_PI);
            USHORT phi = 31.5f + 31.5f * (atan2f(clusterNormal.y, clusterNormal.x) / XM_PI);
			USHORT cone = ceil(15.0f * min(1.0f, max(clusterCone / XM_PI, 0.0f)));

			// Avoid representing the empty normal (phi can be any value for theta == 0)
			if (theta == 0 && phi == 0)
			{
				phi = 63;
			}

			thetaPhiCone = theta << 10;
			thetaPhiCone |= phi << 4;
			thetaPhiCone |= cone;
        }

        Vector3 GetVector3()
        {
			USHORT theta = thetaPhiCone >> 10;
			USHORT phi = (thetaPhiCone & 0x3f0) >> 4;

			// Check if this represents the empty normal (0, 0, 0)
			if (theta == 0 && phi == 0)
			{
				return Vector3(0, 0, 0);
			}

            float t = XM_PI * (theta / 63.0f);
            float p = XM_PI * ((phi / 31.5f) - 1.0f);

            return Vector3(sin(t) * cos(p), sin(t) * sin(p), cos(t));
        }

		float GetCone()
		{
			return XM_PI * ((thetaPhiCone & 0xf) / 15.0f);
		}
    };

    struct Vertex
    {
        // Stores the .ply file vertices
        Vector3 position;
        Vector3 normal;
        byte color[3];
    };

	struct OctreeNodeProperties
	{
		// Mask where the lowest 8 bit store one bit for each of the children: 1 when it exists, 0 when it doesn't
		// The shader can only parse 32bit values -> combine to UINT (8 bit mask, 3 * 8 bit weights) with the weights
		byte childrenMask;

		// Each weight represents the percentage of points assigned to this cluster (0=0%, 255=100%)
		// One weight can be omitted because the sum of all weights is always 100%
		byte weights[3];

		// The different cluster mean normals and colors in object space calculated by k-means algorithm with k=4
		ClusterNormal normals[4];
		Color16 colors[4];
	};

    struct OctreeNodeVertex
    {
        // Bounding volume cube position
        Vector3 position;

		// The properties of the node
		OctreeNodeProperties properties;

        // Width of the whole cube
        float size;
    };

    // Stores all the data that is needed to create octree nodes
    struct OctreeNodeCreationEntry
    {
        UINT nodesIndex;
		UINT childrenIndex;
        std::vector<Vertex> vertices;
        Vector3 position;
        float size;
        int depth;
    };

	// Stores all the data that is needed to traverse the octree
	struct OctreeNodeTraversalEntry
	{
		UINT index;
		Vector3 position;
		float size;
		int parentInsideViewFrustum;	// Its used like a bool but stored as an int because the shader can only read 32bit values
		int depth;
	};

	// Same constant buffers as in hlsl file, keep packing rules in mind
	struct OctreeConstantBuffer
	{
		Matrix World;
		Matrix View;
		Matrix Projection;
		Matrix WorldInverseTranspose;
		Matrix WorldViewProjectionInverse;
		Vector3 cameraPosition;
		float padding0;
		Vector3 localCameraPosition;
		float padding1;
		Vector3 localViewFrustumNearTopLeft;
		float padding2;
		Vector3 localViewFrustumNearTopRight;
		float padding3;
		Vector3 localViewFrustumNearBottomLeft;
		float padding4;
		Vector3 localViewFrustumNearBottomRight;
		float padding5;
		Vector3 localViewFrustumFarTopLeft;
		float padding6;
		Vector3 localViewFrustumFarTopRight;
		float padding7;
		Vector3 localViewFrustumFarBottomLeft;
		float padding8;
		Vector3 localViewFrustumFarBottomRight;
		float padding9;
		Vector3 localViewPlaneNearNormal;
		float padding10;
		Vector3 localViewPlaneFarNormal;
		float padding11;
		Vector3 localViewPlaneLeftNormal;
		float padding12;
		Vector3 localViewPlaneRightNormal;
		float padding13;
		Vector3 localViewPlaneTopNormal;
		float padding14;
		Vector3 localViewPlaneBottomNormal;
		float fovAngleY;
		float samplingRate;
		float splatResolution;
		float overlapFactor;
		int level;

		// Blending
		int blend;					// Bool in the shader
		float depthEpsilon;

		// Compute shader data
		UINT inputCount;
		float padding15;
	};

	struct LightingConstantBuffer
	{
		int light;					// Bool in the shader
		Vector3 lightDirection;
		float lightIntensity;
		float ambient;
		float diffuse;
		float specular;
		float specularExponent;
		float padding[3];
	};
}

#endif