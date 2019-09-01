#include "Camera.h"

PointCloudEngine::Camera::Camera()
{
    // Create the viewport
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.Width = settings->resolutionX;
    viewport.Height = settings->resolutionY;
}

void PointCloudEngine::Camera::PrepareDraw()
{
    RecalculateRightUpForwardViewProjection();

    // Bind viewport to rasterization stage
    d3d11DevCon->RSSetViewports(1, &viewport);
}

void PointCloudEngine::Camera::SetRotationMatrix(Matrix rotation)
{
    this->rotation = rotation;
    recalculate = true;
}

void PointCloudEngine::Camera::Rotate(float x, float y, float z)
{
    rotation *= Matrix::CreateRotationX(x) * Matrix::CreateRotationY(y) * Matrix::CreateRotationZ(z);
    recalculate = true;
}

void PointCloudEngine::Camera::LookAt(Vector3 targetPosition)
{
	// Create the corresponding camera view matrix
	view = XMMatrixLookAtLH(position, targetPosition, Vector3::UnitY);

	// Decompose it
	XMVECTOR outScale, outTranslation, outRotation;
	XMMatrixDecompose(&outScale, &outRotation, &outTranslation, view);

	// Only get the actual rotation matrix
	rotation = Matrix::CreateFromQuaternion(outRotation).Invert();

    recalculate = true;
}

void PointCloudEngine::Camera::SetPosition(Vector3 position)
{
    this->position = position;
    recalculate = true;
}

void PointCloudEngine::Camera::Translate(Vector3 translation)
{
    position += translation;
    recalculate = true;
}

void PointCloudEngine::Camera::TranslateRUF(float deltaRight, float deltaUp, float deltaForward)
{
    position += deltaRight * right + deltaUp * up + deltaForward * forward;
    recalculate = true;
}

void PointCloudEngine::Camera::RecalculateRightUpForwardViewProjection()
{
    // Only calculate when necessary
    if (recalculate)
    {
        // Calculate right, up, forward
		right = Vector3::Transform(Vector3::UnitX, rotation);
		up = Vector3::Transform(Vector3::UnitY, rotation);
		forward = Vector3::Transform(Vector3::UnitZ, rotation);

		// Normalize
		right.Normalize();
		forward.Normalize();
		up.Normalize();

        // Calculate view projection
		view = XMMatrixLookToLH(position, forward, up);
		projection = XMMatrixPerspectiveFovLH(settings->fovAngleY, (float)settings->resolutionX / (float)settings->resolutionY, settings->nearZ, settings->farZ);

        recalculate = false;
    }
}

Vector3 PointCloudEngine::Camera::GetPosition()
{
    return position;
}

Vector3 PointCloudEngine::Camera::GetRight()
{
    RecalculateRightUpForwardViewProjection();
    return right;
}

Vector3 PointCloudEngine::Camera::GetUp()
{
    RecalculateRightUpForwardViewProjection();
    return up;
}

Vector3 PointCloudEngine::Camera::GetForward()
{
    RecalculateRightUpForwardViewProjection();
    return forward;
}

Matrix PointCloudEngine::Camera::GetViewMatrix()
{
    RecalculateRightUpForwardViewProjection();
    return view;
}

Matrix PointCloudEngine::Camera::GetProjectionMatrix()
{
    RecalculateRightUpForwardViewProjection();
    return projection;
}

Matrix PointCloudEngine::Camera::GetRotationMatrix()
{
    return rotation;
}