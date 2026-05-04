#ifndef __FILTER_H
#define __FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define MOVING_AVG_WINDOW  8

typedef struct {
    float buffer[MOVING_AVG_WINDOW];
    uint8_t index;
    uint8_t count;
    float sum;
} MovingAvgFilter_t;

void MovingAvg_Init(MovingAvgFilter_t *filt, float initial_value);
float MovingAvg_Update(MovingAvgFilter_t *filt, float new_sample);
void MovingAvg_Reset(MovingAvgFilter_t *filt, float value);

typedef struct {
    float output;
    float alpha;
} LowPassFilter_t;

void LowPass_Init(LowPassFilter_t *filt, float alpha, float initial);
float LowPass_Update(LowPassFilter_t *filt, float new_sample);

#ifdef __cplusplus
}
#endif

#endif
