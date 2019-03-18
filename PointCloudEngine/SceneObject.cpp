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

SceneObject* SceneObject::Duplicate()
{
    SceneObject *duplicate = Hierarchy::Create(name, transform->GetParent());
    duplicate->transform->position = transform->position;
    duplicate->transform->rotation = transform->rotation;
    duplicate->transform->scale = transform->scale;

    // Duplicate all components of this object
    for (auto it = components.begin(); it != components.end(); it++)
    {
        Component *duplicateComponent = (*it)->Duplicate();
        duplicate->AddComponent(duplicateComponent);
    }

    std::vector<Transform*> const *children = transform->GetChildren();
    int size = children->size();

    // Duplicate all children
    for (int i = 0; i < size; i++)
    {
        (*children)[i]->sceneObject->Duplicate()->transform->SetParent(duplicate->transform);
    }

    return duplicate;
}

void SceneObject::Update()
{
    for (auto it = components.begin(); it != components.end(); it++)
    {
        Component *component = *it;
        
        // Initialize the component if necessary
        if (!component->initialized)
        {
            component->Initialize(this);
            component->initialized = true;
        }

        component->Update(this);
    }
}

void SceneObject::Draw()
{
    for (auto it = components.begin(); it != components.end(); it++)
    {
        (*it)->Draw(this);
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
