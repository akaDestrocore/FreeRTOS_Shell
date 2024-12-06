#include <shell_cmd.h>

static Shell_Handle_t *globalShellHandle;
static TimerHandle_t resetTimer = NULL;

/* Helper functions */
static GPIO_TypeDef* get_gpio_port(const char *port_str);
static int parse_args(int argc, char *argv[], int start_idx, ShellArg_t *args, int max_args);
static const char* get_arg_value(ShellArg_t *args, int arg_count, const char *flag, const char *default_value);
static USART_TypeDef* get_usart_base(const char* uart_name);
static I2C_TypeDef* get_i2c_base(const char* i2c_name) ;
static SPI_TypeDef* get_spi_base(const char* spi_name); 
static TIM_TypeDef* get_timer_base(const char* timer_name);
static void init_uart(Shell_Handle_t *handle, const char* peripheral_name, ShellArg_t *args, int arg_count);
static void init_spi(Shell_Handle_t *handle, const char* peripheral_name, ShellArg_t *args, int arg_count);
static void init_i2c(Shell_Handle_t *handle, const char* peripheral_name, ShellArg_t *args, int arg_count);
static void init_timer(Shell_Handle_t *handle, const char* peripheral_name, ShellArg_t *args, int arg_count);
static void init_rtc(Shell_Handle_t *handle, ShellArg_t *args, int arg_count);

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
    if (SET == handle->resetPending) 
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
    if (RESET == handle->resetPending) 
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
        sh_print(handle, "Usage: pin <set/reset/read/toggle> <port: GPIOA, GPIOB, etc.> <pin_number>\r\n");
        return;
    }

    GPIO_TypeDef *port = get_gpio_port(argv[2]);

    int pin_number = atoi(argv[3]);
    uint16_t pin;
    
    if (pin_number >= 0 && pin_number <= 15) 
    {
        pin = 1 << pin_number; 
    }

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

