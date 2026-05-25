#ifndef CONSTANTS_BUFFER_STRUCTS_H
#define CONSTANTS_BUFFER_STRUCTS_H

#include "chlsl.h"

#define CASCADE_NUM 5
#define CULLING_VIEW_NUM (1 + CASCADE_NUM)

#ifndef __cplusplus
#include "csm.h"
struct MaterialConstants {
    uint baseColorHandle;
    uint metallicRoughnessHandle;
    uint normalHandle;
    uint occlusionHandle;

    uint  emissiveHandle;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;

    float  occlusionStrength;
    float3 emissiveFactor;

    float4 baseColorFactor;

    uint  alphaMode;
    float alphaCutoff;
    int   doubleSided;
    float padding;
};
#else
#include <Vertix.Engine/Effect/Shadow/CascadeShadowMapping.h>
#include <Vertix.Engine/Primitive/DefaultPBRMaterial.h>
using CascadeData       = Vertix::Engine::CascadeData;
using MaterialConstants = Vertix::Engine::DefaultMaterialConstants;
#endif

struct BoundingSphere {
    float3 Center;
    float  Radius;
};

struct BoundingBox {
    float3 Center;
    float  Padding_1;

    float3 Extents;
    float  Padding_2;
};

struct CullingViewData {
    float4 FrustumPlanes[6]; // Left / Right / Bottom / Top / Near / Far

    uint IndirectCommandBufferHandle;
    uint MaxCommandCount;
    uint CullSkipMask;
    uint Padding;
};

struct FrameConstants {
    float4x4 View                  IDENTITY;
    float4x4 Projection            IDENTITY;
    float4x4 ViewProjection        IDENTITY;

    float4x4 ViewProjectionInverse IDENTITY;

    float4 CameraPosition;
    float4 NearFarProjScale;

    float2 FrameResolution;
    float2 FrameResolutionInverse;
};

struct LightConstants {
    float3 LightDirection;
    float  AmbientIntensity;

    float3 LightColor;
    float  LightIntensity;
};

struct CascadeShadowConstants {
    CascadeData CascadeDatas[CASCADE_NUM];
};

struct ObjectConstants {
    float4x4 World                 IDENTITY;
    float4x4 WorldInverseTranspose IDENTITY;
};

struct CullingViewConstants {
    CullingViewData CullingViewDatas[CULLING_VIEW_NUM];

    uint MeshCount;
    float3 Padding;
};

struct MeshCullingConstants {
    uint ObjectHandle   ZERO;
    uint MaterialHandle ZERO;
    uint IndexCount     ZERO;
    uint VertexCount    ZERO;

    uint64 IndexBufferAddress;
    uint64 VertexBufferAddress;

    BoundingBox LocalBounds;
};

struct MeshIndirectCommand {
    uint MaterialHandle;
    uint ObjectHandle;

    uint64 VBAddress;
    uint   VBSize;
    uint   VBStride;

    uint64 IBAddress;
    uint   IBSize;
    uint   IBFormat;

    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int  BaseVertexLocation;
    uint StartInstanceLocation;
    float Padding1;
};

#endif // CONSTANTS_BUFFER_STRUCTS_H
