#include "OctreeRenderer.h"

OctreeRenderer::OctreeRenderer(const std::wstring &pointcloudFile)
{
    // Create the octree, throws exception on fail
    octree = new Octree(pointcloudFile);

    // Initialize constant buffer data
	octreeConstantBufferData.fovAngleY = settings->fovAngleY;
}

void OctreeRenderer::Initialize()
{
    // Create the constant buffer
    D3D11_BUFFER_DESC octreeConstantBufferDesc;
    ZeroMemory(&octreeConstantBufferDesc, sizeof(octreeConstantBufferDesc));
    octreeConstantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    octreeConstantBufferDesc.ByteWidth = sizeof(OctreeConstantBuffer);
    octreeConstantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    hr = d3d11Device->CreateBuffer(&octreeConstantBufferDesc, NULL, &octreeConstantBuffer);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(octreeRendererConstantBuffer));

    // Create the buffer for the compute shader that stores all the octree nodes
    // Maximum size is ~4.2 GB due to UINT_MAX
    D3D11_BUFFER_DESC nodesBufferDesc;
    ZeroMemory(&nodesBufferDesc, sizeof(nodesBufferDesc));
    nodesBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    nodesBufferDesc.ByteWidth = octree->nodes.size() * sizeof(OctreeNode);
    nodesBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    nodesBufferDesc.StructureByteStride = sizeof(OctreeNode);
    nodesBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    D3D11_SUBRESOURCE_DATA nodesBufferData;
    ZeroMemory(&nodesBufferData, sizeof(nodesBufferData));
	nodesBufferData.pSysMem = octree->nodes.data();

    hr = d3d11Device->CreateBuffer(&nodesBufferDesc, &nodesBufferData, &nodesBuffer);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(nodesBuffer));

    D3D11_SHADER_RESOURCE_VIEW_DESC nodesBufferSRVDesc;
    ZeroMemory(&nodesBufferSRVDesc, sizeof(nodesBufferSRVDesc));
    nodesBufferSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    nodesBufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    nodesBufferSRVDesc.Buffer.ElementWidth = sizeof(OctreeNode);
    nodesBufferSRVDesc.Buffer.NumElements = octree->nodes.size();

    hr = d3d11Device->CreateShaderResourceView(nodesBuffer, &nodesBufferSRVDesc, &nodesBufferSRV);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed for the " + NAMEOF(nodesBufferSRV));

    // Create general buffer description for append/consume buffer
    D3D11_BUFFER_DESC appendConsumeBufferDesc;
    ZeroMemory(&appendConsumeBufferDesc, sizeof(appendConsumeBufferDesc));
    appendConsumeBufferDesc.ByteWidth = settings->appendBufferCount * sizeof(OctreeNodeTraversalEntry);
    appendConsumeBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    appendConsumeBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    appendConsumeBufferDesc.StructureByteStride = sizeof(OctreeNodeTraversalEntry);
    appendConsumeBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    // Create general UAV description for append/consume buffers
    D3D11_UNORDERED_ACCESS_VIEW_DESC appendConsumeBufferUAVDesc;
    ZeroMemory(&appendConsumeBufferUAVDesc, sizeof(appendConsumeBufferUAVDesc));
    appendConsumeBufferUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    appendConsumeBufferUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    appendConsumeBufferUAVDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
    appendConsumeBufferUAVDesc.Buffer.NumElements = settings->appendBufferCount;

    // Create the first buffer and its UAV
    hr = d3d11Device->CreateBuffer(&appendConsumeBufferDesc, NULL, &firstBuffer);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(firstBuffer));

    hr = d3d11Device->CreateUnorderedAccessView(firstBuffer, &appendConsumeBufferUAVDesc, &firstBufferUAV);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateUnorderedAccessView) + L" failed for the " + NAMEOF(firstBufferUAV));

    // Create the second buffer and its UAV
    hr = d3d11Device->CreateBuffer(&appendConsumeBufferDesc, NULL, &secondBuffer);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(secondBuffer));

    hr = d3d11Device->CreateUnorderedAccessView(secondBuffer, &appendConsumeBufferUAVDesc, &secondBufferUAV);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateUnorderedAccessView) + L" failed for the " + NAMEOF(secondBufferUAV));

    // Create the vertex append buffer
    hr = d3d11Device->CreateBuffer(&appendConsumeBufferDesc, NULL, &vertexAppendBuffer);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(vertexAppendBuffer));

    hr = d3d11Device->CreateUnorderedAccessView(vertexAppendBuffer, &appendConsumeBufferUAVDesc, &vertexAppendBufferUAV);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateUnorderedAccessView) + L" failed for the " + NAMEOF(vertexAppendBufferUAV));

    D3D11_SHADER_RESOURCE_VIEW_DESC vertexAppendBufferSRVDesc;
    ZeroMemory(&vertexAppendBufferSRVDesc, sizeof(vertexAppendBufferSRVDesc));
    vertexAppendBufferSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    vertexAppendBufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    vertexAppendBufferSRVDesc.Buffer.ElementWidth = sizeof(OctreeNodeTraversalEntry);
    vertexAppendBufferSRVDesc.Buffer.NumElements = settings->appendBufferCount;

    hr = d3d11Device->CreateShaderResourceView(vertexAppendBuffer, &vertexAppendBufferSRVDesc, &vertexAppendBufferSRV);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateShaderResourceView) + L" failed for the " + NAMEOF(vertexAppendBufferSRV));

    // Create the structure count buffer that is simply used to check for the size of the append/consume buffers
    D3D11_BUFFER_DESC structureCountBufferDesc;
    ZeroMemory(&structureCountBufferDesc, sizeof(structureCountBufferDesc));
    structureCountBufferDesc.ByteWidth = sizeof(UINT);
    structureCountBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    structureCountBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    structureCountBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    hr = d3d11Device->CreateBuffer(&structureCountBufferDesc, NULL, &structureCountBuffer);
	ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(structureCountBuffer));
}

