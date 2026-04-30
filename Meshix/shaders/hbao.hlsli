#ifndef HBAO_Techniques
#define HBAO_Techniques

#ifndef HBAO_NUM_DIRECTIONS
#define HBAO_NUM_DIRECTIONS 8
#endif

#ifndef HBAO_NUM_STEPS
#define HBAO_NUM_STEPS 4
#endif

#ifndef HBAO_Radius
#define HBAO_Radius 0.5
#endif

#ifndef HBAO_Bias
#define HBAO_Bias 0.05
#endif

#ifndef HBAO_Multiplier
#define HBAO_Multiplier 1.0
#endif

#ifndef HBAO_Power
#define HBAO_Power 2.0
#endif

#define _2_PI 6.28318530718f

float2 RotateVector(float2 v, float c, float s) {
    return float2(v.x * c - v.y * s, v.x * s + v.y * c);
}

float3 ReconstructViewPosition(float2 texCoord, float linearDepth, float4x4 projection) {
    float ndcX = mad(texCoord.x, 2.0, -1.0);
    float ndcY = mad(texCoord.y, -2.0, 1.0);

    return float3(
        ndcX / projection._11 * linearDepth,
        ndcY / projection._22 * linearDepth,
        -linearDepth
    );
}

float Falloff(float distSq, float negInvR2) {
    return saturate(distSq * negInvR2 + 1.0f);
}

float ComputeAO(float3 P, float3 N, float3 S, float negInvR2) {
    float3 V      = S - P;
    float  distSq = dot(V, V);
    float  NdotV  = dot(N, V) * rsqrt(max(distSq, 1e-6f));
    return saturate(NdotV - HBAO_Bias) * Falloff(distSq, negInvR2);
}

float GetRadiusPixels(float linearDepth, float4x4 projection, float resolutionX) {
    float focalLen     = projection._11;
    float radiusPixels = HBAO_Radius * focalLen / linearDepth
                       * resolutionX * 0.5f;

    return min(radiusPixels, 128.0f);
}

float HBAO(float3 viewPos,
           float3 viewNormal,
           float2 texCoord,
           float radiusPixels,
           Texture2D<float4> gPostionDepth,
           SamplerState pointSampler,
           float4x4 projection,
           float2 frameResolutionInverse,
           float2 noisePixelPos) {
    float stepSizePixels = radiusPixels / float(HBAO_NUM_STEPS + 1);
    float negativeInvR2  = -1.0f / (HBAO_Radius * HBAO_Radius);

    float angle      = InterleavedGradientNoise(noisePixelPos);
    float angleCos   = cos(angle);
    float angleSin   = sin(angle);
    float stepJitter = frac(angle * 7.3456789f);

    float AO = 0.0f;
    [unroll]
    for (int i = 0; i < HBAO_NUM_DIRECTIONS; ++i) {
        float  baseAngle = _2_PI * i / float(HBAO_NUM_DIRECTIONS);
        float2 baseDir   = float2(cos(baseAngle), sin(baseAngle));
        float2 dir       = RotateVector(baseDir, angleCos, angleSin);

        float rayPixels = stepJitter * stepSizePixels + 1.0f;
        [unroll]
        for (int j = 0; j < HBAO_NUM_STEPS; ++j) {
            float2 sampleUV      = saturate(round(rayPixels * dir) * frameResolutionInverse + texCoord);
            float  sampleDepth   = gPostionDepth.SampleLevel(pointSampler, sampleUV, 0).w;

            if (sampleDepth > 0.0f) {
                float3 sampleViewPos = ReconstructViewPosition(sampleUV, sampleDepth, projection);
                AO += ComputeAO(viewPos, viewNormal, sampleViewPos, negativeInvR2);
            }

            rayPixels += stepSizePixels;
        }
    }

    AO *= HBAO_Multiplier / float(HBAO_NUM_DIRECTIONS * HBAO_NUM_STEPS);
    return pow(saturate(1.0f - AO * 2.0f), HBAO_Power);
}

#endif // HBAO_Techniques
