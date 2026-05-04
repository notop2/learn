#ifndef __STORAGE_H
#define __STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define STORAGE_MAGIC       0x454E5253
#define STORAGE_VERSION     0x0001

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t crc;

    uint8_t  fan_mode;
    int32_t  fan_speed;
    float    temp_offset;
    float    humid_offset;
    uint16_t light_threshold;

    uint8_t  reserved[32];
    uint32_t save_count;
} SystemParams_t;

bool STORAGE_Load(SystemParams_t *params);
bool STORAGE_Save(const SystemParams_t *params);
void STORAGE_SetDefaults(SystemParams_t *params);
uint16_t STORAGE_CalcCRC(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
