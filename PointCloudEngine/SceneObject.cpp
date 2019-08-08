#include "SceneObject.h"

SceneObject::SceneObject(std::wstring name, Transform *parent, std::initializer_list<Component*> components)
{
    this->name = name;
    this->transform = new Transform();
    this->transform->SetParent(parent);
    this->components = components;

    transform->sceneObject = this;

    for (auto it = components.begin(); it != components.end(); it++)
    {
        AddComponent(*it);
    }
}

SceneObject::~SceneObject()
{
    SafeDelete(transform);

    for (auto it = components.begin(); it != components.end(); it++)
    {
        Component *component = *it;

        if (!component->shared)
        {
            SafeDelete(*it);
        }
    }
}

SceneObject* SceneObject::FindChildByName(std::wstring childName)
{
    std::vector<Transform*> const *children = transform->GetChildren();

    // First search in the children level
    for (auto it = children->begin(); it != children->end(); it++)
    {
        SceneObject* child = (*it)->sceneObject;

        if (child->name == childName)
        {
            return child;
        }
    }

    // Then search in the grandchildren level
    for (auto it = children->begin(); it != children->end(); it++)
    {
        SceneObject* tmp = (*it)->sceneObject->FindChildByName(childName);

        if (tmp != NULL)
        {
            return tmp;
        }
    }

    return NULL;
}

void PointCloudEngine::SceneObject::RemoveComponent(Component *componentToRemove)
{
    components.erase(std::remove(components.begin(), components.end(), componentToRemove), components.end());
    componentToRemove->Release();
    SafeDelete(componentToRemove);
}

void SceneObject::Update()
{
    for (auto it = components.begin(); it != components.end(); it++)
    {
        Component *component = *it;
        
        // Initialize the component if necessary
        if (!component->initialized)
        {
			component->sceneObject = this;
            component->Initialize();
            component->initialized = true;
        }

		if (component->enabled)
		{
			component->Update();
		}
    }
}

void SceneObject::Draw()
{
    for (auto it = components.begin(); it != components.end(); it++)
    {
		Component *component = *it;

		if (component->enabled)
		{
			component->Draw();
		}
    }
}

void SceneObject::Release()
{
    for (auto it = components.begin(); it != components.end(); it++)
    {
        Component *component = *it;

        if (!component->shared)
        {
            component->Release();
        }
    }
}
