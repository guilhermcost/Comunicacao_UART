#ifndef SRC_MAINCODE_H_
#define SRC_MAINCODE_H_

extern UART_HandleTypeDef huart1;

void start_rtos(void);
void dataGenerator(void *arg);
void reciveDataTask(void *arg);

#endif /* SRC_MAINCODE_H_ */
