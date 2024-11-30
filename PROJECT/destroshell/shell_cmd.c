#include <shell_cmd.h>

static Shell_Handle_t *globalShellHandle;
static TimerHandle_t resetTimer = NULL;

void shell_cmd_clear(Shell_Handle_t *handle)
{
    sh_print(handle, "⟹ \033[2J\033[H");
}

void shell_cmd_help(Shell_Handle_t *handle)
{
    sh_print(handle, "⟹ Available commands: help, reset, status, clear\r\n");
}

void shell_cmd_status(Shell_Handle_t *handle)
{
    sh_print(handle, "⟹ System is running.\r\n");
}

void reset_callback(TimerHandle_t xTimer) 
{
    sh_print(globalShellHandle, "System is resetting now...\r\n");
    NVIC_SystemReset(); 
}

void shell_cmd_reset(Shell_Handle_t *handle) 
{
    if (NULL == resetTimer) 
    {
        resetTimer = xTimerCreate("ResetTimer", pdMS_TO_TICKS(10000), pdFALSE, (void *)0, reset_callback);
    }

    if (NULL != resetTimer) 
    {
        xTimerStart(resetTimer, 0);
        sh_print(handle, "System will reset in 10 seconds. Type 'cancel reset' to cancel.\r\n");
    } 
    else 
    {
        sh_print(handle, "Failed to create reset timer.\r\n");
    }
}

void shell_cmd_reset_cancel(Shell_Handle_t *handle) 
{
    if ((NULL != resetTimer) && pdTRUE == (xTimerIsTimerActive(resetTimer))) 
    {
        xTimerStop(resetTimer, 0);
        sh_print(handle, "Reset has been canceled.\r\n");
    } 
    else 
    {
        sh_print(handle, "No reset in progress.\r\n");
    }
}