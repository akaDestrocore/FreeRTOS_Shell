#ifndef __DESTROSHELL_H__
#define __DESTROSHELL_H__

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

/* Configuration constants */
#define SHELL_MAX_COMMANDS 4
#define SHELL_QUEUE_LENGTH 10
#define SHELL_QUEUE_ITEM_SIZE 256

/*
 * Configuration structure for Shell
 */
typedef struct {
    UART_HandleTypeDef *huart;  /* UART handle */
    QueueHandle_t queue;        /* Queue for received commands */
} Shell_Handle_t;

/*
 * Shell command structure
 */
typedef struct {
    const char *commandName;      /* Command name */
} ShellCommand_t;

/*
 * API prototypes
 */
void Shell_Init(Shell_Handle_t *handle, UART_HandleTypeDef *huart);
void Shell_Task(void *pvParameters);
void vUartTask(void *pvParameters);
void sh_print(Shell_Handle_t *handle, const char *str);

#endif /* __DESTROSHELL_H__ */