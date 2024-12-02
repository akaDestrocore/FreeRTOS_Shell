#include <destroshell.h>

/* Private variables ----------------------------------------------------------*/
static Shell_Handle_t *globalShellHandle = NULL;
ShellCommand_t shellCommands[SHELL_MAX_COMMANDS];
uint8_t commandCount = 0;

/**
  * @brief  reset timer callback
  * @param xTimer timer handle
  * @retval None
  */
static void vResetTimerCallback(TimerHandle_t xTimer) 
{
    Shell_Handle_t *handle = (Shell_Handle_t *)pvTimerGetTimerID(xTimer);
    if (handle->resetPending) 
    {
        NVIC_SystemReset();
    }
}

/**
  * @brief  shell initialization
  * @param handle shell handle
  * @param huart UART handle
  * @retval None
  */
void Shell_Init(Shell_Handle_t *handle, UART_HandleTypeDef *huart) 
{
    handle->huart = huart;
    handle->queue = xQueueCreate(SHELL_QUEUE_LENGTH, SHELL_QUEUE_ITEM_SIZE);
    handle->bufferIndex = 0;
    handle->resetPending = false;
    globalShellHandle = handle;

    // Create reset timer
    handle->resetTimer = xTimerCreate("ResetTimer", 
                                    pdMS_TO_TICKS(60000),  // 60 second delay
                                    pdFALSE,                
                                    (void*)handle,        // Timer ID
                                    vResetTimerCallback);

    if (NULL == handle->queue) 
    {
        sh_print(handle, "Shell queue creation failed.\r\n");
        return;
    }
    
    sh_print(handle, "\r\n➩ ➩ ➩ destroshell v1.0 🢤 🢤 🢤\r\n");
    sh_print(handle, "Type 'help' to see available commands\r\n");
}

/**
  * @brief  helper function to parse user input arguments
  * @param cmd command string with arguments
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void Shell_ParseArgs(char *cmd, int *argc, char *argv[]) 
{
    *argc = 0;
    char *token = strtok(cmd, " ");
    
    while (token != NULL && *argc < SHELL_MAX_ARGS) 
    {
        strncpy(argv[*argc], token, SHELL_MAX_ARG_LEN - 1);
        argv[*argc][SHELL_MAX_ARG_LEN - 1] = '\0';
        (*argc)++;
        token = strtok(NULL, " ");
    }
}

/**
  * @brief  shell print function
  * @param handle shell handle
  * @param str character array
  * @retval None
  */
void sh_print(Shell_Handle_t *handle, const char *str) 
{
    if ((NULL != handle) && (NULL != handle->huart) && (0 != str)) 
    {
        HAL_UART_Transmit(handle->huart, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
    }
}

/**
  * @brief  register a new command in the shell
  * @param name command name
  * @param description command description character array
  * @param handler command handler function
  * @retval None
  */
void Shell_RegisterCommand(const char *name, const char *description, const char *usage, void (*handler)(Shell_Handle_t*, int argc, char *argv[])) 
{
    if (commandCount < SHELL_MAX_COMMANDS) 
    {
        shellCommands[commandCount].commandName = name;
        shellCommands[commandCount].description = description;
        shellCommands[commandCount].usage = usage;
        shellCommands[commandCount].commandHandler = handler;
        commandCount++;
    } 
    else 
    {
        sh_print(globalShellHandle, "Cannot register more commands. Limit reached.\r\n");
    }
}

/**
  * @brief  main shell task
  * @param pvParameters A value that is passed as the paramater to the created task. 
  * If pvParameters is set to the address of a variable then the variable must still exist when 
  * the created task executes - so it is not valid to pass the address of a stack variable.
  * @retval None
  */
void Shell_Task(void *pvParameters) 
{
    Shell_Handle_t *handle = (Shell_Handle_t *)pvParameters;
    char receivedCommand[SHELL_QUEUE_ITEM_SIZE];
    int argc;
    char argv[SHELL_MAX_ARGS][SHELL_MAX_ARG_LEN];
    char *argvPtr[SHELL_MAX_ARGS];

    for (int i = 0; i < SHELL_MAX_ARGS; i++) 
    {
        argvPtr[i] = argv[i];
    }

    if ((NULL == handle) || (NULL == handle->queue)) 
    {
        return;
    }

    while (1) 
    {
        if (pdPASS == xQueueReceive(handle->queue, receivedCommand, portMAX_DELAY)) 
        {
            if (strlen(receivedCommand) == 0) 
            {
                sh_print(handle, (const char*)prompt);
                continue;
            }

            Shell_ParseArgs(receivedCommand, &argc, argvPtr);
            bool commandFound = false;

            for (uint8_t i = 0; i < commandCount; i++) 
            {
                if (strcmp(argvPtr[0], shellCommands[i].commandName) == 0) 
                {
                    shellCommands[i].commandHandler(handle, argc, argvPtr);
                    commandFound = true;
                    break;
                }
            }

            if (!commandFound) 
            {
                char str[256];
                sprintf(str, "➩ Unknown command: %s\r\n", receivedCommand);
                sh_print(handle, str);
            }
            sh_print(handle, (const char*)prompt);
        }
    }
}

/**
  * @brief  main UART task
  * @param pvParameters A value that is passed as the paramater to the created task.
  * If pvParameters is set to the address of a variable then the variable must still exist when
  * the created task executes - so it is not valid to pass the address of a stack variable.
  * @retval None
  */
void vUartTask(void *pvParameters) 
{
    Shell_Handle_t *handle = (Shell_Handle_t *)pvParameters;
    uint8_t ch;
    
    sh_print(handle, (const char*)prompt);

    while (1) 
    {
        if (HAL_OK == HAL_UART_Receive(handle->huart, &ch, 1, HAL_MAX_DELAY)) 
        {
            HAL_UART_Transmit(handle->huart, &ch, 1, HAL_MAX_DELAY);
            
            if (ch == '\b' || ch == 0x7F) 
            {
                if (handle->bufferIndex > 0) 
                {
                    handle->bufferIndex--;
                    sh_print(handle, "\b \b");
                }
                continue;
            }
            
            if (ch == '\r') 
            {
                sh_print(handle, "\n");
                if (handle->bufferIndex > 0) 
                {
                    handle->cmdBuffer[handle->bufferIndex] = '\0';
                    xQueueSend(handle->queue, handle->cmdBuffer, portMAX_DELAY);
                    handle->bufferIndex = 0;
                } 
                else 
                {
                    xQueueSend(handle->queue, "", portMAX_DELAY);
                }
                continue;
            }
            
            if (ch == '\n') 
            {
                continue;
            }
            
            if (handle->bufferIndex < SHELL_QUEUE_LENGTH - 1 && ch >= 32 && ch <= 126) 
            {
                handle->cmdBuffer[handle->bufferIndex++] = ch;
            }
        }
    }
}