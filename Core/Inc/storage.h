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
    uint32_t magic;              /* 魔数，用于标识 Flash 中是否有有效数据 */
    uint16_t version;            /* 参数结构体版本号 */
    uint16_t crc;                /* CRC16 校验和 */

    uint8_t  fan_mode;           /* 风扇模式：0=自动，1=手动 */
    int32_t  fan_speed;          /* 手动模式下的风扇速度 */
    float    temp_offset;        /* 温度校准偏移量 */
    float    humid_offset;       /* 湿度校准偏移量 */
    uint16_t light_threshold;    /* 光照阈值 */

    uint8_t  reserved[32];       /* 保留字段，方便后续扩展 */
    uint32_t save_count;         /* 写入次数统计 */
} SystemParams_t;

bool STORAGE_Load(SystemParams_t *params);
bool STORAGE_Save(const SystemParams_t *params);
void STORAGE_SetDefaults(SystemParams_t *params);
uint16_t STORAGE_CalcCRC(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
