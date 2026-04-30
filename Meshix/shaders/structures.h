#ifndef CONSTANTS_BUFFER_STRUCTS_H
#define CONSTANTS_BUFFER_STRUCTS_H

#include "chlsl.h"
#include "csm.h"

#define CASCADE_NUM 5

#ifndef __cplusplus
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

struct CascadeShadowConstants {
    CascadeData CascadeDatas[CASCADE_NUM];
};
#endif

struct FrameConstants {
    float4x4 View           IDENTITY;
    float4x4 Projection     IDENTITY;
    float4x4 ViewProjection IDENTITY;

    float4 CameraPosition;

    float2 FrameResolution;
    float2 FrameResolutionInverse;
};

struct LightConstants {
    float3 LightDirection;
    float  AmbientIntensity;

    float3 LightColor;
    float  LightIntensity;
};

struct ObjectConstants {
    float4x4 World                 IDENTITY;
    float4x4 WorldInverseTranspose IDENTITY;
};

#endif // CONSTANTS_BUFFER_STRUCTS_H
