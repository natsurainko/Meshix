#include "../structures.h"

// ====================================================
//                    Vertex Shader
// ====================================================

cbuffer VS : register(b0) {
    uint ObjectHandle;
}

ConstantBuffer<FrameConstants> frameConstants  : register(b1);

StructuredBuffer<ObjectConstants> objectConstants : register(t0);

struct VSInput {
    float3 Position  : POSITION;
    float3 Normal    : NORMAL;
    float2 TexCoord  : TEXCOORD;
    float3 Tangent   : TANGENT;
    float3 Bitangent : BINORMAL;
};

struct VSOutput {
    float3 FragNormal : NORMAL;
    float3 Tangent    : TANGENT0;
    float3 Bitangent  : TANGENT1;
    float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_POSITION;
};

VSOutput VSMain(VSInput vsInput) {
    ObjectConstants object = objectConstants[ObjectHandle];
    VSOutput output;
    output.FragNormal = mul((float3x3)object.WorldInverseTranspose, vsInput.Normal);
    output.Tangent    = mul((float3x3)object.WorldInverseTranspose, vsInput.Tangent);
    output.Bitangent  = mul((float3x3)object.WorldInverseTranspose, vsInput.Bitangent);
    output.TexCoord   = vsInput.TexCoord;
    output.Position   = mul(frameConstants.ViewProjection, mul(object.World, float4(vsInput.Position, 1.0f)));
    return output;
}

// ====================================================
//                    Pixel Shader
// ====================================================

cbuffer PS : register(b0) {
    uint MaterialHandle;
}

SamplerState AnisotropicSampler : register(s0);

StructuredBuffer<MaterialConstants> materialConstants : register(t0);

// 1-based handle
template<typename T>
T GetTexture(uint handle) {
    return ResourceDescriptorHeap[handle];
}

// 1-based handle
MaterialConstants GetMaterial(uint handle) {
    return materialConstants[handle];
}

struct PSOutput {
    float4 GNormal : SV_Target0;
    float4 GAlbedo : SV_Target1;
    float4 GORM    : SV_Target2;
};

PSOutput PSMain(VSOutput psInput) {
    float3 normal    = psInput.FragNormal;
    float4 baseColor = float4(1.0, 1.0, 1.0, 1.0);
    float  metallic  = 0;
    float  roughness = 0.25;
    float  occlusion = 1.0;

    if (MaterialHandle) {
        MaterialConstants material = GetMaterial(MaterialHandle);

        baseColor = material.baseColorFactor;
        metallic  = material.metallicFactor;
        roughness = material.roughnessFactor;

        if (material.baseColorHandle) {
            Texture2D<float4> tBaseColor = GetTexture<Texture2D<float4> >(material.baseColorHandle);
            baseColor *= tBaseColor.Sample(AnisotropicSampler, psInput.TexCoord);

            // The alphaMode here can only be 0 or 1. When it is 2, forward rendering will be used directly.
            if (material.alphaMode && baseColor.a <= material.alphaCutoff) discard;
        }
        if (material.metallicRoughnessHandle) {
            Texture2D<float3> tRoughnessMetallic = GetTexture<Texture2D<float3> >(material.metallicRoughnessHandle);
            float2 roughnessMetallic = tRoughnessMetallic.Sample(AnisotropicSampler, psInput.TexCoord).gb;
            roughness *= roughnessMetallic.x;
            metallic  *= roughnessMetallic.y;
        }
        if (material.occlusionHandle) {
            Texture2D<float> tOcclusion = GetTexture<Texture2D<float> >(material.occlusionHandle);
            occlusion = tOcclusion.Sample(AnisotropicSampler, psInput.TexCoord).r;
            occlusion = lerp(1.0, occlusion, material.occlusionStrength);
        }
        if (material.normalHandle) {
            Texture2D<float3> tNormal = GetTexture<Texture2D<float3> >(material.normalHandle);
            float3 tangentNormal = mad(tNormal.Sample(AnisotropicSampler, psInput.TexCoord).rgb, 2.0, -1.0);
            tangentNormal = normalize(tangentNormal * float3(material.normalScale, material.normalScale, 1.0));

            float3 N = normalize(psInput.FragNormal);
            float3 T = normalize(psInput.Tangent);
            float3 B = normalize(psInput.Bitangent);
            float3x3 TBN = float3x3(T, B, N);
            normal = mul(tangentNormal, TBN);
        }
    }

    PSOutput output;
    output.GNormal = float4(normalize(normal), 0);
    output.GAlbedo = baseColor;
    output.GORM    = float4(occlusion, clamp(roughness, 1e-4, 1.0), metallic, 0);

    return output;
}
