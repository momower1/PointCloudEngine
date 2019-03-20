#ifndef TRANSFROM_H
#define TRANSFORM_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Transform
    {
    public:
        void SetParent(Transform *parent);
        Transform* GetParent();
        std::vector<Transform*> const * GetChildren();
    
        SceneObject *sceneObject = NULL;
        Vector3 scale = Vector3::One;;
        Vector3 position = Vector3::Zero;
        Quaternion rotation = Quaternion::Identity;
        Matrix worldMatrix = Matrix::Identity;

    private:
        Transform *parent = NULL;
        std::vector<Transform*> children;
    };
}
#endif
