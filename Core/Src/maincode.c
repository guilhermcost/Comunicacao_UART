#include <stdint.h>
#include "main.h"
#include "cmsis_os.h"
#include "maincode.h"
#include "queue.h"

TaskHandle_t hLedTask;
TaskHandle_t hReciveDataTask;
QueueHandle_t queueHandler01;

HAL_StatusTypeDef UART_state;
uint8_t UART_data;

void start_rtos(void) {
	queueHandler01 = xQueueCreate(10, sizeof(uint8_t));

	xTaskCreate(
		dataGenerator,	//função
		"ledTask",	//nome
		128,		//pilha
		NULL, 		//parametro
		1,			//prioridade
		&hLedTask
	);

	xTaskCreate(
		reciveDataTask,
		"reciveDataTask",
		128,
		NULL,
		1,
		&hReciveDataTask
	);

	vTaskStartScheduler();

	while(1);
}

void dataGenerator(void *arg) {
	uint8_t pinState = 0;
	uint8_t sampleData[10] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j' };
	BaseType_t queueStatus;
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(100);


	while(1) {
		pinState = !pinState;

		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, pinState);

		for(int i = 0; i < sizeof(sampleData); i++) {
			sampleData[i] ^= 0x20;
			queueStatus = xQueueSend(queueHandler01, (void*)(sampleData + i), xFrequency);
			vTaskDelayUntil( &xLastWakeTime, xFrequency );
		}


	}
	vTaskDelete(hLedTask);
}


//void uart() {
//	UART_state = HAL_UART_Receive(&huart1, &UART_data, 1, 1);
//	if(UART_state == HAL_OK) {
//		HAL_UART_Transmit(&huart1, &UART_data, 1, 1);
//	}
//}

void reciveDataTask(void *arg) {
	uint8_t recivedData;
	BaseType_t queueStatus;
	TickType_t xLastWakeTime;
	const TickType_t xFrequency = pdMS_TO_TICKS(100);

	xLastWakeTime = xTaskGetTickCount();

	while(1) {
		queueStatus = xQueueReceive(queueHandler01, (void*)(&recivedData), xFrequency);
		if(queueStatus == pdPASS) {
			HAL_UART_Transmit(&huart1, &recivedData, 1, 1);
		}
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}

	vTaskDelete(hReciveDataTask);
}






