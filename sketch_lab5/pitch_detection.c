#include "pitch_detection.h"
#include <math.h>

static float threshold_yin = 0.15f;

void pitch_init(float threshold) {
    threshold_yin = threshold;
}

float pitch_detect(const uint8_t *buffer) {
    static float delta[BUF_LEN];
    static float cumulative[BUF_LEN];

    // Compute difference function
    delta[0] = 1.0f;
    for (int lag = 1; lag < BUF_LEN; ++lag) {
        float sumSq = 0.0f;
        for (int i = 0; i + lag < BUF_LEN; ++i) {
            float d = (float)buffer[i] - (float)buffer[i + lag];
            sumSq += d * d;
        }
        delta[lag] = sumSq;
    }

    // Cumulative mean normalized difference
    cumulative[0] = 1.0f;
    float sumAcc = 0.0f;
    for (int lag = 1; lag < BUF_LEN; ++lag) {
        sumAcc += delta[lag];
        cumulative[lag] = delta[lag] * lag / sumAcc;
    }

    // Absolute threshold
    int lagEstimate = -1;
    for (int lag = 1; lag < BUF_LEN; ++lag) {
        if (cumulative[lag] < threshold_yin) {
            lagEstimate = lag;
            while (lag + 1 < BUF_LEN && cumulative[lag + 1] < cumulative[lag]) {
                lag++;
            }
            lagEstimate = lag;
            break;
        }
    }
    if (lagEstimate < 1) return 0.0f;

    // Parabolic interpolation
    float valPrev = (lagEstimate > 0) ? cumulative[lagEstimate - 1] : cumulative[lagEstimate];
    float valCur  = cumulative[lagEstimate];
    float valNext = (lagEstimate + 1 < BUF_LEN) ? cumulative[lagEstimate + 1] : valCur;
    float denominator = 2 * (2 * valCur - valPrev - valNext);
    float offset = (denominator == 0) ? 0.0f : (valPrev - valNext) / denominator;
    float lagInterp = lagEstimate + offset;

    return SAMPLE_RATE / lagInterp;
}