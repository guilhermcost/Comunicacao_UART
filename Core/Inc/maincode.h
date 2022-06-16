#ifndef SRC_MAINCODE_H_
#define SRC_MAINCODE_H_

extern UART_HandleTypeDef huart1;

void start_rtos(void);
void generateDataTask(void *arg);
void transmitDataTask(void *arg);
void recieveDataTask(void *arg);
void ledTask(void *arg);

#endif /* SRC_MAINCODE_H_ */
