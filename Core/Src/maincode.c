#include <stdint.h>
#include "main.h"
#include "cmsis_os.h"
#include "maincode.h"
#include "queue.h"

#define MS_TO_TRANSMIT 1
#define DATA_PACK 45

TaskHandle_t hGenerateDataTask;
TaskHandle_t hTransmitDataTask;
TaskHandle_t hReciveDataTask;
QueueHandle_t queueHandler01;

HAL_StatusTypeDef UART_state;
uint8_t UART_data;

void start_rtos(void) {
	queueHandler01 = xQueueCreate(1, sizeof(uint8_t) * DATA_PACK);

	xTaskCreate(
		generateDataTask,
		"dataGeneratorTask",
		128,
		NULL,
		1,
		&hGenerateDataTask
	);

	xTaskCreate(
		transmitDataTask,
		"transmitDataTask",
		128,
		NULL,
		1,
		&hTransmitDataTask
	);

	vTaskStartScheduler();

	while(1);
}

void generateDataTask(void *arg) {
	uint8_t pinState = 0;
	uint8_t sampleData[DATA_PACK] = {
		0x00,
		'A', '1', 'A', '2', 'A', '3',
		'B', '1', 'B', '2', 'B', '3',
		'C', '1', 'C', '2', 'C', '3',
		'C', '4', 'C', '5', 'C', '6',
		'C', '7', 'C', '8', 'C', '9',
		'D', '1', 'D', '2', 'D', '3',
		'N', '1', 'N', '2', 'N', '3',
		'\r', '\n'
	};

	BaseType_t queueStatus;
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = pdMS_TO_TICKS(MS_TO_TRANSMIT);

	uint8_t packCounter = 0;
	uint8_t pack10Counter = 0;

	while(1) {
		pinState = !pinState;

		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, pinState);

		if(packCounter >= 1) {
			sampleData[31] += 1;
			sampleData[33] += 1;
			sampleData[35] += 1;
		} else {
			sampleData[31] = 'D';
			sampleData[33] = 'D';
			sampleData[35] = 'D';
		}

		if(packCounter >= 7) {
			if(pack10Counter == 0) {
				sampleData[0] |= 0b01000000;
				if(packCounter >= 8) {
					sampleData[37] += 1;
					sampleData[39] += 1;
					sampleData[41] += 1;
				} else {
					sampleData[37] = 'N';
					sampleData[39] = 'N';
					sampleData[41] = 'N';
				}
			}

		} else {
			sampleData[0] &= ~0b01000000;
			sampleData[37] = '*';
			sampleData[39] = '*';
			sampleData[41] = '*';
		}

		queueStatus = xQueueSend(queueHandler01, (void*)sampleData, xFrequency);
		vTaskDelayUntil( &xLastWakeTime, xFrequency );

		packCounter++;
		if(packCounter >= 10) {
			packCounter = 0;
			pack10Counter++;
			if(pack10Counter >= 10) pack10Counter = 0;
		}


	}
	vTaskDelete(hGenerateDataTask);
}

void transmitDataTask(void *arg) {
	uint8_t generatedData [DATA_PACK];
	BaseType_t queueStatus;
	TickType_t xLastWakeTime = xTaskGetTickCount();
	const TickType_t xFrequency = pdMS_TO_TICKS(MS_TO_TRANSMIT);

	while(1) {
		queueStatus = xQueueReceive(queueHandler01, (void*)generatedData, xFrequency);
		if(queueStatus == pdPASS) {
			HAL_UART_Transmit(&huart1, generatedData, DATA_PACK, 1);
		}
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}

	vTaskDelete(hTransmitDataTask);
}

