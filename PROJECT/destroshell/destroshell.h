#ifndef __DESTROSHELL_H__
#define __DESTROSHELL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <timers.h>
#include <stm32f4xx_hal.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* Configuration constants */
#define SHELL_MAX_COMMANDS 100
#define SHELL_QUEUE_LENGTH 256
#define SHELL_QUEUE_ITEM_SIZE 256
#define SHELL_MAX_ARGS 10
#define SHELL_MAX_ARG_LEN 32

/* Some character string definitions*/
static const char *prompt = "[root@root ~]# ";

/*
 * Configuration structure for Shell
 */
typedef struct {
    UART_HandleTypeDef *huart;          /* UART handle */
    QueueHandle_t queue;                /* Queue for received commands */
    char cmdBuffer[SHELL_QUEUE_LENGTH];
    uint16_t bufferIndex;
    TimerHandle_t resetTimer;           /* Timer for delayed reset */
    bool resetPending;                  /* Flag to track if reset is pending */
} Shell_Handle_t;

/*
 * Shell command structure
 */
typedef struct {
    const char *commandName;
    const char *description;
    void (*commandHandler)(Shell_Handle_t*, int argc, char *argv[]);
} ShellCommand_t;

/* API prototypes */
void Shell_Init(Shell_Handle_t *handle, UART_HandleTypeDef *huart);
void Shell_Task(void *pvParameters);
void vUartTask(void *pvParameters);
void sh_print(Shell_Handle_t *handle, const char *str);
void Shell_RegisterCommand(const char *name, const char *description, void (*handler)(Shell_Handle_t*, int argc, char *argv[]));

#ifdef __cplusplus
}
#endif
#endif /* __DESTROSHELL_H__ */