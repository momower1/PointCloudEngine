#ifndef HIERARCHY_H
#define HIERARCHY_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class Hierarchy
    {
    public:
        // Static functions to keep tack of all scene objects and use automatic memory management
        static SceneObject* Create(std::wstring name, Transform *parent = NULL, std::initializer_list<Component*> components = {});
        static void UpdateAllSceneObjects();
        static void DrawAllSceneObjects();
        static void ReleaseAllSceneObjects();

        // Stores the root level of the transform hierarchy tree, used to calculate the world matrices from top to bottom
        static std::vector<Transform*> rootTransforms;

        // Stores all the created scene objects, used to call functions on all of them
        static std::vector<SceneObject*> sceneObjects;

    private:
        // Calculate world matrices from top to bottom in the transform hierarchy tree
        static void CalculateWorldMatrices (std::vector<Transform*> const *children);
    };
}
#endif
