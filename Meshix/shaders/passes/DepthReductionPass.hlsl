// ====================================================
//                  Compute Shader
// ====================================================

Texture2D<float4> gPositionDepth : register(t0);

RWByteAddressBuffer DepthBoundsBuffer : register(u0);

#define GROUP_SIZE     64
#define MAX_WAVE_COUNT 8

groupshared uint gsMinDepth[MAX_WAVE_COUNT];
groupshared uint gsMaxDepth[MAX_WAVE_COUNT];

[numthreads(8, 8, 1)]
void CSMain(uint3 threadId      : SV_DispatchThreadID,
            uint3 groupId       : SV_GroupID,
            uint3 groupThreadId : SV_GroupThreadID)
{
    uint  waveLaneCount = WaveGetLaneCount();
    uint2 pixel         = threadId.xy * 4; // Sampling 1/4 of the screen pixels

    float linearDepth = gPositionDepth.Load(int3(pixel, 0)).w;
    float minDepthPre;
    float maxDepthPre;

    if (linearDepth <= 0) {
        minDepthPre = asfloat(0x7F7FFFFF);
        maxDepthPre = 0;
    } else {
        minDepthPre = linearDepth;
        maxDepthPre = linearDepth;
    }

    float waveMin = WaveActiveMin(minDepthPre);
    float waveMax = WaveActiveMax(maxDepthPre);

    uint flatIndex        = groupThreadId.y * 8 + groupThreadId.x;
    uint waveCountInGroup = (GROUP_SIZE + waveLaneCount - 1) / waveLaneCount;
    uint waveIndexInGroup = flatIndex / waveLaneCount;

    if (WaveIsFirstLane()) {
        gsMinDepth[waveIndexInGroup] = asuint(waveMin);
        gsMaxDepth[waveIndexInGroup] = asuint(waveMax);
    }

    GroupMemoryBarrierWithGroupSync();

    if (flatIndex < waveCountInGroup) {
        float tileMin = asfloat(gsMinDepth[flatIndex]);
        float tileMax = asfloat(gsMaxDepth[flatIndex]);

        tileMin = WaveActiveMin(tileMin);
        tileMax = WaveActiveMax(tileMax);

        if (WaveIsFirstLane()) {
            DepthBoundsBuffer.InterlockedMin(0, asuint(tileMin));
            DepthBoundsBuffer.InterlockedMax(4, asuint(tileMax));
        }
    }
}
