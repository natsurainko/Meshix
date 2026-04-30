#ifndef PCSS_Techniques
#define PCSS_Techniques

#ifndef BLOCKER_SEARCH_NUM_SAMPLES
#define BLOCKER_SEARCH_NUM_SAMPLES 16
#endif

#ifndef BLOCKER_SEARCH_MAX_RANGE_SCALE
#define BLOCKER_SEARCH_MAX_RANGE_SCALE 0.125f
#endif

#ifndef PCF_BIAS_SCALE
#define PCF_BIAS_SCALE 20.0
#endif

#ifndef PCF_NUM_SAMPLES_MIN
#define PCF_NUM_SAMPLES_MIN 4
#endif

#ifndef PCF_NUM_SAMPLES_MAX
#define PCF_NUM_SAMPLES_MAX 32
#endif

#ifndef PCSS_SOFTNESS
#define PCSS_SOFTNESS 0.075f
#endif

#ifndef LIGHT_SIZE
#define LIGHT_SIZE 10.0
#endif

float2 RotateVector(float2 v, float2 trig) {
    float c = trig.x;
    float s = trig.y;
    return float2(v.x * c - v.y * s, v.x * s + v.y * c);
}

float2 VogelDiskSample(uint sampleIndex, uint sampleCount, float angle) {
    const float goldenAngle = 2.399963f;
    const float r = sqrt(sampleIndex + 0.5f) / sqrt(sampleCount);

    const float theta = sampleIndex * goldenAngle + angle;
    float sine, cosine;
    sincos(theta, sine, cosine);

    return float2(cosine, sine) * r;
}

float GetMinPercentage() {
    // blockerSearchMaxRangeScale / lightSize;
    return BLOCKER_SEARCH_MAX_RANGE_SCALE / LIGHT_SIZE;
}

float GetPcssWidth(float radiusScale, float shadowMapTexelSize) {
    // lightSize * radiusScale * 64.0 * shadowMapTexelSize
    return LIGHT_SIZE * radiusScale * 64.0 * shadowMapTexelSize;
}

float2 FindBlocker(
    Texture2DArray<float> tDepthMap,
    SamplerState          pointSampler,
    float3 shadowCoords,
    uint   layer,
    float  searchWidth,
    float2 angleTrig)
{
    float blockerSum  = 0;
    float numBlockers = 0;

    [loop]
    for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) {
        float2 offset         = POISSON_DISK[i] * searchWidth;
        float2 rotatedOffset  = RotateVector(offset, angleTrig);
        float  shadowMapDepth = tDepthMap.Sample(pointSampler, float3(shadowCoords.xy + rotatedOffset, layer));
        if (shadowMapDepth < shadowCoords.z) {
            blockerSum += shadowMapDepth;
            ++numBlockers;
        }
    }

    float avgBlockerDepth = blockerSum / numBlockers; // blockerSum > 0 ? blockerSum / numBlockers : 0;
    return float2(numBlockers, avgBlockerDepth);
}

// Ref: https://github.com/qiutang98/chord/blob/408900645557b1ef3a9a9987eb24d099a47659c8/install/resource/shader/cascade_shadow_projection.hlsli#L8
// Use cache occluder dist to fit one curve similar to tonemapper, to get some effect like pcss.
// can reduce tiny acne natively.
float ContactHardenPCFKernal(
	float occluders,
	float occluderDistSum,
	float compareDepth,
	uint  shadowSampleCount)
{
    // Normalize occluder dist.
    float occluderAvgDist = occluderDistSum / occluders;
    float w = 1.0f / float(shadowSampleCount);

    // 0 -> contact harden.
    // 1 -> soft, full pcf.
    float pcfWeight =  clamp(occluderAvgDist / compareDepth, 0.0, 1.0);

    // Normalize occluders.
    float percentageOccluded = clamp(occluders * w, 0.0, 1.0);

    // S curve fit.
    percentageOccluded = 2.0f * percentageOccluded - 1.0f;
    float occludedSign = sign(percentageOccluded);
    percentageOccluded = 1.0f - (occludedSign * percentageOccluded);
    percentageOccluded = lerp(percentageOccluded * percentageOccluded * percentageOccluded, percentageOccluded, pcfWeight);
    percentageOccluded = 1.0f - percentageOccluded;

    percentageOccluded *= occludedSign;
    percentageOccluded = 0.5f * percentageOccluded + 0.5f;

    return 1.0f - percentageOccluded;
}