/**
  * @brief  initialize selected peripheral
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
void shell_cmd_init(Shell_Handle_t *handle, int argc, char *argv[]) 
{
    static UART_Config_t uart_configs[6] = {0};
    static I2C_Config_t i2c_configs[3] = {0};
    static SPI_Config_t spi_configs[6] = {0};
    static TIM_Config_t tim_configs[14] = {0};
    static RTC_Config_t rtc_config = {0};
    
    if (argc < 3) 
    {
        sh_print(handle, "Usage: init <peripheral_type> <peripheral_name> [options]\r\n");
        sh_print(handle, "\r\nAvailable peripherals and options:\r\n");
        sh_print(handle, "1. USART/UART:\r\n");
        sh_print(handle, "   init usart <usart1/uart4/etc> [-baud <rate>] [-wl <7/8/9>] [-sb <0.5/1/1.5/2>]\r\n");
        sh_print(handle, "   [-p <none/odd/even>] [-m <tx/rx/tx_rx>] [-os <8/16>]\r\n\n");
        sh_print(handle, "2. I2C:\r\n");
        sh_print(handle, "   init i2c <i2c1/i2c2/i2c3> [-speed <std/fast/high>] [-addr <7/10>]\r\n\n");
        sh_print(handle, "3. SPI:\r\n");
        sh_print(handle, "   init spi <spi1/spi2/etc> [-mode <0/1/2/3>] [-df <8/16>] [-br <2/4/.../256>]\r\n\n");
        sh_print(handle, "4. TIMER:\r\n");
        sh_print(handle, "   init tim <tim1/tim2/etc> [-period <ms>] [-prescaler <value>]\r\n\n");
        sh_print(handle, "5. RTC:\r\n");
        sh_print(handle, "   init rtc [-clock <lse/lsi/hse>] [-format <12/24>]\r\n");
        return;
    }

    const char* peripheral_type = argv[1];
    const char* peripheral_name = argv[2];
    char buf[256];

    // Parse arguments
    ShellArg_t args[10];
    int arg_count = parse_args(argc, argv, 3, args, 10);

    if (0 == strcmp(peripheral_type, "UART") || 0 == strcmp(peripheral_type, "USART") || 0 == strcmp(peripheral_type, "usart") || 0 == strcmp(peripheral_type, "uart")) 
    {
        init_uart(handle, peripheral_name, args, arg_count);
    }
    else if (0 == strcmp(peripheral_type, "I2C") || 0 == strcmp(peripheral_type, "i2c")) 
    {
        init_i2c(handle, peripheral_name, args, arg_count);
    }
    else if (0 == strcmp(peripheral_type, "SPI") || 0 == strcmp(peripheral_type, "spi")) 
    {
        init_spi(handle, peripheral_name, args, arg_count);
    }
    else if (0 == strcmp(peripheral_type, "TIM") || 0 == strcmp(peripheral_type, "tim")) 
    {
        init_timer(handle, peripheral_name, args, arg_count);
    }
    else if (0 == strcmp(peripheral_type, "RTC") || 0 == strcmp(peripheral_type, "rtc")) 
    {
        init_rtc(handle, args, arg_count);
    }
    else 
    {
        sprintf(buf, "Unknown peripheral type: %s\r\n", peripheral_type);
        sh_print(handle, buf);
    }
}

/**
  * @brief  initialize selected UART peripheral
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
static void init_uart(Shell_Handle_t *handle, const char* peripheral_name, ShellArg_t *args, int arg_count) 
{
    char buf[256];
    USART_TypeDef* usart_base = get_usart_base(peripheral_name);
    if (NULL == usart_base) 
    {
        sprintf(buf, "Invalid UART instance: %s\r\n", peripheral_name);
        sh_print(handle, buf);
        return;
    }

    // Enable GPIO and UART clocks
    if (usart_base == USART1) 
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE(); // PA9/PA10
    } 
    else if (usart_base == USART2) 
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE(); // PA2/PA3
    }
    else if (usart_base == USART3) 
    {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE(); // PB10/PB11
    }
    else if (usart_base == UART4) 
    {
        __HAL_RCC_UART4_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE(); // PA0/PA1
    }
    else if (usart_base == UART5) 
    {
        __HAL_RCC_UART5_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE(); // PD2
        __HAL_RCC_GPIOC_CLK_ENABLE(); // PC12
    }
    else if (usart_base == USART6) 
    {
        __HAL_RCC_USART6_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE(); // PC6/PC7
    }
    
    // Get parameters with defaults
    const char* baud_str = get_arg_value(args, arg_count, "baud", "115200");
    const char* wordlen_str = get_arg_value(args, arg_count, "wl", "8");
    const char* stopbits_str = get_arg_value(args, arg_count, "sb", "1");
    const char* parity_str = get_arg_value(args, arg_count, "p", "none");
    const char* mode_str = get_arg_value(args, arg_count, "m", "tx_rx");

    // Configure GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    
    if (usart_base == USART1) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    else if (usart_base == USART2) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    else if (usart_base == USART3) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    else if (usart_base == UART4) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
        GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    else if (usart_base == UART5) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIO_PIN_12;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
    else if (usart_base == USART6) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
        GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }

    // Initialize UART
    UART_HandleTypeDef huart = {0};
    huart.Instance = usart_base;
    huart.Init.BaudRate = atoi(baud_str);
    huart.Init.WordLength = (strcmp(wordlen_str, "9") == 0) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
    huart.Init.StopBits = (strcmp(stopbits_str, "2") == 0) ? UART_STOPBITS_2 : UART_STOPBITS_1;
    huart.Init.Parity = (strcmp(parity_str, "none") == 0) ? UART_PARITY_NONE : 
                        (strcmp(parity_str, "odd") == 0) ? UART_PARITY_ODD : UART_PARITY_EVEN;
    huart.Init.Mode = (strcmp(mode_str, "tx") == 0) ? UART_MODE_TX :
                      (strcmp(mode_str, "rx") == 0) ? UART_MODE_RX : UART_MODE_TX_RX;
    huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_OK != HAL_UART_Init(&huart)) 
    {
        sprintf(buf, "Failed to initialize %s\r\n", peripheral_name);
        sh_print(handle, buf);
        return;
    }

    sprintf(buf, "%s initialized successfully\r\n", peripheral_name);
    sh_print(handle, buf);
}

/**
  * @brief  initialize selected SPI peripheral
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
static void init_spi(Shell_Handle_t *handle, const char* peripheral_name, ShellArg_t *args, int arg_count) 
{
    char buf[256];
    SPI_TypeDef* spi_base = get_spi_base(peripheral_name);
    if (NULL == spi_base) 
    {
        sprintf(buf, "Invalid SPI instance: %s\r\n", peripheral_name);
        sh_print(handle, buf);
        return;
    }

    // Enable GPIO and SPI clocks
    if (spi_base == SPI1) 
    {
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE(); // PA5 - PA7
    } 
    else if (spi_base == SPI2) 
    {
        __HAL_RCC_SPI2_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE(); 
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    else if (spi_base == SPI3) 
    {
        __HAL_RCC_SPI3_CLK_ENABLE();  
        __HAL_RCC_GPIOC_CLK_ENABLE(); // PC10 - PC12
    }

    // Get parameters with defaults
    const char* mode_str = get_arg_value(args, arg_count, "m", "master");
    const char* dir_str = get_arg_value(args, arg_count, "dir", "2lines");
    const char* datasize_str = get_arg_value(args, arg_count, "datasize", "8");
    const char* pol_str = get_arg_value(args, arg_count, "pol", "low");
    const char* clkphase_str = get_arg_value(args, arg_count, "clkphase", "1edge");
    const char* nss_str = get_arg_value(args, arg_count, "nss", "soft");
    const char* baudr_psc_str = get_arg_value(args, arg_count, "baudr_psc", "2");
    const char* firstbit_str = get_arg_value(args, arg_count, "firstbit", "msb");
    const char* timode_str = get_arg_value(args, arg_count, "timode", "disable");
    const char* crccalc_str = get_arg_value(args, arg_count, "crccalc", "disable");
    const char* crcpoly_str = get_arg_value(args, arg_count, "crcpoly", "10");
    

    // Configure GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    
    if (spi_base == SPI1) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    else if (spi_base == SPI2) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    else if (spi_base == SPI3) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
        GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }

    // Initialize SPI
    SPI_HandleTypeDef hspi = {0};
    hspi.Instance = spi_base;
    hspi.Init.Mode = (strcmp(mode_str, "master") == 0) ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    hspi.Init.Direction = (strcmp(dir_str, "2lines") == 0) ? SPI_DIRECTION_2LINES : 
                          (strcmp(dir_str, "2lines_rxonly") == 0) ? SPI_DIRECTION_2LINES_RXONLY : SPI_DIRECTION_1LINE;
    hspi.Init.DataSize = (strcmp(datasize_str, "8") == 0) ? SPI_DATASIZE_8BIT : SPI_DATASIZE_16BIT;
    hspi.Init.CLKPolarity = (strcmp(pol_str, "low") == 0) ? SPI_POLARITY_LOW : SPI_POLARITY_HIGH;
    hspi.Init.CLKPhase = (strcmp(clkphase_str, "1edge") == 0) ? SPI_PHASE_1EDGE : SPI_PHASE_2EDGE;
    hspi.Init.NSS = (strcmp(nss_str, "soft") == 0) ? SPI_NSS_SOFT : 
                    (strcmp(nss_str, "hard_input") == 0) ?  SPI_NSS_HARD_INPUT : SPI_NSS_HARD_OUTPUT;
    hspi.Init.BaudRatePrescaler = (strcmp(baudr_psc_str, "2") == 0) ? SPI_BAUDRATEPRESCALER_2 : 
                                  (strcmp(baudr_psc_str, "4") == 0) ? SPI_BAUDRATEPRESCALER_4 : 
                                  (strcmp(baudr_psc_str, "8") == 0) ? SPI_BAUDRATEPRESCALER_8 : 
                                  (strcmp(baudr_psc_str, "16") == 0) ? SPI_BAUDRATEPRESCALER_16 : 
                                  (strcmp(baudr_psc_str, "32") == 0) ? SPI_BAUDRATEPRESCALER_32 : 
                                  (strcmp(baudr_psc_str, "64") == 0) ? SPI_BAUDRATEPRESCALER_64 : 
                                  (strcmp(baudr_psc_str, "128") == 0) ? SPI_BAUDRATEPRESCALER_128 : SPI_BAUDRATEPRESCALER_256;
    hspi.Init.FirstBit = (strcmp(firstbit_str, "msb") == 0) ? SPI_FIRSTBIT_MSB : SPI_FIRSTBIT_LSB;
    hspi.Init.TIMode = (strcmp(timode_str, "enable") == 0) ? SPI_TIMODE_ENABLE : SPI_TIMODE_DISABLE;
    hspi.Init.CRCCalculation = (strcmp(crccalc_str, "enable") == 0) ? SPI_CRCCALCULATION_ENABLE : SPI_CRCCALCULATION_DISABLE;
    hspi.Init.CRCPolynomial = atoi(crcpoly_str);

    if (HAL_OK != HAL_SPI_Init(&hspi)) 
    {
        sprintf(buf, "Failed to initialize %s\r\n", peripheral_name);
        sh_print(handle, buf);
        return;
    }

    sprintf(buf, "%s initialized successfully\r\n", peripheral_name);
    sh_print(handle, buf);
}

/**
  * @brief  initialize selected I2C peripheral
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
static void init_i2c(Shell_Handle_t *handle, const char* peripheral_name, ShellArg_t *args, int arg_count) 
{
    char buf[256];
    I2C_TypeDef* i2c_base = get_i2c_base(peripheral_name);
    if (NULL == i2c_base) 
    {
        sprintf(buf, "Invalid I2C instance: %s\r\n", peripheral_name);
        sh_print(handle, buf);
        return;
    }

    // Enable GPIO and I2C clocks
    if (i2c_base == I2C1) 
    {
        __HAL_RCC_I2C1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE(); // PB6/PB7
    } 
    else if (i2c_base == I2C2) 
    {
        __HAL_RCC_I2C2_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE(); // PB10/PB11
    }
    else if (i2c_base == I2C3) 
    {
        __HAL_RCC_I2C3_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE(); // PA8
        __HAL_RCC_GPIOC_CLK_ENABLE(); //PC9
    }
    
    
    // Get parameters with defaults
    const char* clkspeed_str = get_arg_value(args, arg_count, "clkspeed", "100000");
    const char* dc_str = get_arg_value(args, arg_count, "dc", "2");
    const char* ownaddr1_str = get_arg_value(args, arg_count, "ownaddr1", "0");
    const char* addrmode_str = get_arg_value(args, arg_count, "addrmode", "7");
    const char* dual_str = get_arg_value(args, arg_count, "dual", "disable");
    const char* ownaddr2_str = get_arg_value(args, arg_count, "ownaddr2", "0");
    const char* engc_str = get_arg_value(args, arg_count, "engc", "disable");
    const char* nostrech_str = get_arg_value(args, arg_count, "nostrech", "disable");

    // Configure GPIO
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    
    if (i2c_base == I2C1) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    else if (i2c_base == I2C2) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
        GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    else if (i2c_base == I2C3) 
    {
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    // Initialize I2C
    I2C_HandleTypeDef hi2c = {0};
    hi2c.Instance = i2c_base;
    hi2c.Init.ClockSpeed = atoi(clkspeed_str);
    hi2c.Init.DutyCycle = (strcmp(dc_str, "2") == 0) ? I2C_DUTYCYCLE_2 : I2C_DUTYCYCLE_16_9;
    hi2c.Init.OwnAddress1 = atoi(ownaddr1_str);
    hi2c.Init.AddressingMode = (strcmp(addrmode_str, "7") == 0) ? I2C_ADDRESSINGMODE_7BIT : I2C_ADDRESSINGMODE_10BIT;
    hi2c.Init.DualAddressMode = (strcmp(dual_str, "enable") == 0) ? I2C_DUALADDRESS_ENABLE : I2C_DUALADDRESS_DISABLE;
    hi2c.Init.OwnAddress2 = atoi(ownaddr2_str);
    hi2c.Init.GeneralCallMode = (strcmp(engc_str, "enable") == 0) ? I2C_GENERALCALL_ENABLE : I2C_GENERALCALL_DISABLE;
    hi2c.Init.NoStretchMode = (strcmp(nostrech_str, "enable") == 0) ? I2C_NOSTRETCH_ENABLE : I2C_NOSTRETCH_DISABLE;

    if (HAL_OK != HAL_I2C_Init(&hi2c)) 
    {
        sprintf(buf, "Failed to initialize %s\r\n", peripheral_name);
        sh_print(handle, buf);
        return;
    }

    sprintf(buf, "%s initialized successfully\r\n", peripheral_name);
    sh_print(handle, buf);
}

/**
  * @brief  initialize selected Timer peripheral
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
static void init_timer(Shell_Handle_t *handle, const char* peripheral_name, ShellArg_t *args, int arg_count) 
{
    char buf[256];
    TIM_TypeDef* timer_base = get_timer_base(peripheral_name);
    if (NULL == timer_base) 
    {
        sprintf(buf, "Invalid Timer instance: %s\r\n", peripheral_name);
        sh_print(handle, buf);
        return;
    }

    // Enable Timer clock
    if (timer_base == TIM1) __HAL_RCC_TIM1_CLK_ENABLE();
    else if (timer_base == TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
    else if (timer_base == TIM3) __HAL_RCC_TIM3_CLK_ENABLE();
    else if (timer_base == TIM4) __HAL_RCC_TIM4_CLK_ENABLE();
    else if (timer_base == TIM5) __HAL_RCC_TIM5_CLK_ENABLE();
    else if (timer_base == TIM6) __HAL_RCC_TIM6_CLK_ENABLE();
    else if (timer_base == TIM7) __HAL_RCC_TIM7_CLK_ENABLE();
    else if (timer_base == TIM8) __HAL_RCC_TIM8_CLK_ENABLE();
    else if (timer_base == TIM9) __HAL_RCC_TIM9_CLK_ENABLE();
    else if (timer_base == TIM10) __HAL_RCC_TIM10_CLK_ENABLE();
    else if (timer_base == TIM11) __HAL_RCC_TIM11_CLK_ENABLE();
    else if (timer_base == TIM12) __HAL_RCC_TIM12_CLK_ENABLE();
    else if (timer_base == TIM13) __HAL_RCC_TIM13_CLK_ENABLE();
    else if (timer_base == TIM14) __HAL_RCC_TIM14_CLK_ENABLE();

    // Get parameters with defaults
    const char* period_str = get_arg_value(args, arg_count, "period", "1000");
    const char* prescaler_str = get_arg_value(args, arg_count, "prescaler", "8400");
    const char* clockdiv_str = get_arg_value(args, arg_count, "clockdiv", "1");
    const char* countmode_str = get_arg_value(args, arg_count, "countmode", "up");
    const char* autoreload_str = get_arg_value(args, arg_count, "autoreload", "enable");
    const char* repcounter_str = get_arg_value(args, arg_count, "repcounter", "0");

    // Initialize Timer
    TIM_HandleTypeDef htim = {0};
    htim.Instance = timer_base;
    htim.Init.Period = atoi(period_str);
    htim.Init.Prescaler = atoi(prescaler_str);
    htim.Init.ClockDivision = (strcmp(clockdiv_str, "1") == 0) ? TIM_CLOCKDIVISION_DIV1 : 
                              (strcmp(clockdiv_str, "2") == 0) ? TIM_CLOCKDIVISION_DIV2 : TIM_CLOCKDIVISION_DIV4;
    htim.Init.CounterMode = (strcmp(countmode_str, "up") == 0) ? TIM_COUNTERMODE_UP : 
                            (strcmp(countmode_str, "down") == 0) ? TIM_COUNTERMODE_DOWN : 
                            (strcmp(countmode_str, "center1") == 0) ? TIM_COUNTERMODE_CENTERALIGNED1 :
                            (strcmp(countmode_str, "center2") == 0) ? TIM_COUNTERMODE_CENTERALIGNED2 : 
                            TIM_COUNTERMODE_CENTERALIGNED3;
    htim.Init.AutoReloadPreload = (strcmp(autoreload_str, "enable") == 0) ? TIM_AUTORELOAD_PRELOAD_ENABLE : 
                                  TIM_AUTORELOAD_PRELOAD_DISABLE;
    htim.Init.RepetitionCounter = atoi(repcounter_str);

    if (HAL_OK != HAL_TIM_Base_Init(&htim)) 
    {
        sprintf(buf, "Failed to initialize %s\r\n", peripheral_name);
        sh_print(handle, buf);
        return;
    }

    if (HAL_OK != HAL_TIM_Base_Start(&htim)) 
    {
        sprintf(buf, "Failed to start %s\r\n", peripheral_name);
        sh_print(handle, buf);
        return;
    }

    sprintf(buf, "%s initialized successfully\r\n", peripheral_name);
    sh_print(handle, buf);
}

/**
  * @brief  initialize RTC
  * @param handle shell handle
  * @param argc argument count
  * @param argv argument vector
  * @retval None
  */
