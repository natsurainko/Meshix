#include "../structures.h"
#include "../noise.hlsli"
#include "../csm.hlsli"

#define POISSON_16
#include "../poisson.hlsli"
#include "../pcss.hlsli"

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

ConstantBuffer<LightConstants>         lightConstants         : register(b0);
ConstantBuffer<CascadeShadowConstants> cascadeShadowConstants : register(b1);

cbuffer Constants : register(b2)
{
    float2 ShadowMapTexelSize;
}

SamplerState           NearestSampler      : register(s0);

Texture2D<float4>     gPositionDepth   : register(t0);
Texture2D<float4>     gNormalRoughness : register(t1);
Texture2DArray<float> ShadowMap        : register(t2);

// static const float kBias[CASCADE_NUM] = { 3.5, 1.5, 1.0, 1.0, 1.0 };

// Jitter shadow depth bias.
// Ref: https://github.com/qiutang98/chord/blob/408900645557b1ef3a9a9987eb24d099a47659c8/install/resource/shader/pcss.hlsl#L256
float CalculateShadowBias(float NdotL) {
	float f1 = saturate(1.0 - NdotL);
	float f2 = f1 * f1;
	float f4 = f2 * f2;
	float f8 = f4 * f4;

	// float j_bias_const = 1e-5f * lerp(pushConsts.biasLerpMin_const, pushConsts.biasLerpMax_const, n_01);
	// pushConsts.biasLerpMin_const = 5.0;
	// pushConsts.biasLerpMax_const = 10.0;

	float  j_bias_const = 5e-5f * lerp(5.0, 10.0, 0.0); // 1e-5f * lerp(5.0, 10.0, 0.0);
	return j_bias_const + f4 * j_bias_const + (f8 * f4) * j_bias_const;
}

// Ref: https://github.com/qiutang98/chord/blob/408900645557b1ef3a9a9987eb24d099a47659c8/install/resource/shader/pcss.hlsl#L61
// Surface normal based bias, see https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps for more details.
/*
float3 GetBiasNormalOffset(float NdotL, float3 normal) {
    return 2.0 * ShadowMapTexelSize.x * saturate(1.0f - NdotL) * normal;
}

float3 GetBiasNormalOffset(float3 N, float NdotL) {
	return N * clamp(1.0f - NdotL, 0.0f, 1.0f) * ShadowMapTexelSize.x * 10.0f;
}
*/

float PSMain(VSOutput psInput) : SV_TARGET0 {
    float4 positionDepth = gPositionDepth.SampleLevel(NearestSampler, psInput.TexCoord, 0);
    if (positionDepth.w <= 0) return 0;

	uint        layer;
	float4      shadowCoords;
	CascadeData cascadeData;
    FastSelectCascadeLayer(positionDepth.w, cascadeShadowConstants.CascadeDatas, layer, cascadeData);

    float3 normal = gNormalRoughness.SampleLevel(NearestSampler, psInput.TexCoord, 0).rgb;
	float  NdotL  = clamp(dot(normal, -lightConstants.LightDirection.xyz), 0.0, 1.0);

	float  pcfBias = CalculateShadowBias(NdotL); // * kBias[layer];
	// float3 normalOffset = GetBiasNormalOffset(normal, NdotL);

	// positionDepth.xyz += normalOffset;
	SelectCascadeLayer(positionDepth, cascadeShadowConstants.CascadeDatas, layer, shadowCoords, cascadeData);

	shadowCoords.z -= (1e-4f + pcfBias) / cascadeData.RadiusScale * cascadeData.ZStartBiasScale;

    return PCSS(ShadowMap,
                NearestSampler,
                layer,
                shadowCoords.xyz,
				cascadeData.DepthConvertToView.xy,
                pcfBias,
				cascadeData.RadiusScale,
				cascadeData.ShadowMapTexelSize,
                psInput.Position.xy);
}
