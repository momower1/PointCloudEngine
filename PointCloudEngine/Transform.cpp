#include "Transform.h"

void Transform::SetParent(Transform *parent)
{
    if (this->parent == NULL)
    {
        // Remove this transform from the root transforms
        for (auto it = Hierarchy::rootTransforms.begin(); it != Hierarchy::rootTransforms.end(); it++)
        {
            if (*it == this)
            {
                Hierarchy::rootTransforms.erase(it);
                break;
            }
        }
    }
    else
    {
        std::vector<Transform*> *parentChildren = &this->parent->children;

        // Remove this transform from the old parents children
        for (auto it = parentChildren->begin(); it != parentChildren->end(); it++)
        {
            if (*it == this)
            {
                parentChildren->erase(it);
                break;
            }
        }
    }

    this->parent = parent;

    // Set parent as root if it would otherwise be NULL
    if (this->parent == NULL)
    {
        Hierarchy::rootTransforms.push_back(this);
    }
    else
    {
        // Add this transform to the children of the new parent
        this->parent->children.push_back(this);
    }
}

Transform* Transform::GetParent()
{
    return parent;
}

std::vector<Transform*> const * Transform::GetChildren()
{
    return &children;
}