static void init_rtc(Shell_Handle_t *handle, ShellArg_t *args, int arg_count) 
{
    char buf[256];

    // Enable PWR Clock
    __HAL_RCC_PWR_CLK_ENABLE();

    // Enable access to RTC
    HAL_PWR_EnableBkUpAccess();

    // Get parameters with defaults
    const char* clock_str = get_arg_value(args, arg_count, "clock", "lse");
    const char* format_str = get_arg_value(args, arg_count, "format", "24");
    const char* asyncpre_str = get_arg_value(args, arg_count, "asyncpre", "128");
    const char* syncpre_str = get_arg_value(args, arg_count, "syncpre", "256");

    // Configure RTC clock source
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    if (strcmp(clock_str, "lse") == 0) 
    {
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
        RCC_OscInitStruct.LSEState = RCC_LSE_ON;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    } 
    else if (strcmp(clock_str, "lsi") == 0) 
    {
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
        RCC_OscInitStruct.LSIState = RCC_LSI_ON;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    } 
    else if (strcmp(clock_str, "hse") == 0) 
    {
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV8;
    }

    if (HAL_OK != HAL_RCC_OscConfig(&RCC_OscInitStruct)) 
    {
        sh_print(handle, "Failed to configure RTC clock source\r\n");
        return;
    }

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    if (HAL_OK != HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct)) 
    {
        sh_print(handle, "Failed to configure RTC peripheral clock\r\n");
        return;
    }

    // Enable RTC Clock
    __HAL_RCC_RTC_ENABLE();

    // Initialize RTC
    RTC_HandleTypeDef hrtc = {0};
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = (strcmp(format_str, "24") == 0) ? RTC_HOURFORMAT_24 : RTC_HOURFORMAT_12;
    hrtc.Init.AsynchPrediv = atoi(asyncpre_str);
    hrtc.Init.SynchPrediv = atoi(syncpre_str);
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_OK != HAL_RTC_Init(&hrtc)) 
    {
        sh_print(handle, "Failed to initialize RTC\r\n");
        return;
    }

    sh_print(handle, "RTC initialized successfully\r\n");
}

