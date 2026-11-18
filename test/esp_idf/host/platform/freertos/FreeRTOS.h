#pragma once
/* Scheduling boundary only: SDK Ethernet/event/timer interfaces are unmodified. */
#include "sdkconfig.h"
#include <stdint.h>
#include <stddef.h>
typedef uint32_t TickType_t;
typedef int BaseType_t;
typedef unsigned UBaseType_t;
#define pdMS_TO_TICKS(ms) ((TickType_t)(((uint64_t)(ms) * CONFIG_FREERTOS_HZ) / 1000U))
#define pdTRUE 1
#define pdFALSE 0
#define configTICK_RATE_HZ CONFIG_FREERTOS_HZ
#define portMAX_DELAY UINT32_MAX
#define portTICK_PERIOD_MS (1000 / CONFIG_FREERTOS_HZ)
