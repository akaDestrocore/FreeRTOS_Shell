#include <destroshell.h>
#include <string.h>
#include <stdio.h>

/* Private variables ----------------------------------------------------------*/
static Shell_Handle_t *globalShellHandle = NULL;

/**
  * @brief  Shell initialization
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
    sh_print(handle, "\r\n>>> destroshell v1.0 <<<\r\n");
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
            
            if (0 == strcmp(receivedCommand, "help")) 
            {
                sh_print(handle, "⟹ Available commands: help, reset, status, clear\r\n");
            } 
            else if (0 == strcmp(receivedCommand, "reset")) 
            {
                sh_print(handle, "⟹ System reset not implemented yet.\r\n");
            } 
            else if (0 == strcmp(receivedCommand, "status")) 
            {
                sh_print(handle, "⟹ System is running.\r\n");
            }
            else if (0 == strcmp(receivedCommand, "clear")) 
            {
                sh_print(handle, "⟹ \033[2J\033[H");
            }
            else 
            {
                char str[256];
                sprintf(str, "⟹ Unknown command: %s\r\n", receivedCommand);
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
                    sh_print(handle, " \b"); 
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
