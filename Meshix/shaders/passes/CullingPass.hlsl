// ====================================================
//                  Compute Shader
// ====================================================

#include "../structures.h"

ConstantBuffer<FrameConstants>       frameConstants       : register(b0);
ConstantBuffer<CullingViewConstants> cullingViewConstants : register(b1);

StructuredBuffer<MaterialConstants>    materialConstants    : register(t0);
StructuredBuffer<ObjectConstants>      objectConstants      : register(t1);
StructuredBuffer<MeshCullingConstants> meshCullingConstants : register(t2);

RWBuffer<uint> meshIndirectCount : register(u0);

bool CullAABBWorld(
    float3 worldCenter,
    float3 worldExtents,
    float4 planes[6],
    uint skipMask)
{
    for (int i = 0; i < 6; i++) {
        if (skipMask & (1u << i)) continue;
        float r = dot(worldExtents, abs(planes[i].xyz));
        float s = dot(worldCenter, planes[i].xyz) + planes[i].w;
        if (s + r < -0.001) return true;
    }
    return false;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 threadId : SV_DispatchThreadID) {
    uint meshIndex = threadId.x;
    if (meshIndex >= cullingViewConstants.MeshCount) return;

    MeshCullingConstants mesh     = meshCullingConstants[meshIndex]; if (!mesh.ObjectHandle) return;
    MaterialConstants    material = materialConstants[mesh.MaterialHandle]; if (material.alphaMode == 2) return;
    ObjectConstants      object   = objectConstants[mesh.ObjectHandle - 1];

    float3 worldCenter = mul(object.World, float4(mesh.LocalBounds.Center, 1.0)).xyz;
    float3 worldExtents =
        abs(mesh.LocalBounds.Extents.x * float3(object.World._11, object.World._21, object.World._31)) +
        abs(mesh.LocalBounds.Extents.y * float3(object.World._12, object.World._22, object.World._32)) +
        abs(mesh.LocalBounds.Extents.z * float3(object.World._13, object.World._23, object.World._33));

    for (uint i = 0; i < CULLING_VIEW_NUM; ++i) {
        CullingViewData viewData = cullingViewConstants.CullingViewDatas[i];
        if (CullAABBWorld(worldCenter, worldExtents, viewData.FrustumPlanes, viewData.CullSkipMask)) continue;

        uint slot;
        InterlockedAdd(meshIndirectCount[i], 1, slot);
        if (slot < viewData.MaxCommandCount) {
            MeshIndirectCommand indirectCommand;
            indirectCommand.MaterialHandle = mesh.MaterialHandle;
            indirectCommand.ObjectHandle   = mesh.ObjectHandle - 1;

            indirectCommand.VBAddress = mesh.VertexBufferAddress;
            indirectCommand.VBSize    = 56 * mesh.VertexCount;
            indirectCommand.VBStride  = 56;

            indirectCommand.IBAddress = mesh.IndexBufferAddress;
            indirectCommand.IBSize    = 4 * mesh.IndexCount;
            indirectCommand.IBFormat  = 42; // DXGI_FORMAT_R32_UINT

            indirectCommand.IndexCountPerInstance = mesh.IndexCount;
            indirectCommand.InstanceCount = 1;
            indirectCommand.StartIndexLocation = 0;
            indirectCommand.BaseVertexLocation = 0;
            indirectCommand.StartInstanceLocation = 0;

            indirectCommand.Padding1 = 0;

            RWStructuredBuffer<MeshIndirectCommand> cmdBuf = ResourceDescriptorHeap[NonUniformResourceIndex(viewData.IndirectCommandBufferHandle)];
            cmdBuf[slot] = indirectCommand;
        }
    }
}
