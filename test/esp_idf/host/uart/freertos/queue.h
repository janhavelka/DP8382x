#pragma once

#include "freertos/FreeRTOS.h"

typedef void* QueueHandle_t;

#ifdef __cplusplus
extern "C" {
#endif
BaseType_t xQueueReceive(QueueHandle_t queue, void* value, TickType_t wait);
#ifdef __cplusplus
}
#endif