void OctreeRenderer::Update()
{
    // Set GUI variables
    GUI::vertexCount = vertexBufferCount;
}

void OctreeRenderer::Draw()
{
    // Transform the camera position into local space and save it in the constant buffers
    Matrix world = sceneObject->transform->worldMatrix;
    Matrix worldInverse = world.Invert();
    Vector3 cameraPosition = camera->GetPosition();
    Vector3 localCameraPosition = Vector4::Transform(Vector4(cameraPosition.x, cameraPosition.y, cameraPosition.z, 1), worldInverse);

	// Transform the view frustum into the local space of the vertices in order to do view frustum culling against the view frustum planes
	Matrix worldViewProjectionInverse = (sceneObject->transform->worldMatrix * camera->GetViewMatrix() * camera->GetProjectionMatrix()).Invert();

	// Set the initial values to be transformed by the matrix
	Vector3 localViewFrustum[8] =
	{
		Vector3(-1, 1, 0),		// Near Plane Top Left
		Vector3(1, 1, 0),		// Near Plane Top Right
		Vector3(-1, -1, 0),		// Near Plane Bottom Left
		Vector3(1, -1, 0),		// Near Plane Bottom Right
		Vector3(-1, 1, 1),		// Far Plane Top Left
		Vector3(1, 1, 1),		// Far Plane Top Right
		Vector3(-1, -1, 1),		// Far Plane Bottom Left
		Vector3(1, -1, 1)		// Far Plane Bottom Right
	};

	for (int i = 0; i < 8; i++)
	{
		// Transform into local space
		Vector4 transformed = Vector4(localViewFrustum[i].x, localViewFrustum[i].y, localViewFrustum[i].z, 1);
		transformed = Vector4::Transform(transformed, worldViewProjectionInverse);

		// Normalize by dividing by w
		localViewFrustum[i] = (1.0 / transformed.w) * Vector3(transformed.x, transformed.y, transformed.z);
	}

    // Set shader constant buffer variables
	octreeConstantBufferData.World = world.Transpose();
	octreeConstantBufferData.WorldInverseTranspose = worldInverse;
	octreeConstantBufferData.View = camera->GetViewMatrix().Transpose();
	octreeConstantBufferData.Projection = camera->GetProjectionMatrix().Transpose();
	octreeConstantBufferData.WorldViewProjectionInverse = worldViewProjectionInverse.Transpose();
	octreeConstantBufferData.cameraPosition = cameraPosition;
	octreeConstantBufferData.localCameraPosition = localCameraPosition;
	octreeConstantBufferData.localViewFrustumNearTopLeft = localViewFrustum[0];
	octreeConstantBufferData.localViewFrustumNearTopRight = localViewFrustum[1];
	octreeConstantBufferData.localViewFrustumNearBottomLeft = localViewFrustum[2];
	octreeConstantBufferData.localViewFrustumNearBottomRight = localViewFrustum[3];
	octreeConstantBufferData.localViewFrustumFarTopLeft = localViewFrustum[4];
	octreeConstantBufferData.localViewFrustumFarTopRight = localViewFrustum[5];
	octreeConstantBufferData.localViewFrustumFarBottomLeft = localViewFrustum[6];
	octreeConstantBufferData.localViewFrustumFarBottomRight = localViewFrustum[7];
	octreeConstantBufferData.localViewPlaneNearNormal = (localViewFrustum[1] - localViewFrustum[0]).Cross(localViewFrustum[2] - localViewFrustum[0]);
	octreeConstantBufferData.localViewPlaneFarNormal = (localViewFrustum[7] - localViewFrustum[6]).Cross(localViewFrustum[4] - localViewFrustum[6]);
	octreeConstantBufferData.localViewPlaneLeftNormal = (localViewFrustum[0] - localViewFrustum[4]).Cross(localViewFrustum[6] - localViewFrustum[4]);
	octreeConstantBufferData.localViewPlaneRightNormal = (localViewFrustum[3] - localViewFrustum[7]).Cross(localViewFrustum[5] - localViewFrustum[7]);
	octreeConstantBufferData.localViewPlaneTopNormal = (localViewFrustum[4] - localViewFrustum[0]).Cross(localViewFrustum[1] - localViewFrustum[0]);
	octreeConstantBufferData.localViewPlaneBottomNormal = (localViewFrustum[3] - localViewFrustum[2]).Cross(localViewFrustum[6] - localViewFrustum[2]);
	octreeConstantBufferData.localViewPlaneNearNormal.Normalize();
	octreeConstantBufferData.localViewPlaneFarNormal.Normalize();
	octreeConstantBufferData.localViewPlaneLeftNormal.Normalize();
	octreeConstantBufferData.localViewPlaneRightNormal.Normalize();
	octreeConstantBufferData.localViewPlaneTopNormal.Normalize();
	octreeConstantBufferData.localViewPlaneBottomNormal.Normalize();
    octreeConstantBufferData.splatResolution = settings->splatResolution;
	octreeConstantBufferData.samplingRate = settings->samplingRate;
	octreeConstantBufferData.blendFactor = settings->blendFactor;
    octreeConstantBufferData.useCulling = settings->useCulling;
    octreeConstantBufferData.level = settings->octreeLevel;

    // Draw overlapping splats to make sure that continuous surfaces without holes are drawn
    // Higher overlap factor reduces the spacing between tilted splats but reduces the detail (blend overlapping splats to improve this)
    // 1.0f = Orthogonal splats to the camera are as large as the pixel area they should fill and do not overlap
    // 2.0f = Orthogonal splats to the camera are twice as large and overlap with all their surrounding splats
	octreeConstantBufferData.overlapFactor = settings->overlapFactor;

	// Do not blend in the first pass
	octreeConstantBufferData.useBlending = false;

    // Update the hlsl file buffer, set shader buffer to our created buffer
    d3d11DevCon->UpdateSubresource(octreeConstantBuffer, 0, NULL, &octreeConstantBufferData, 0, 0);

    // Set shader buffer
    d3d11DevCon->VSSetConstantBuffers(0, 1, &octreeConstantBuffer);
    d3d11DevCon->GSSetConstantBuffers(0, 1, &octreeConstantBuffer);
	d3d11DevCon->PSSetConstantBuffers(0, 1, &octreeConstantBuffer);

    // Get the vertex buffer and use the specified implementation
    if (settings->useGPUTraversal)
    {
        DrawOctreeCompute();
    }
    else
    {
        DrawOctree();
    }
}

