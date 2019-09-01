#ifndef CAMERA_H
#define CAMERA_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Camera
    {
    public:
        Camera();
        void PrepareDraw();
        void SetRotationMatrix(Matrix rotation);
        void Rotate(float x, float y, float z);
        void LookAt(Vector3 targetPosition);
        void SetPosition(Vector3 position);
        void Translate(Vector3 translation);
        void TranslateRUF(float deltaRight, float deltaUp, float deltaForward);

        Vector3 GetPosition();
        Vector3 GetRight();
        Vector3 GetUp();
        Vector3 GetForward();
        Matrix GetViewMatrix();
        Matrix GetProjectionMatrix();
        Matrix GetRotationMatrix();

    private:
        void RecalculateRightUpForwardViewProjection();

        D3D11_VIEWPORT viewport;
        Vector3 position, right, up, forward;
        Matrix view, projection, rotation;
        bool recalculate = true;
    };
}

#endif