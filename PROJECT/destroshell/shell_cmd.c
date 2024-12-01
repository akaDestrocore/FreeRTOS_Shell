#include <shell_cmd.h>

static Shell_Handle_t *globalShellHandle;
static TimerHandle_t resetTimer = NULL;

/**
  * @brief  clear screen command
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void shell_cmd_clear(Shell_Handle_t *handle, int argc, char *argv[]) 
{
    sh_print(handle, "\033[2J\033[H");  
}

/**
  * @brief  command that prints information about available commands
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void shell_cmd_help(Shell_Handle_t *handle, int argc, char *argv[]) 
{
    if (argc > 1) 
    {
        // Show specific command help
        for (uint8_t i = 0; i < commandCount; i++) 
        {
            if (0 == strcmp(argv[1], shellCommands[i].commandName)) 
            {
                sh_print(handle, "\r\nCommand: ");
                sh_print(handle, shellCommands[i].commandName);
                sh_print(handle, "\r\nDescription: ");
                sh_print(handle, shellCommands[i].description);
                sh_print(handle, "\r\n");
                return;
            }
        }
        sh_print(handle, "Command not found\r\n");
    } 
    else 
    {
        // Show all commands
        sh_print(handle, "\r\nAvailable commands:\r\n");
        for (uint8_t i = 0; i < commandCount; i++) 
        {
            sh_print(handle, shellCommands[i].commandName);
            sh_print(handle, " : ");
            sh_print(handle, shellCommands[i].description);
            sh_print(handle, "\r\n");
        }
    }
}

/**
  * @brief  prints system status
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void shell_cmd_status(Shell_Handle_t *handle, int argc, char *argv[]) 
{
    sh_print(handle, "⟹ System is running.\r\n");
}

/**
  * @brief  system reset command
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void shell_cmd_reset(Shell_Handle_t *handle, int argc, char *argv[]) 
{
    if (handle->resetPending) 
    {
        sh_print(handle, "Reset already pending. Use 'cancel reset' to cancel.\r\n");
        return;
    }

    handle->resetPending = true;
    sh_print(handle, "System will reset in 60 seconds...\r\n");
    sh_print(handle, "Use 'cancel reset' to cancel.\r\n");
    
    if (pdPASS != xTimerStart(handle->resetTimer, 0)) 
    {
        sh_print(handle, "Failed to start reset timer.\r\n");
        handle->resetPending = false;
    }
}

/**
  * @brief  system reset command
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void shell_cmd_reset_cancel(Shell_Handle_t *handle, int argc, char *argv[]) 
{
    if (false == handle->resetPending) 
    {
        sh_print(handle, "No reset pending.\r\n");
        return;
    }

    if (pdPASS != xTimerStop(handle->resetTimer, 0)) 
    {
        sh_print(handle, "Failed to stop reset timer.\r\n");
        return;
    }

    handle->resetPending = false;
    sh_print(handle, "Reset cancelled.\r\n");
}