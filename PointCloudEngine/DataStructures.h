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

    struct PointCloudVertex
    {
        Vector3 position;
        Vector3 normal;
        byte red, green, blue, alpha;
    };
}

#endif