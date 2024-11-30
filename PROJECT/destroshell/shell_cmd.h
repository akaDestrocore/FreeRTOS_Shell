#ifndef __SHELL_CMD_H__
#define __SHELL_CMD_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <destroshell.h>
#include <timers.h>

/* API prototypes */
void shell_cmd_clear(Shell_Handle_t *handle);
void shell_cmd_help(Shell_Handle_t *handle);
void shell_cmd_status(Shell_Handle_t *handle);
void shell_cmd_reset(Shell_Handle_t *handle);
void shell_cmd_reset_cancel(Shell_Handle_t *handle);

#ifdef __cplusplus
}
#endif
#endif /* __SHELL_CMD_H__ */