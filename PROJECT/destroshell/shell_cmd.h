#ifndef __SHELL_CMD_H__
#define __SHELL_CMD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <destroshell.h>
#include <timers.h>

/* External variables */
extern uint8_t commandCount;
extern ShellCommand_t shellCommands[];

/* Shell arguments */
typedef struct {
    const char* flag;
    const char* value;
} ShellArg_t;

typedef struct {
    UART_HandleTypeDef handle;
    USART_TypeDef* instance;
    bool initialized;
} UART_Config_t;

typedef struct {
    I2C_HandleTypeDef handle;
    I2C_TypeDef* instance;
    bool initialized;
} I2C_Config_t;

typedef struct {
    SPI_HandleTypeDef handle;
    SPI_TypeDef* instance;
    bool initialized;
} SPI_Config_t;

typedef struct {
    TIM_HandleTypeDef handle;
    TIM_TypeDef* instance;
    bool initialized;
} TIM_Config_t;

typedef struct {
    RTC_HandleTypeDef handle;
    bool initialized;
} RTC_Config_t;

/* API prototypes */
void shell_cmd_clear(Shell_Handle_t *handle, int argc, char *argv[]);
void shell_cmd_help(Shell_Handle_t *handle, int argc, char *argv[]);
void shell_cmd_status(Shell_Handle_t *handle, int argc, char *argv[]);
void shell_cmd_reset(Shell_Handle_t *handle, int argc, char *argv[]);
void shell_cmd_reset_cancel(Shell_Handle_t *handle, int argc, char *argv[]);
void shell_cmd_tasks(Shell_Handle_t *handle, int argc, char *argv[]);
void shell_cmd_heap(Shell_Handle_t *handle, int argc, char *argv[]);
void shell_cmd_stack(Shell_Handle_t *handle, int argc, char *argv[]);
void shell_cmd_pin(Shell_Handle_t *handle, int argc, char *argv[]);
void shell_cmd_init(Shell_Handle_t *handle, int argc, char *argv[]);

#ifdef __cplusplus
}
#endif
#endif /* __SHELL_CMD_H__ */