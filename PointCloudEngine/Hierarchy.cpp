#include "Hierarchy.h"

std::vector<Transform*> Hierarchy::rootTransforms;
std::vector<SceneObject*> Hierarchy::sceneObjects;

SceneObject* Hierarchy::Create(std::wstring name, Transform *parent, std::initializer_list<Component*> components)
{
    SceneObject* tmp = new SceneObject(name, parent, components);
    sceneObjects.push_back(tmp);
    return tmp;
}

void PointCloudEngine::Hierarchy::ReleaseSceneObject(SceneObject *sceneObjectToRemove)
{
    // Remove from root transforms
    rootTransforms.erase(std::remove(rootTransforms.begin(), rootTransforms.end(), sceneObjectToRemove->transform), rootTransforms.end());

    // Remove from scene objects
    sceneObjects.erase(std::remove(sceneObjects.begin(), sceneObjects.end(), sceneObjectToRemove), sceneObjects.end());
    sceneObjectToRemove->Release();
    SAFE_DELETE(sceneObjectToRemove);
}

void PointCloudEngine::Hierarchy::CalculateWorldMatrices()
{
	CalculateWorldMatrices(&rootTransforms);
}

void Hierarchy::UpdateAllSceneObjects()
{
    for (auto it = sceneObjects.begin(); it != sceneObjects.end(); it++)
    {
        (*it)->Update();
    }
}

void Hierarchy::DrawAllSceneObjects()
{
    for (auto it = sceneObjects.begin(); it != sceneObjects.end(); it++)
    {
        (*it)->Draw();
    }
}

void Hierarchy::ReleaseAllSceneObjects()
{
    for (auto it = sceneObjects.begin(); it != sceneObjects.end(); it++)
    {
        SceneObject* sceneObject = *it;
        sceneObject->Release();
        SAFE_DELETE(sceneObject);
    }

    sceneObjects.clear();
}

void Hierarchy::CalculateWorldMatrices(std::vector<Transform*> const *children)
{
    for (auto it = children->begin(); it != children->end(); it++)
    {
        Transform *transform = (*it);
        Transform *parent = transform->GetParent();

        Matrix M = Matrix::CreateScale(transform->scale) * Matrix::CreateFromQuaternion(transform->rotation) * Matrix::CreateTranslation(transform->position);

        if (parent != NULL)
        {
            M = M * parent->worldMatrix;
        }

        transform->worldMatrix = M;

        // Do the same for the children of this transform since the parent matrix is now calculated
        CalculateWorldMatrices(transform->GetChildren());
    }
}