void OctreeRenderer::Release()
{
    SAFE_DELETE(octree);

    SAFE_RELEASE(nodesBuffer);
    SAFE_RELEASE(firstBuffer);
    SAFE_RELEASE(secondBuffer);
    SAFE_RELEASE(vertexAppendBuffer);
    SAFE_RELEASE(structureCountBuffer);
    SAFE_RELEASE(nodesBufferSRV);
    SAFE_RELEASE(firstBufferUAV);
    SAFE_RELEASE(secondBufferUAV);
	SAFE_RELEASE(vertexAppendBufferSRV);
    SAFE_RELEASE(vertexAppendBufferUAV);
    SAFE_RELEASE(octreeConstantBuffer);
}

void PointCloudEngine::OctreeRenderer::GetBoundingCubePositionAndSize(Vector3 &outPosition, float &outSize)
{
	outPosition = octree->rootPosition;
	outSize = octree->rootSize;
}

void PointCloudEngine::OctreeRenderer::RemoveComponentFromSceneObject()
{
	sceneObject->RemoveComponent(this);
}

void PointCloudEngine::OctreeRenderer::DrawOctree()
{
	// Create new buffer from the current octree traversal on the cpu
    std::vector<OctreeNodeVertex> octreeVertices = octree->GetVertices(octreeConstantBufferData);

    vertexBufferCount = octreeVertices.size();

    if (vertexBufferCount > 0)
    {
        ID3D11Buffer* vertexBuffer = NULL;

        // Create a vertex buffer description
        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(OctreeNodeVertex) * vertexBufferCount;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;

        // Fill a D3D11_SUBRESOURCE_DATA struct with the data we want in the buffer
        D3D11_SUBRESOURCE_DATA vertexBufferData;
        ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
        vertexBufferData.pSysMem = &octreeVertices[0];

        // Create the buffer
        hr = d3d11Device->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11Device->CreateBuffer) + L" failed for the " + NAMEOF(vertexBuffer));

        // Set the shaders
        if (settings->viewMode == ViewMode::OctreeSplats)
        {
            d3d11DevCon->VSSetShader(octreeSplatShader->vertexShader, 0, 0);
            d3d11DevCon->GSSetShader(octreeSplatShader->geometryShader, 0, 0);
            d3d11DevCon->PSSetShader(octreeSplatShader->pixelShader, 0, 0);
        }
        else if (settings->viewMode == ViewMode::OctreeNodes)
        {
            d3d11DevCon->VSSetShader(octreeCubeShader->vertexShader, 0, 0);
            d3d11DevCon->GSSetShader(octreeCubeShader->geometryShader, 0, 0);
            d3d11DevCon->PSSetShader(octreeCubeShader->pixelShader, 0, 0);
        }
        else if (settings->viewMode == ViewMode::OctreeNormalClusters)
        {
            d3d11DevCon->VSSetShader(octreeClusterShader->vertexShader, 0, 0);
            d3d11DevCon->GSSetShader(octreeClusterShader->geometryShader, 0, 0);
            d3d11DevCon->PSSetShader(octreeClusterShader->pixelShader, 0, 0);
        }

        // Set the Input (Vertex) Layout
        d3d11DevCon->IASetInputLayout(octreeCubeShader->inputLayout);

        // Bind the vertex buffer and to the input assembler (IA)
        UINT offset = 0;
        UINT stride = sizeof(OctreeNodeVertex);
        d3d11DevCon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

        // Set primitive topology
        d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

		// Only blend splats
		if (settings->useBlending && (settings->viewMode == ViewMode::OctreeSplats))
		{
			DrawBlended(vertexBufferCount, octreeConstantBuffer, &octreeConstantBufferData, octreeConstantBufferData.useBlending);
		}
		else
		{
			d3d11DevCon->Draw(vertexBufferCount, 0);
		}

        SAFE_RELEASE(vertexBuffer);
    }
}