float PCF_Filter(
    Texture2DArray<float> tDepthMap,
	SamplerState          pointSampler,
    float3 shadowCoords,
    uint   layer,
    float  filterRadiusUV,
    float  penumbraRatio,
	float  shadowMapTexelSize,
    float  pcfBias,
    float  angle)
{
	float sampleTexelDim    = filterRadiusUV / shadowMapTexelSize;
	float sampleRate        = (min(sampleTexelDim, PCF_NUM_SAMPLES_MAX + 1.0) - 1.0) / PCF_NUM_SAMPLES_MAX;
	uint  filterSampleCount = uint(lerp(float(PCF_NUM_SAMPLES_MIN), float(PCF_NUM_SAMPLES_MAX), sampleRate));

	float occluders       = 0.0f;
    float occluderDistSum = 0.0f;
    uint  pixelCount      = filterSampleCount * 4;

    [loop]
    for (uint i = 0; i < filterSampleCount; ++i) {
        float2 randSample    = VogelDiskSample(i, filterSampleCount, angle);
        float2 sampleUV      = shadowCoords.xy + randSample * filterRadiusUV;
        float  lengthRatio   = penumbraRatio * length(randSample);

        float  biasPCF       = lerp(0.0f, pcfBias * PCF_BIAS_SCALE, lengthRatio);
		float  compareDepth  = shadowCoords.z - biasPCF;
        float4 depth4        = tDepthMap.Gather(pointSampler, float3(sampleUV, layer));

		[unroll]
		for (uint j = 0; j < 4; ++j) {
            float dist        = depth4[j] - compareDepth;
            float isOccluded  = 1.0f - step(0.0f, dist);

            occluders        += isOccluded;
            occluderDistSum  += (-dist) * isOccluded;
        }

        // sum += tDepthMap.SampleCmpLevelZero(comparisonSampler, float3(sampleUV, layer), shadowCoords.z - biasPCF);
    }

 	// return 1.0 - occluders / float(pixelCount);
	return ContactHardenPCFKernal(occluders, occluderDistSum, shadowCoords.z, pixelCount);
    // return sum / filterSampleCount;
}

float PCSS(Texture2DArray<float> tDepthMap,
           SamplerState          pointSampler,
           uint   layer,
           float3 shadowCoords,
           float2 depthToEye,
           float  pcfBias,
           float  radiusScale,
           float  shadowMapTexelSize,
           float2 noisePixelPos)
{
    float  angle     = InterleavedGradientNoise(noisePixelPos);
    float2 angleTrig = float2(cos(angle), sin(angle));

    float pcssWidth     = GetPcssWidth(radiusScale, shadowMapTexelSize);
    // float minPercentage = GetMinPercentage();

    // STEP 1: blocker search
    float  findBlockerWidth  = BLOCKER_SEARCH_MAX_RANGE_SCALE * pcssWidth;
    float2 findBlockerResult = FindBlocker(
		tDepthMap,
		pointSampler,
    	shadowCoords,
		layer,
		findBlockerWidth,
		angleTrig);
    
    float numBlockers     = findBlockerResult.x;
    float avgBlockerDepth = findBlockerResult.y;

    float blocker   = (avgBlockerDepth - depthToEye.y) / depthToEye.x;
    float zReceiver = (shadowCoords.z - depthToEye.y) / depthToEye.x;

    // There are no occluders so early out (this saves filtering)
    [branch]
    if (numBlockers < 1) return 1.0f;
    // [branch]
    // if (numBlockers >= BLOCKER_SEARCH_NUM_SAMPLES) return 0.0f;

    // STEP 2: penumbra size
    float penumbraRatio = saturate((zReceiver - blocker) / blocker) * PCSS_SOFTNESS;
    // penumbraRatio = min(penumbraRatio, minPercentage);
    float filterRadiusUV = max(penumbraRatio * pcssWidth, shadowMapTexelSize);

    // STEP 3: filtering
    return PCF_Filter(
		tDepthMap,
		pointSampler,
    	shadowCoords,
		layer,
		filterRadiusUV,
		penumbraRatio,
    	shadowMapTexelSize,
		pcfBias,
		angle);
}

#endif // PCSS_Techniques