/* Helper Functions */
static GPIO_TypeDef* get_gpio_port(const char *port_str) 
{
    if (strcmp(port_str, "gpioa") == 0 || strcmp(port_str, "portA") == 0 || strcmp(port_str, "A") == 0 || strcmp(port_str, "a") == 0) return GPIOA;
    if (strcmp(port_str, "gpiob") == 0 || strcmp(port_str, "portB") == 0 || strcmp(port_str, "B") == 0 || strcmp(port_str, "b") == 0) return GPIOB;
    if (strcmp(port_str, "gpioc") == 0 || strcmp(port_str, "portC") == 0 || strcmp(port_str, "C") == 0 || strcmp(port_str, "c") == 0) return GPIOC;
    if (strcmp(port_str, "gpiod") == 0 || strcmp(port_str, "portD") == 0 || strcmp(port_str, "D") == 0 || strcmp(port_str, "d") == 0) return GPIOD;
    if (strcmp(port_str, "gpioe") == 0 || strcmp(port_str, "portE") == 0 || strcmp(port_str, "E") == 0 || strcmp(port_str, "e") == 0) return GPIOE;
    return NULL;
}

static USART_TypeDef* get_usart_base(const char* uart_name) 
{
    if (0 == strcmp(uart_name, "USART1") || 0 == strcmp(uart_name, "UART1") || 0 == strcmp(uart_name, "usart1") || 0 == strcmp(uart_name, "uart1")) return USART1;
    if (0 == strcmp(uart_name, "USART2") || 0 == strcmp(uart_name, "UART2") || 0 == strcmp(uart_name, "usart2") || 0 == strcmp(uart_name, "uart2")) return USART2;
    if (0 == strcmp(uart_name, "USART3") || 0 == strcmp(uart_name, "UART3") || 0 == strcmp(uart_name, "usart3") || 0 == strcmp(uart_name, "uart3")) return USART3;
    if (0 == strcmp(uart_name, "USART4") || 0 == strcmp(uart_name, "UART4") || 0 == strcmp(uart_name, "usart4") || 0 == strcmp(uart_name, "uart4")) return UART4;
    if (0 == strcmp(uart_name, "USART5") || 0 == strcmp(uart_name, "UART5") || 0 == strcmp(uart_name, "usart5") || 0 == strcmp(uart_name, "uart5")) return UART5;
    if (0 == strcmp(uart_name, "USART6") || 0 == strcmp(uart_name, "UART6") || 0 == strcmp(uart_name, "usart6") || 0 == strcmp(uart_name, "uart6")) return USART6;
    return NULL;
}

