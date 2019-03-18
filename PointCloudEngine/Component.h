#ifndef COMPONENT_H
#define COMPONENT_H

#pragma once
#include "PointCloudEngine.h"

// Abstract class for all components that can be added to SceneObjects
class Component
{
public:
    // Pass the scene object that this component is attached to
    // Especially shared components don't know which object they are attached to
    virtual void Initialize (SceneObject *sceneObject) = 0;
    virtual void Update (SceneObject *sceneObject) = 0;
    virtual void Draw (SceneObject *sceneObject) = 0;
    virtual void Release () = 0;
    virtual Component* Duplicate () = 0;

    // If this component is shared it should not be deleted if the game object is deleted
    bool shared = false;

    // Used to automatically initialize the component e.g. when it is created at runtime
    bool initialized = false;
};

#endif