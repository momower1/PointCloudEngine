#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#pragma once
#include "PointCloudEngine.h"

namespace PointCloudEngine
{
    class SceneObject
    {
    public:
        SceneObject(std::wstring name, Transform *parent, std::initializer_list<Component*> components = {});
        ~SceneObject();
        SceneObject* FindChildByName(std::wstring childName);
        void RemoveComponent(Component *componentToRemove);
        void Update();
        void Draw();
        void Release();

        std::wstring name = L"SceneObject";
        Transform *transform;

        // Template functions have to be defined in the header file
        template<typename T> T* AddComponent(T *t)
        {
            Component* component = dynamic_cast<Component*>(t);

            if (component != NULL)
            {
                components.push_back(component);
                return t;
            }

            return NULL;
        }

        template<typename T> T* GetComponent()
        {
            for (auto it = components.begin(); it != components.end(); it++)
            {
                T* t = dynamic_cast<T*>(*it);

                if (t != NULL)
                {
                    return t;
                }
            }

            return NULL;
        }

        template<typename T> std::vector<T*> GetComponents()
        {
            std::vector<T*> result;

            for (auto it = components.begin(); it != components.end(); it++)
            {
                T* t = dynamic_cast<T*>(*it);

                if (t != NULL)
                {
                    result.push_back(t);
                }
            }

            return result;
        }

        template<typename T> T* GetComponentInHierarchyUp()
        {
            T* t = GetComponent<T>();

            if (t != NULL)
            {
                return t;
            }
            else
            {
                Transform *parent = transform->GetParent();

                if (parent != NULL)
                {
                    return parent->sceneObject->GetComponentInHierarchyUp<T>();
                }
            }

            return NULL;
        }

    private:
        std::vector<Component*> components;
    };
}
#endif
