#ifndef PITCH_DETECTION_H
#define PITCH_DETECTION_H
#include <stdint.h>

#define BUF_LEN         512
#define SAMPLE_RATE     19000

void pitch_init(float threshold); // Configure YIN threshold
float pitch_detect(const uint8_t *buffer); // Perform pitch detection on buffer

#endif