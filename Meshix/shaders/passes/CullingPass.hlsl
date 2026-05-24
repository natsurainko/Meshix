// ====================================================
//                  Compute Shader
// ====================================================

#include "../structures.h"

ConstantBuffer<FrameConstants> frameConstants  : register(b0);

cbuffer constants : register(b1) {
    uint MeshCount;
}

StructuredBuffer<MaterialConstants>    materialConstants    : register(t0);
StructuredBuffer<ObjectConstants>      objectConstants      : register(t1);
StructuredBuffer<MeshCullingConstants> meshCullingConstants : register(t2);

RWStructuredBuffer<MeshIndirectCommand> indirectCommands    : register(u0);
RWBuffer<uint>                          visibleCount        : register(u1);

bool CullAABB(
    BoundingBox localBox,
    float4x4 world,
    float4 planes[6])
{
    float3 worldCenter = mul(world, float4(localBox.Center, 1.0)).xyz;
    float3 worldExtents =
        abs(localBox.Extents.x * float3(world._11, world._21, world._31)) +
        abs(localBox.Extents.y * float3(world._12, world._22, world._32)) +
        abs(localBox.Extents.z * float3(world._13, world._23, world._33));

    for (int i = 0; i < 6; ++i) {
        float r = dot(worldExtents, abs(planes[i].xyz));
        float s = dot(worldCenter, planes[i].xyz) + planes[i].w;
        if (s + r < 0.0) return true;
    }

    return false;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 threadId : SV_DispatchThreadID) {
    uint meshIndex = threadId.x;
    if (meshIndex >= MeshCount) return;

    MeshCullingConstants cullingMesh = meshCullingConstants[meshIndex];
    MaterialConstants material = materialConstants[cullingMesh.MaterialHandle];
    if (material.alphaMode == 2) return;

    if (!cullingMesh.ObjectHandle) return;
    ObjectConstants object = objectConstants[cullingMesh.ObjectHandle - 1];
    if (CullAABB(cullingMesh.LocalBounds, object.World, frameConstants.FrustumPlanes)) return;

    MeshIndirectCommand indirectCommand = (MeshIndirectCommand)0;
    indirectCommand.VBAddress = cullingMesh.VertexBufferAddress;
    indirectCommand.VBSize    = 56 * cullingMesh.VertexCount;
    indirectCommand.VBStride  = 56;

    indirectCommand.IBAddress = cullingMesh.IndexBufferAddress;
    indirectCommand.IBSize    = 4 * cullingMesh.IndexCount;
    indirectCommand.IBFormat  = 42; // DXGI_FORMAT_R32_UINT

    indirectCommand.IndexCountPerInstance = cullingMesh.IndexCount;
    indirectCommand.InstanceCount = 1;
    indirectCommand.StartIndexLocation = 0;
    indirectCommand.BaseVertexLocation = 0;
    indirectCommand.StartInstanceLocation = 0;

    indirectCommand.ObjectHandle = cullingMesh.ObjectHandle - 1;
    indirectCommand.MaterialHandle = cullingMesh.MaterialHandle;

    uint slot;
    InterlockedAdd(visibleCount[0], 1, slot);
    indirectCommands[slot] = indirectCommand;
}
