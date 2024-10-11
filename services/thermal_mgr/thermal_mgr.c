#include "thermal_mgr.h"
#include "errors.h"
#include "lm75bd.h"
#include "console.h"
#include "logging.h"

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
#define THERMAL_MGR_QUEUE_TIMEOUT (pdMS_TO_TICKS(100))  // 100 ms timeout

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
    // Check if the event pointer is valid
    if (event == NULL) {
        return ERR_CODE_INVALID_ARG;  
    }

    // Check if the thermal manager queue handle is valid
    if (thermalMgrQueueHandle == NULL) {
        return ERR_CODE_INVALID_STATE;  
    }

    // Attempt to send the event to the thermal manager queue
    if (xQueueSend(thermalMgrQueueHandle, event, THERMAL_MGR_QUEUE_TIMEOUT) == pdPASS) {
        return ERR_CODE_SUCCESS;  

    // Return error if sending the event fails
    return ERR_CODE_QUEUE_FULL;  
    }
}

void osHandlerLM75BD(void) {
    error_code_t errCode; 

    // Create a new event
    thermal_mgr_event_t event;
    event.type = THERMAL_MGR_EVENT_OS_INTERRUPT; 

    // Log the error if sending the event fails
    LOG_IF_ERROR_CODE(thermalMgrSendEvent(&event)); 
}

static void thermalMgr(void *pvParameters) {
    error_code_t errCode; 
    lm75bd_config_t *config = (lm75bd_config_t *)pvParameters;

    while (1) {
        thermal_mgr_event_t event;

        // Wait for the next event from the queue
        if (xQueueReceive(thermalMgrQueueHandle, &event, portMAX_DELAY) == pdPASS) {
            // Check if the event type is MEASURE_TEMP_CMD
            if (event.type == THERMAL_MGR_EVENT_MEASURE_TEMP_CMD) {
                // Variable to store the current temperature
                float temperature;

                // Read the current temperature from the LM75BD sensor
                errCode = readTempLM75BD(config->devAddr, &temperature); 
                LOG_IF_ERROR_CODE(errCode); 

                // Check for read temperature error and handle accordingly
                RETURN_IF_ERROR_CODE(errCode);

                // Check temperature against thresholds
                if (temperature > 80.0f) {
                    // Over-temperature condition detected
                    overTemperatureDetected();
                } else if (temperature < 75.0f) {
                    // Safe operating conditions restored
                    safeOperatingConditions();
                }

                // Send the measured temperature as telemetry
                addTemperatureTelemetry(temperature);
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





