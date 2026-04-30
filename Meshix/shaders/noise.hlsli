#ifndef Noise_Techniques
#define Noise_Techniques

float InterleavedGradientNoise(float2 pixelPos) {
    return frac(52.9829189 * frac(0.06711056 * pixelPos.x + 0.00583715 * pixelPos.y)) * 6.2831;
}

#endif // Noise_Techniques
