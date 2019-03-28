#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    struct Camera
    {
    public:
        Matrix view, projection;
        Vector3 position, right, up, forward;

        void CalculateRightUpForward (float pitch, float yaw)
        {
            // Smoothen the rotation
            rotation = 0.5f * rotation + 0.5f * Matrix::CreateFromYawPitchRoll(yaw, pitch, 0);
            right = Vector3::Transform(Vector3::UnitX, rotation);
            right.Normalize();
            forward = Vector3::Transform(Vector3::UnitZ, rotation);
            forward.Normalize();
            up = forward.Cross(right);
            up.Normalize();
        }

        void CalculateViewProjection (float fovAngleY, float aspectRatio, float nearZ, float farZ)
        {
            view = XMMatrixLookToLH(position, forward, up);
            projection = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);
        }

    private:
        Matrix rotation = Matrix::Identity;
    };

    struct Color8
    {
        byte red;
        byte green;
        byte blue;
        byte alpha;

        Color8()
        {
            red = 0;
            green = 0;
            blue = 0;
            alpha = 0;
        }

        Color8(byte red, byte green, byte blue, byte alpha)
        {
            this->red = red;
            this->green = green;
            this->blue = blue;
            this->alpha = alpha;
        }
    };

    struct PolarNormal
    {
        // Compact representation of a normal with polar coordinates using inclination theta and azimuth phi
        // [0, pi] therefore 0=0, 255=pi
        byte theta;
        // [-pi, pi] therefore 0=-pi, 255=pi
        byte phi;

        PolarNormal()
        {
            theta = 0;
            phi = 0;
        }

        PolarNormal(Vector3 normal)
        {
            normal.Normalize();

            theta = 255.0f * (acos(normal.z) / XM_PI);
            phi = 128.0f + 128.0f * (atan2f(normal.y, normal.x) / XM_PI);
        }

        Vector3 ToVector3()
        {
            float t = XM_PI * (theta / 255.0f);
            float p = XM_PI * ((phi / 128.0f) - 1.0f);

            return Vector3(sin(t) * cos(p), sin(t) * sin(p), cos(t));
        }
    };

    struct Vertex
    {
        // Stores the .ply file vertices
        Vector3 position;
        Vector3 normal;
        Color8 color;
    };

    struct OctreeNodeVertex
    {
        // Bounding volume cube position
        Vector3 position;

        // The different normals and colors in object space based on 6 view directions (all sides of a the bounding volume cube)
        PolarNormal normals[6];
        Color8 colors[6];

        // Width of the whole cube
        float size;
    };
}

#endif