void PointCloudEngine::OctreeRenderer::DrawOctreeCompute()
{
    // Set the constant buffer
    d3d11DevCon->CSSetConstantBuffers(0, 1, &octreeConstantBuffer);

    // Use compute shader to traverse the octree
    UINT zero = 0;
    d3d11DevCon->CSSetShader(octreeComputeShader->computeShader, 0, 0);
    d3d11DevCon->CSSetShaderResources(0, 1, &nodesBufferSRV);
    d3d11DevCon->CSSetUnorderedAccessViews(2, 1, &vertexAppendBufferUAV, &zero);

	// Set root entry for the first buffer, will be used as input consume buffer in the shader
	OctreeNodeTraversalEntry rootEntry;
	rootEntry.index = 0;
	rootEntry.position = octree->rootPosition;
	rootEntry.size = octree->rootSize;
	rootEntry.parentInsideViewFrustum = false;
	rootEntry.depth = 0;

	D3D11_BOX rootEntryBox;
	rootEntryBox.left = 0;
	rootEntryBox.right = sizeof(OctreeNodeTraversalEntry);
	rootEntryBox.top = 0;
	rootEntryBox.bottom = 1;
	rootEntryBox.front = 0;
	rootEntryBox.back = 1;

	// Copy to the first element in the buffer
	d3d11DevCon->UpdateSubresource(firstBuffer, 0, &rootEntryBox, &rootEntry, 0, 0);

    // Stop iterating when all levels of the octree were checked
    octreeConstantBufferData.inputCount = 1;
    bool firstBufferIsInputConsumeBuffer = true;

    // Swap the input and output buffer as long as there is data in the output append buffer
    do
    {
        // Unbind first
        d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);
        d3d11DevCon->CSSetUnorderedAccessViews(1, 1, nullUAV, &zero);

        if (firstBufferIsInputConsumeBuffer)
        {
            d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &firstBufferUAV, &octreeConstantBufferData.inputCount);
            d3d11DevCon->CSSetUnorderedAccessViews(1, 1, &secondBufferUAV, &zero);
        }
        else
        {
            d3d11DevCon->CSSetUnorderedAccessViews(0, 1, &secondBufferUAV, &octreeConstantBufferData.inputCount);
            d3d11DevCon->CSSetUnorderedAccessViews(1, 1, &firstBufferUAV, &zero);
        }

        // Update the input count of the constant buffer to make sure that not too much is appended or consumed
        d3d11DevCon->UpdateSubresource(octreeConstantBuffer, 0, NULL, &octreeConstantBufferData, 0, 0);

        // Execution of the compute shader, appends the indices of the nodes that should be checked next to the output append buffer
        d3d11DevCon->Dispatch(ceil(octreeConstantBufferData.inputCount / 1024.0f), 1, 1);

        // Get the output append buffer structure count
        if (firstBufferIsInputConsumeBuffer)
        {
			octreeConstantBufferData.inputCount = min(settings->appendBufferCount, GetStructureCount(secondBufferUAV));
        }
        else
        {
			octreeConstantBufferData.inputCount = min(settings->appendBufferCount, GetStructureCount(firstBufferUAV));
        }

        // Swap the buffers
        firstBufferIsInputConsumeBuffer = !firstBufferIsInputConsumeBuffer;

    } while (octreeConstantBufferData.inputCount > 0);

    // Get the actual vertex buffer count from the vertex append buffer structure counter
    vertexBufferCount = min(settings->appendBufferCount, GetStructureCount(vertexAppendBufferUAV));

    // Unbind nodes and vertex append buffer in order to use it in the vertex shader
    d3d11DevCon->CSSetShaderResources(0, 1, nullSRV);
    d3d11DevCon->CSSetUnorderedAccessViews(0, 1, nullUAV, &zero);
    d3d11DevCon->CSSetUnorderedAccessViews(1, 1, nullUAV, &zero);
    d3d11DevCon->CSSetUnorderedAccessViews(2, 1, nullUAV, &zero);

    // Set the shaders, only the vertex shader is different from the CPU implementation
    d3d11DevCon->VSSetShader(octreeComputeVSShader->vertexShader, 0, 0);

    if (settings->viewMode == ViewMode::OctreeSplats)
    {
        d3d11DevCon->GSSetShader(octreeSplatShader->geometryShader, 0, 0);
        d3d11DevCon->PSSetShader(octreeSplatShader->pixelShader, 0, 0);
    }
    else if (settings->viewMode == ViewMode::OctreeNodes)
    {
        d3d11DevCon->GSSetShader(octreeCubeShader->geometryShader, 0, 0);
        d3d11DevCon->PSSetShader(octreeCubeShader->pixelShader, 0, 0);
    }
    else if (settings->viewMode == ViewMode::OctreeNormalClusters)
    {
        d3d11DevCon->GSSetShader(octreeClusterShader->geometryShader, 0, 0);
        d3d11DevCon->PSSetShader(octreeClusterShader->pixelShader, 0, 0);
    }

    // Set the vertex append buffer as structured buffer in the vertex shader
    d3d11DevCon->VSSetShaderResources(0, 1, &nodesBufferSRV);
    d3d11DevCon->VSSetShaderResources(1, 1, &vertexAppendBufferSRV);

    // Set an empty input layout and vertex buffer that only sends the vertex id to the shader
    d3d11DevCon->IASetInputLayout(NULL);
    d3d11DevCon->IASetVertexBuffers(0, 1, nullBuffer, &zero, &zero);
    d3d11DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// Only blend splats
	if (settings->useBlending && (settings->viewMode == ViewMode::OctreeSplats))
	{
		DrawBlended(vertexBufferCount, octreeConstantBuffer, &octreeConstantBufferData, octreeConstantBufferData.useBlending);
	}
	else
	{
		d3d11DevCon->Draw(vertexBufferCount, 0);
	}

    // Unbind the shader resources
    d3d11DevCon->VSSetShaderResources(0, 1, nullSRV);
    d3d11DevCon->VSSetShaderResources(1, 1, nullSRV);
}

UINT PointCloudEngine::OctreeRenderer::GetStructureCount(ID3D11UnorderedAccessView *UAV)
{
    UINT output = 0;

    if (structureCountBuffer != NULL)
    {
        // Copy the count into the buffer
        d3d11DevCon->CopyStructureCount(structureCountBuffer, 0, UAV);

        // Read the value by mapping the memory to the CPU
        D3D11_MAPPED_SUBRESOURCE mappedSubresource;
        hr = d3d11DevCon->Map(structureCountBuffer, 0, D3D11_MAP_READ, 0, &mappedSubresource);
		ERROR_MESSAGE_ON_HR(hr, NAMEOF(d3d11DevCon->Map) + L" failed for the " + NAMEOF(structureCountBuffer));

        output = *(UINT*)mappedSubresource.pData;

        d3d11DevCon->Unmap(structureCountBuffer, 0);
    }

    return output;
}
