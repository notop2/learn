#include "filter.h"

void MovingAvg_Init(MovingAvgFilter_t *filt, float initial_value)
{
    filt->index = 0;
    filt->count = 0;
    filt->sum = initial_value * MOVING_AVG_WINDOW;
    for (uint8_t i = 0; i < MOVING_AVG_WINDOW; i++) {
        filt->buffer[i] = initial_value;
    }
}

float MovingAvg_Update(MovingAvgFilter_t *filt, float new_sample)
{
    filt->sum -= filt->buffer[filt->index];
    filt->buffer[filt->index] = new_sample;
    filt->sum += new_sample;

    filt->index = (filt->index + 1) % MOVING_AVG_WINDOW;

    if (filt->count < MOVING_AVG_WINDOW) {
        filt->count++;
    }

    return filt->sum / (float)filt->count;
}

void MovingAvg_Reset(MovingAvgFilter_t *filt, float value)
{
    MovingAvg_Init(filt, value);
}

void LowPass_Init(LowPassFilter_t *filt, float alpha, float initial)
{
    filt->alpha = alpha;
    filt->output = initial;
}

float LowPass_Update(LowPassFilter_t *filt, float new_sample)
{
    filt->output = filt->alpha * new_sample + (1.0f - filt->alpha) * filt->output;
    return filt->output;
}
