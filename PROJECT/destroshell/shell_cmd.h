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

/* Helper functions */
GPIO_TypeDef* get_gpio_port(const char *port_str);
uint16_t get_gpio_pin(const char *pin_str);

#ifdef __cplusplus
}
#endif
#endif /* __SHELL_CMD_H__ */