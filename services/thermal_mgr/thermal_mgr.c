#include "thermal_mgr.h"
#include "errors.h"
#include "lm75bd.h"
#include "console.h"

#include <FreeRTOS.h>
#include <os_task.h>
#include <os_queue.h>

#include <string.h>

#define THERMAL_MGR_STACK_SIZE 256U

static TaskHandle_t thermalMgrTaskHandle;
static StaticTask_t thermalMgrTaskBuffer;
static StackType_t thermalMgrTaskStack[THERMAL_MGR_STACK_SIZE];

#define THERMAL_MGR_QUEUE_LENGTH 10U
#define THERMAL_MGR_QUEUE_ITEM_SIZE sizeof(thermal_mgr_event_t)

static QueueHandle_t thermalMgrQueueHandle;
static StaticQueue_t thermalMgrQueueBuffer;
static uint8_t thermalMgrQueueStorageArea[THERMAL_MGR_QUEUE_LENGTH * THERMAL_MGR_QUEUE_ITEM_SIZE];

static void thermalMgr(void *pvParameters);

void initThermalSystemManager(lm75bd_config_t *config) {
  memset(&thermalMgrTaskBuffer, 0, sizeof(thermalMgrTaskBuffer));
  memset(thermalMgrTaskStack, 0, sizeof(thermalMgrTaskStack));
  
  thermalMgrTaskHandle = xTaskCreateStatic(
    thermalMgr, "thermalMgr", THERMAL_MGR_STACK_SIZE,
    config, 1, thermalMgrTaskStack, &thermalMgrTaskBuffer);

  memset(&thermalMgrQueueBuffer, 0, sizeof(thermalMgrQueueBuffer));
  memset(thermalMgrQueueStorageArea, 0, sizeof(thermalMgrQueueStorageArea));

  thermalMgrQueueHandle = xQueueCreateStatic(
    THERMAL_MGR_QUEUE_LENGTH, THERMAL_MGR_QUEUE_ITEM_SIZE,
    thermalMgrQueueStorageArea, &thermalMgrQueueBuffer);

}

error_code_t thermalMgrSendEvent(thermal_mgr_event_t *event) {
  /* Send an event to the thermal manager queue */
  return xQueueSend(thermalManagerQueue, event, portMAX_DELAY);
  return ERR_CODE_SUCCESS;
}

void osHandlerLM75BD(void) {
  /* Implement this function */
  // Reading the current temperature from the sensor
  int16_t currentTemp = readTempLM75BD(sensorConfig.devAddr);


  // Create a thermal manager event
  thermal_mgr_event_t event;


  // Determine if we're in an over-temperature state or safe conditions
  if (currentTemp >= THERMAL_OVER_TEMP_THRESHOLD_CELSIUS) {
      // Set the event type to over-temperature detected
      event.type = THERMAL_MGR_EVENT_OVER_TEMP_DETECTED;
  } else if (currentTemp < THERMAL_HYSTERESIS_THRESHOLD_CELSIUS) {
      // Set the event type to safe operating conditions
      event.type = THERMAL_MGR_EVENT_SAFE_CONDITIONS;
  }


  // Send the event to the thermal manager queue (do not block in ISR)
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(thermalManagerQueue, &event, &xHigherPriorityTaskWoken);


  // Perform a context switch if necessary
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void thermalMgr(void *pvParameters) {
  /* Implement this task */

  // Extract the config struct from pvParameters
  thermal_mgr_config_t config = *(thermal_mgr_config_t *)pvParameters;
  thermal_mgr_event_t receivedEvent;
  
  while (1) {
     if (xQueueReceive(thermalManagerQueue, &receivedEvent, portMAX_DELAY) == pdTRUE) {
          // Check the event type
          if (receivedEvent.type == THERMAL_MGR_EVENT_MEASURE_TEMP_CMD) {
              // Measure the temperature using your driver function
              int16_t currentTemp = readTempLM75BD(config.devAddr);
             
              // Send the measured temperature as telemetry
              addTemperatureTelemetry(currentTemp);
          }
     }
    
  }
}

void addTemperatureTelemetry(float tempC) {
  printConsole("Temperature telemetry: %f deg C\n", tempC);
}

void overTemperatureDetected(void) {
  printConsole("Over temperature detected!\n");
}

void safeOperatingConditions(void) { 
  printConsole("Returned to safe operating conditions!\n");
}

