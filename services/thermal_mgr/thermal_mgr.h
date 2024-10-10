#pragma once

#include "lm75bd.h"
#include "errors.h"

#include <FreeRTOS.h>
#include <os_task.h>
#include <os_queue.h>

// Thermal Manager Event Types
typedef enum {
  THERMAL_MGR_EVENT_MEASURE_TEMP_CMD,
  THERMAL_MGR_EVENT_OVER_TEMP_DETECTED,  // Added missing event
  THERMAL_MGR_EVENT_SAFE_CONDITIONS      // Added missing event
} thermal_mgr_event_type_t;

// Thermal Manager Event Structure
typedef struct {
  thermal_mgr_event_type_t type;
} thermal_mgr_event_t;

// Thermal Manager Configuration Structure
typedef struct {
  lm75bd_config_t sensorConfig;
} thermal_mgr_config_t;

// Constants for temperature thresholds
#define THERMAL_OVER_TEMP_THRESHOLD_CELSIUS 75.0  // Example threshold
#define THERMAL_HYSTERESIS_THRESHOLD_CELSIUS 70.0 // Example threshold

// External declaration of thermal manager queue and task handles
extern QueueHandle_t thermalMgrQueueHandle;
extern TaskHandle_t thermalMgrTaskHandle;

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations
void initThermalSystemManager(lm75bd_config_t *config);
error_code_t thermalMgrSendEvent(thermal_mgr_event_t *event);
void addTemperatureTelemetry(float tempC);
void overTemperatureDetected(void);
void safeOperatingConditions(void);
void osHandlerLM75BD(void); // New function added

#ifdef __cplusplus
}
#endif

