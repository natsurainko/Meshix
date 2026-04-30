#include "../structures.h"
#include "../pbr.hlsli"

// ====================================================
//                    Vertex Shader
// ====================================================

struct VSInput {
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};

VSOutput VSMain(VSInput vsInput) {
    VSOutput output;
    output.Position = float4(vsInput.Position, 1.0);
    output.TexCoord = vsInput.TexCoord;
    return output;
}

// ====================================================
//                    Pixel Shader
// ====================================================

ConstantBuffer<FrameConstants> frameConstants : register(b0);
ConstantBuffer<LightConstants> lightConstants : register(b1);

SamplerState LinearSampler : register(s0);

Texture2D<float4> gPositionDepth    : register(t0);
Texture2D<float4> gNormalRoughness  : register(t1);
Texture2D<float4> gAlbedoMetallic   : register(t2);
Texture2D<float>  gMaterialAo       : register(t3);
Texture2D<float>  gAmbientOcclusion : register(t4);
Texture2D<float>  gShadow           : register(t5);

float3 ACESFilmic(float3 x) {
    return saturate((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14));
}

float4 PSMain(VSOutput psInput) : SV_TARGET0 {
    float4 positionDepth = gPositionDepth.Sample(LinearSampler, psInput.TexCoord);
    if (positionDepth.a <= 0) discard;
    float3 fragPos = positionDepth.xyz;

    float4 normalRoughness = gNormalRoughness.Sample(LinearSampler, psInput.TexCoord);
    float4 albedoMetallic  = gAlbedoMetallic.Sample(LinearSampler, psInput.TexCoord);
    float  materialAO      = gMaterialAo.Sample(LinearSampler, psInput.TexCoord);
    float  HBAO            = gAmbientOcclusion.Sample(LinearSampler, psInput.TexCoord);
    float  shadow          = gShadow.Sample(LinearSampler, psInput.TexCoord).r;

    float3 albedo    = albedoMetallic.rgb;
    float  metallic  = albedoMetallic.a;
    float  roughness = normalRoughness.w;
    float  ao        = materialAO * HBAO;

    float3 N  = normalize(normalRoughness.xyz);
    float3 V  = normalize(frameConstants.CameraPosition.xyz - fragPos);
    float  NdotV = max(dot(N, V), 0.0);

    float3 Lo = float3(0.0, 0.0, 0.0);
    // for (.. lights) ..
    {
        // if (light is directional light)
        {
            Lo += PBRLighting(NdotV,
                              N,
                              V,
                              normalize(-lightConstants.LightDirection.xyz),
                              lightConstants.LightColor * lightConstants.LightIntensity,
                              albedo,
                              roughness,
                              metallic);
        }

        /* if (light is point light)
        {
            float  L           = light.Position - fragPos;
            float  distance    = length(L);
            float  attenuation = 1.0 / (distance * distance);
            float3 radiance    = light.LightColor * (light.LightIntensity * attenuation);

            Lo += PBRLighting(NdotV,
                              N,
                              V,
                              normalize(L),
                              F0,
                              radiance,
                              roughness,
                              metallic,
                              albedo);
        }*/
    }

    // ambient lighting (note that the next IBL tutorial will replace
    // this ambient lighting with environment lighting).
    float3 ambient = (lightConstants.AmbientIntensity * ao) * albedo;
    float3 color   = mad(Lo, shadow, ambient);

    // HDR Tone Mapping
    // color = color / (color + float3(1.0, 1.0, 1.0));

    // ACES Filmic Tone Mapping
    color = ACESFilmic(color);

    // using sRGB rtvs to skip gamma correct
    // color = pow(color, gammaCorrection);

    return float4(color, 1.0);
}
