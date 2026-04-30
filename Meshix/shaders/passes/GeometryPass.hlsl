#include "../structures.h"

// ====================================================
//                    Vertex Shader
// ====================================================

ConstantBuffer<FrameConstants>  frameConstants  : register(b0);
ConstantBuffer<ObjectConstants> objectConstants : register(b1);

struct VSInput {
    float3 Position  : POSITION;
    float3 Normal    : NORMAL;
    float2 TexCoord  : TEXCOORD;
    float3 Tangent   : TANGENT;
    float3 Bitangent : BINORMAL;
};

struct VSOutput {
    float4 FragPos    : POSITION;
    float3 FragNormal : NORMAL;
    float3 Tangent    : TANGENT0;
    float3 Bitangent  : TANGENT1;
    float2 TexCoord   : TEXCOORD;
    float4 Position   : SV_POSITION;
};

VSOutput VSMain(VSInput vsInput) {
    float4 worldPos = mul(objectConstants.World, float4(vsInput.Position, 1.0f));
    float4 viewPos  = mul(frameConstants.View, worldPos);

    VSOutput output;
    output.FragPos    = float4(worldPos.xyz, -viewPos.z);
    output.FragNormal = mul((float3x3)objectConstants.WorldInverseTranspose, vsInput.Normal);
    output.Tangent    = mul((float3x3)objectConstants.WorldInverseTranspose, vsInput.Tangent);
    output.Bitangent  = mul((float3x3)objectConstants.WorldInverseTranspose, vsInput.Bitangent);
    output.TexCoord   = vsInput.TexCoord;
    output.Position   = mul(frameConstants.Projection, viewPos);
    return output;
}

// ====================================================
//                    Pixel Shader
// ====================================================

uint MaterialHandle : register(b0);

SamplerState AnisotropicSampler : register(s0);

StructuredBuffer<MaterialConstants> materialConstants : register(t0);

// 1-based handle
template<typename T>
T GetTexture(uint handle) {
    return ResourceDescriptorHeap[handle - 1];
}

// 1-based handle
MaterialConstants GetMaterial(uint handle) {
    return materialConstants[handle - 1];
}

struct PSOutput {
    float4 GPositionDepth   : SV_Target0;
    float4 GNormalRoughness : SV_Target1;
    float4 GAlbedoMetallic  : SV_Target2;
    float  GAo              : SV_Target3;
};

PSOutput PSMain(VSOutput psInput) {
    float3 normal    = psInput.FragNormal;
    float4 baseColor = float4(1.0, 1.0, 1.0, 1.0);
    float  metallic  = 0;
    float  roughness = 0.5;
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
            Texture2D<float3> tMetallicRoughness = GetTexture<Texture2D<float3> >(material.metallicRoughnessHandle);
            float2 metallicRoughness = tMetallicRoughness.Sample(AnisotropicSampler, psInput.TexCoord).gb;
            roughness *= metallicRoughness.x;
            metallic  *= metallicRoughness.y;
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
    output.GPositionDepth   = psInput.FragPos;
    output.GNormalRoughness = float4(normalize(normal), clamp(roughness, 1e-4, 1.0));
    output.GAlbedoMetallic  = float4(baseColor.rgb, metallic);
    output.GAo              = occlusion;

    return output;
}
