#include <stdint.h>
#include "main.h"
#include "cmsis_os.h"
#include "maincode.h"
#include "queue.h"
#include "semphr.h"

#define MS_TO_TRANSMIT 1
#define MS_TO_RECIEVE  1
#define DATA_PACK  45
#define DATA_PACK2 21

TaskHandle_t hGenerateDataTask;
TaskHandle_t hTransmitDataTask;
TaskHandle_t hRecieveDataTask;
TaskHandle_t hLedTask;
QueueHandle_t queueHandler01;
QueueHandle_t queueHandler02;
SemaphoreHandle_t xUart_semaphore;

void start_rtos(void) {
	queueHandler01 = xQueueCreate(1, sizeof(uint8_t) * DATA_PACK);
	queueHandler02 = xQueueCreate(1, sizeof(uint8_t) * DATA_PACK2);

	xUart_semaphore = xSemaphoreCreateMutex();

	xTaskCreate(
		generateDataTask,
		"generateDataTask",
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

	xTaskCreate(
		recieveDataTask,
		"recieveDataTask",
		128,
		NULL,
		1,
		&hRecieveDataTask
	);

	xTaskCreate(
		ledTask,
		"ledTask",
		128,
		NULL,
		1,
		&hLedTask
	);

	vTaskStartScheduler();

	while(1);
}

void generateDataTask(void *arg) {
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
	uint8_t parity = 0;

	while(1) {

		if(packCounter >= 1) {
			sampleData[31] += 1;
			sampleData[33] += 1;
			sampleData[35] += 1;
		} else {
			sampleData[31] = 'D';
			sampleData[33] = 'D';
			sampleData[35] = 'D';
		}

		sampleData[0]  = packCounter;
		sampleData[0] |= 0b01000000;

		if(packCounter >= 7) {
			if(pack10Counter == 0) {
				sampleData[0] |= 0b00010000;
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
			sampleData[0] &= ~0b00010000;
			sampleData[37] = '*';
			sampleData[39] = '*';
			sampleData[41] = '*';
		}

		// Para incluir tstamp (comentado para fins de teste)
//		TickType_t now = xTaskGetTickCount();
//		sampleData[43] = (uint8_t)((now & 0xFF00) >> 8);
//		sampleData[44] = (uint8_t)((now & 0x00FF) >> 0);

		uint8_t p = 0;
		for(int i = 0; i < sizeof(sampleData); i++) p ^= sampleData[i];
		p = ((p>>7)^(p>>6)^(p>>5)^(p>>4)^(p>>3)^(p>>2)^(p>>1)^(p>>0)) & 1;
		parity = (p << 5);
		sampleData[0] |= parity;

		queueStatus = xQueueSend(queueHandler01, (void*)sampleData, xFrequency);
		if(queueStatus != pdTRUE) xQueueSend(queueHandler01, (void*)sampleData, xFrequency);
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
			xSemaphoreTake(xUart_semaphore, portMAX_DELAY );
			HAL_UART_Transmit(&huart1, generatedData, DATA_PACK, 1);
			xSemaphoreGive(xUart_semaphore);
		}
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}

	vTaskDelete(hTransmitDataTask);
}

void ledTask(void *arg) {
	uint8_t recievedData [DATA_PACK2];
	BaseType_t queueStatus;
	const TickType_t xFrequency = pdMS_TO_TICKS(MS_TO_RECIEVE);

	while(1) {
		queueStatus = xQueueReceive(queueHandler02, (void*)recievedData, xFrequency);
		uint8_t state = (queueStatus == pdPASS) ? 0 : 1;
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, state);
	}

	vTaskDelete(hLedTask);
}

void recieveDataTask(void *arg) {
	HAL_UART_StateTypeDef UART_status;
	uint8_t Loopback_state = 0;
	const TickType_t xFrequency = pdMS_TO_TICKS(MS_TO_RECIEVE);
	uint8_t UART_data_rx [3] = { '*', '*', '*', };
	uint8_t recievedData [DATA_PACK2] = {
		'>', '>', 'A', 'P', 'E',
		'R', 'I', 'O', 'D', 'I',
		'C', '_', 'R', 'X', '_',
		'_', '_', '_', '*', '\r',
		'\n',
	};

	while(1) {
		xSemaphoreTake(xUart_semaphore, xFrequency);
		switch(Loopback_state) {
			case 0:
				UART_status = HAL_UART_GetState(&huart1);
				if(UART_status == HAL_UART_STATE_READY || UART_status == HAL_UART_STATE_BUSY_TX) {
					HAL_UART_Receive_IT(&huart1, UART_data_rx, 3);
					Loopback_state = 1;
				}
			break;
			case 1:
				UART_status = HAL_UART_GetState(&huart1);
				if(UART_status == HAL_UART_STATE_READY) {
					recievedData[16] = UART_data_rx[0];
					recievedData[17] = UART_data_rx[1];
					recievedData[18] = UART_data_rx[2];
					HAL_UART_Transmit(&huart1, recievedData, DATA_PACK2, 1);
					xQueueSend(queueHandler02, (void*)recievedData, xFrequency);
					Loopback_state = 0;
				}
			break;
		}
		xSemaphoreGive(xUart_semaphore);
	}

	vTaskDelete(hRecieveDataTask);
}

