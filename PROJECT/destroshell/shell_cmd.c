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

/**
  * @brief  print current task information
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void shell_cmd_tasks(Shell_Handle_t *handle, int argc, char *argv[]) 
{
    if (argc > 1 && 0 == strcmp(argv[1], "list")) 
    {
        char buf[512];
        vTaskList(buf); 
        sh_print(handle, "\r\nTask Name\tState\tPrio\tStack\tNum\r\n");
        sh_print(handle, "------------------------------------------------\r\n");
        sh_print(handle, buf);
    } 
    else if (argc > 2 && 0 == strcmp(argv[1], "info")) 
    {
        TaskHandle_t xHandle = NULL;
        TaskStatus_t xTaskDetails;
        
        // Compare names to find task
        UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
        TaskStatus_t *pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
        
        if (NULL != pxTaskStatusArray) 
        {
            uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
            
            for (UBaseType_t i = 0; i < uxArraySize; i++) 
            {
                if (0 == strcmp(argv[2], pxTaskStatusArray[i].pcTaskName)) 
                {
                    memcpy(&xTaskDetails, &pxTaskStatusArray[i], sizeof(TaskStatus_t));
                    xHandle = pxTaskStatusArray[i].xHandle;
                    break;
                }
            }
            
            vPortFree(pxTaskStatusArray);
            
            if (NULL != xHandle) 
            {
                char buf[256];
                sprintf(buf, "\r\nTask: %s\r\nState: %lu\r\nPriority: %lu\r\nStack High Water Mark: %lu\r\n",
                        xTaskDetails.pcTaskName, 
                        xTaskDetails.eCurrentState,
                        xTaskDetails.uxCurrentPriority,
                        xTaskDetails.usStackHighWaterMark);
                sh_print(handle, buf);
            } 
            else 
            {
                sh_print(handle, "Task not found\r\n");
            }
        } 
        else 
        {
            sh_print(handle, "Failed to allocate memory for task information\r\n");
        }
    } 
    else 
    {
        sh_print(handle, "Usage: tasks list | tasks info <task_name>\r\n");
    }
}

/**
  * @brief  print current heap information
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void shell_cmd_heap(Shell_Handle_t *handle, int argc, char *argv[]) 
{
    HeapStats_t heapStats;
    vPortGetHeapStats(&heapStats);
    char buf[256];
    sprintf(buf, "\r\nHeap Information:\r\n"
                 "Total Heap: %d bytes\r\n"
                 "Free Heap: %d bytes\r\n"
                 "Used Heap: %d bytes\r\n"
                 "Minimum Ever Free: %d bytes\r\n",
            configTOTAL_HEAP_SIZE,
            heapStats.xAvailableHeapSpaceInBytes,
            configTOTAL_HEAP_SIZE - heapStats.xAvailableHeapSpaceInBytes,
            heapStats.xMinimumEverFreeBytesRemaining);
    sh_print(handle, buf);
}

/**
  * @brief  print current stack information
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
 void shell_cmd_stack(Shell_Handle_t *handle, int argc, char *argv[]) 
 {
    UBaseType_t uxTaskCount = uxTaskGetNumberOfTasks();
    TaskStatus_t *pxTaskStatusArray = pvPortMalloc(uxTaskCount * sizeof(TaskStatus_t));
    
    if (NULL != pxTaskStatusArray) 
    {
        uxTaskCount = uxTaskGetSystemState(pxTaskStatusArray, uxTaskCount, NULL);
        sh_print(handle, "\r\nStack Usage Information:\r\n");
        sh_print(handle, "Task Name\tStack High Water Mark\r\n");
        sh_print(handle, "--------------------------------\r\n");
        
        for (UBaseType_t i = 0; i < uxTaskCount; i++) 
        {
            char buf[128];
            sprintf(buf, "%s\t%lu\r\n", 
                    pxTaskStatusArray[i].pcTaskName,
                    pxTaskStatusArray[i].usStackHighWaterMark);
            sh_print(handle, buf);
        }
        
        vPortFree(pxTaskStatusArray);
    } 
    else 
    {
        sh_print(handle, "Failed to allocate memory for task information\r\n");
    }
}

/**
  * @brief  configure selected pin 
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void shell_cmd_pin(Shell_Handle_t *handle, int argc, char *argv[]) 
{
    if (argc < 4) 
    {
        sh_print(handle, "Usage: pin <set/reset/read/toggle> <port: A, B, etc.> <pin_number>\r\n");
        return;
    }

    GPIO_TypeDef *port = get_gpio_port(argv[2]);
    uint16_t pin = get_gpio_pin(argv[3]);
    
    if (0 == port || 0 == pin) 
    {
        sh_print(handle, "Invalid port or pin\r\n");
        return;
    }

    if (0 == strcmp(argv[1], "set")) 
    {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    } 
    else if (0 == strcmp(argv[1], "reset")) 
    {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    } 
    else if (0 == strcmp(argv[1], "read")) 
    {
        GPIO_PinState state = HAL_GPIO_ReadPin(port, pin);
        char buf[32];
        sprintf(buf, "Pin state: %d\r\n", state);
        sh_print(handle, buf);
    } 
    else if (0 == strcmp(argv[1], "toggle")) 
    {
        HAL_GPIO_TogglePin(port, pin);
    } 
    else 
    {
        sh_print(handle, "Invalid command\r\n");
    }
}

/* Helper Functions */
GPIO_TypeDef* get_gpio_port(const char *port_str) 
{
    if (strcmp(port_str, "GPIOA") == 0 || strcmp(port_str, "gpioa") == 0 || strcmp(port_str, "portA") == 0 || strcmp(port_str, "A") == 0 
    || strcmp(port_str, "a") == 0) return GPIOA;
    if (strcmp(port_str, "GPIOB") == 0 || strcmp(port_str, "gpiob") == 0 || strcmp(port_str, "portB") == 0 || strcmp(port_str, "B") == 0 
    || strcmp(port_str, "b") == 0) return GPIOB;
    if (strcmp(port_str, "GPIOC") == 0 || strcmp(port_str, "gpioc") == 0 || strcmp(port_str, "portC") == 0 || strcmp(port_str, "C") == 0 
    || strcmp(port_str, "c") == 0) return GPIOC;
    if (strcmp(port_str, "GPIOD") == 0 || strcmp(port_str, "gpiod") == 0 || strcmp(port_str, "portD") == 0 || strcmp(port_str, "D") == 0 
    || strcmp(port_str, "d") == 0) return GPIOD;
    if (strcmp(port_str, "GPIOE") == 0 || strcmp(port_str, "gpioe") == 0 || strcmp(port_str, "portE") == 0 || strcmp(port_str, "E") == 0 
    || strcmp(port_str, "e") == 0) return GPIOE;
    return NULL;
}

uint16_t get_gpio_pin(const char *pin_str) 
{
    int pin_num = atoi(pin_str);
    if (pin_num >= 0 && pin_num <= 15) 
    {
        return (1 << pin_num);
    }
    return 0;
}