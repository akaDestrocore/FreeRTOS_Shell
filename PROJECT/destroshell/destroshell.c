#include <destroshell.h>

/* Private variables ----------------------------------------------------------*/
static Shell_Handle_t *globalShellHandle = NULL;
static ShellCommand_t shellCommands[SHELL_MAX_COMMANDS];
static uint8_t commandCount = 0;

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
    globalShellHandle = handle;  

    if (NULL == handle->queue) 
    {
        sh_print(handle, "Shell queue creation failed.\r\n");
        return;
    }
    sh_print(handle, "\r\n➩ ➩ ➩ destroshell v1.0 🢤 🢤 🢤\r\n");
    sh_print(handle, "Type 'help' to see available commands\r\n");
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
  * @param handler command handler function
  * @retval None
  */
void Shell_RegisterCommand(const char *name, void (*handler)(Shell_Handle_t *)) 
{
    if (commandCount < SHELL_MAX_COMMANDS) 
    {
        shellCommands[commandCount].commandName = name;
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

    if ((NULL == handle) || (NULL == handle->queue)) 
    {
        return;
    }

    while (1) 
    {
        if (pdPASS == xQueueReceive(handle->queue, receivedCommand, portMAX_DELAY)) 
        {
            if (0 == strlen(receivedCommand)) 
            {
                sh_print(handle, (const char*)prompt);
                continue;
            }

            bool commandFound = false;
            for (uint8_t i = 0; i < commandCount; i++) {
                if (strcmp(receivedCommand, shellCommands[i].commandName) == 0) {
                    shellCommands[i].commandHandler(handle);
                    commandFound = true;
                    break;
                }
            }

            if (false == commandFound) 
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
    char input[SHELL_QUEUE_LENGTH];
    uint16_t index = 0;
    uint8_t ch;
    
    sh_print(handle, (const char*)prompt);

    while (1) 
    {
        if (HAL_OK == HAL_UART_Receive(handle->huart, &ch, 1, HAL_MAX_DELAY)) 
        {
            HAL_UART_Transmit(handle->huart, &ch, 1, HAL_MAX_DELAY);
            
            if (ch == '\b' || ch == 0x7F) 
            {
                if (index > 0) 
                {
                    index--;
                    sh_print(handle, "\b \b");
                }
                continue;
            }
            
            if (ch == '\r') 
            {
                sh_print(handle, "\n");
                if (index > 0) 
                {
                    input[index] = '\0';
                    xQueueSend(handle->queue, input, portMAX_DELAY);
                    index = 0;
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
            
            if (index < SHELL_QUEUE_LENGTH - 1 && ch >= 32 && ch <= 126) 
            {
                input[index++] = ch;
            }
        }
    }
}