static I2C_TypeDef* get_i2c_base(const char* i2c_name) 
{
    if (0 == strcmp(i2c_name, "i2c1") || 0 == strcmp(i2c_name, "I2C1")) return I2C1;
    if (0 == strcmp(i2c_name, "i2c2") || 0 == strcmp(i2c_name, "I2C2")) return I2C2;
    if (0 == strcmp(i2c_name, "i2c3") || 0 == strcmp(i2c_name, "I2C3")) return I2C3;
    return NULL;
}

static SPI_TypeDef* get_spi_base(const char* spi_name) 
{
    if (0 == strcmp(spi_name, "spi1") || 0 == strcmp(spi_name, "SPI1")) return SPI1;
    if (0 == strcmp(spi_name, "spi2") || 0 == strcmp(spi_name, "SPI2")) return SPI2;
    if (0 == strcmp(spi_name, "spi3") || 0 == strcmp(spi_name, "SPI3")) return SPI3;
    return NULL;
}

static TIM_TypeDef* get_timer_base(const char* timer_name) 
{
    if (0 == strcmp(timer_name, "tim1") || 0 == strcmp(timer_name, "TIM1")) return TIM1;
    if (0 == strcmp(timer_name, "tim2") || 0 == strcmp(timer_name, "TIM2")) return TIM2;
    if (0 == strcmp(timer_name, "tim3") || 0 == strcmp(timer_name, "TIM3")) return TIM3;
    if (0 == strcmp(timer_name, "tim4") || 0 == strcmp(timer_name, "TIM4")) return TIM4;
    if (0 == strcmp(timer_name, "tim5") || 0 == strcmp(timer_name, "TIM5")) return TIM5;
    if (0 == strcmp(timer_name, "tim6") || 0 == strcmp(timer_name, "TIM6")) return TIM6;
    if (0 == strcmp(timer_name, "tim7") || 0 == strcmp(timer_name, "TIM7")) return TIM7;
    if (0 == strcmp(timer_name, "tim8") || 0 == strcmp(timer_name, "TIM8")) return TIM8;
    if (0 == strcmp(timer_name, "tim9") || 0 == strcmp(timer_name, "TIM9")) return TIM9;
    if (0 == strcmp(timer_name, "tim10") || 0 == strcmp(timer_name, "TIM10")) return TIM10;
    if (0 == strcmp(timer_name, "tim11") || 0 == strcmp(timer_name, "TIM11")) return TIM11;
    if (0 == strcmp(timer_name, "tim12") || 0 == strcmp(timer_name, "TIM12")) return TIM12;
    if (0 == strcmp(timer_name, "tim13") || 0 == strcmp(timer_name, "TIM13")) return TIM13;
    if (0 == strcmp(timer_name, "tim14") || 0 == strcmp(timer_name, "TIM14")) return TIM14;
    return NULL;
}

static int parse_args(int argc, char *argv[], int start_index, ShellArg_t *args, int max_args) 
{
    int arg_count = 0;
    
    for (int i = start_index; i < argc && arg_count < max_args; i++) 
    {
        if ('-' == argv[i][0]) 
        {
            args[arg_count].flag = argv[i] + 1; 
            if (i + 1 < argc && argv[i + 1][0] != '-') 
            {
                args[arg_count].value = argv[i + 1];
                i++;  
            } 
            else 
            {
                // no value
                args[arg_count].value = NULL;
            }
            arg_count++;
        }
    }
    return arg_count;
}

static const char* get_arg_value(ShellArg_t *args, int arg_count, const char *flag, const char *default_value) 
{
    for (int i = 0; i < arg_count; i++) 
    {
        if (0 == strcmp(args[i].flag, flag)) 
        {
            return args[i].value ? args[i].value : default_value;
        }
    }
    return default_value;
}
