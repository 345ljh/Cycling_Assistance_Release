/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "inputmethod_app.h"
#include "route_app.h"
#include "keyarray.h"
#include "tlv493d.h"
#include "bsp_log.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
InputMethod* inputmethod;
W25Q* w25q;
TFT* tft;
KeyArray* keyarray;

GPS* gps;
ESP32* esp32;

InputMethodApp* inputmethodapp;
RouteApp* routeapp;

// 前台应用
void* foreground_app;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM5_Init();
  MX_USART1_UART_Init();
  MX_TIM6_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */
  BSP_Log_Init();
  // HAL Layer init
  GPIO_TypeDef* row_gpio[KEY_ROW] = {GPIOC, GPIOC, GPIOC};
  uint16_t row_pin[KEY_ROW] = {1 << 10, 1 << 11, 1 << 12};
  GPIO_TypeDef* col_gpio[KEY_COL] = {GPIOD, GPIOB, GPIOB, GPIOB};
  uint16_t col_pin[KEY_COL] = {1 << 2, 1 << 3, 1 << 4, 1 << 5};
  keyarray = KeyArray_Init(row_gpio, row_pin, col_gpio, col_pin);

  w25q = W25Q_Init(&hspi2, GPIOB, 1 << 12);
  inputmethod = InputMethod_Init();
  gps = GPS_Init(&huart1, &htim6);
  esp32 = ESP32_Init(&huart2);
  tft = TFT_Init(&hspi1, GPIOC, 1 << 4, GPIOB, 1 << 0, GPIOC, 1 << 5, GPIOA, 1 << 6);
  TFT_HardInit(tft);

  // APP Layer init
  inputmethodapp = InputMethodApp_Init(inputmethod, w25q, tft);
  routeapp = RouteApp_Init(&htim7, &hadc1, tft, gps, esp32, inputmethod, w25q);

  gps->data.latitude = 22.581946;
  gps->data.longitude = 113.965307;
  
  foreground_app = inputmethodapp;
  InputMethodApp_UpdateInput(inputmethodapp);
  InputMethodApp_UpdateSelection(inputmethodapp);
  InputMethodApp_UpdateTyped(inputmethodapp);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    if(routeapp->esp32->rx_data_temp.flag == 1 || routeapp->esp32->rx_data_temp.flag == 2){  // 规划失败
      RouteApp_ShowError(routeapp);
      routeapp->esp32->rx_data_temp.flag = 0;
    }
    if(routeapp->esp32->rx_data_temp.flag == 255){  // 规划成功
      routeapp->mileage = 0;
      routeapp->zoom_rate = 1;
      routeapp->zoom_rate_temp = 1;
      routeapp->show_center = routeapp->esp32->rx_data_temp.routepoint.center;
      TFT_Fill(routeapp->tft, 0, 0, 320, 241, 0);
      RouteApp_SetStatus(routeapp);
      foreground_app = routeapp;
      routeapp->esp32->rx_data_temp.flag = 254;
    }
    if(routeapp->esp32->rx_data_temp.flag == 254){  // 手动修改zoom大小, 或是规划成功后初始化路径显示
      RouteApp_SetRoute(routeapp);
      routeapp->esp32->rx_data_temp.flag = 0;
    }
    HAL_Delay(200);
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
  uint8_t keydown = 0;
  for(int i = 0; i < 4; i++){
    if(GPIO_Pin == keyarray->col_pin[i]){
      keydown = 1;
      break;
    }
  }
  if(keydown){ // pin = 2/3/4/5
    if(HAL_GetTick() - keyarray->last_irq_time < 100) return;  // 限制中断触发频率
    KeyArray_Judge(keyarray, GPIO_Pin);

    switch(keyarray->now_key){
      case 0:
        if(foreground_app == inputmethodapp) InputMethodApp_Delete(inputmethodapp);
        break;        
      case 1:
        if(foreground_app == inputmethodapp) InputMethodApp_Up(inputmethodapp);
        if(foreground_app == routeapp && !gps->watchdog_count) gps->data.latitude += GPS_OFFLINE_ADJUST_STEP;
        break;
      case 2:
        if(foreground_app == inputmethodapp) InputMethodApp_Ascii(inputmethodapp);
        if(foreground_app == routeapp){
          routeapp->zoom_rate_temp *= 1.2589254f;  // 2dB
          if(routeapp->zoom_rate_temp >= 10) routeapp->zoom_rate_temp = 10;
        }
        break;
      case 3:
        if(foreground_app == inputmethodapp) RouteApp_SendMessage(routeapp);
        break;
      case 4:
        if(foreground_app == inputmethodapp) InputMethodApp_Prev(inputmethodapp);
        if(foreground_app == routeapp && !gps->watchdog_count) gps->data.longitude -= GPS_OFFLINE_ADJUST_STEP;
        break;
      case 5:
        if(foreground_app == inputmethodapp) InputMethodApp_Ensure(inputmethodapp);
        if(foreground_app == routeapp){
          routeapp->zoom_rate = routeapp->zoom_rate_temp;
          // 直接在这里RouteApp_SetRoute(routeapp);会导致GPS数据解析不正常, 疑似中断执行时间过长导致
          routeapp->esp32->rx_data_temp.flag = 254;  // 这里直接使用了esp32的接收flag, 方便第一次进入routeapp时复用函数
        }
        break;
      case 6:
        if(foreground_app == inputmethodapp) InputMethodApp_Next(inputmethodapp);
        if(foreground_app == routeapp && !gps->watchdog_count) gps->data.longitude += GPS_OFFLINE_ADJUST_STEP;
        break;
      case 7:
        if(foreground_app == routeapp){
            foreground_app = inputmethodapp;
            TFT_Fill(routeapp->tft, 0, 30, 320, 41, 0);
            InputMethodApp_UpdateInput(inputmethodapp);
            InputMethodApp_UpdateSelection(inputmethodapp);
            InputMethodApp_UpdateTyped(inputmethodapp);
        };
        break;
      case 8:
        if(foreground_app == inputmethodapp) InputMethodApp_Clear(inputmethodapp);
        break;
      case 9:
        if(foreground_app == inputmethodapp) InputMethodApp_Down(inputmethodapp);
        if(foreground_app == routeapp && !gps->watchdog_count) gps->data.latitude -= GPS_OFFLINE_ADJUST_STEP;
        break;
      case 10:
        if(foreground_app == routeapp){
          routeapp->zoom_rate_temp /= 1.2589254f;
          if(routeapp->zoom_rate_temp <= 1) routeapp->zoom_rate_temp = 1;
        }
      case 11:
        if(foreground_app == inputmethodapp) InputMethodApp_Switch(inputmethodapp);
        break;
      }
    keyarray->last_irq_time = HAL_GetTick();
  }
}

// 串口空闲中断
void My_UART_IRQHandler(UART_HandleTypeDef *huart){
  if(huart == gps->huart){
    // 判断是否为空闲中断
    if(RESET != __HAL_UART_GET_FLAG(gps->huart, UART_FLAG_IDLE)){
      GPS_RxCallback(gps);
    }
  }
  if(huart == esp32->huart){
    if(RESET != __HAL_UART_GET_FLAG(esp32->huart, UART_FLAG_IDLE)){
      ESP32_RxCallback(esp32);
    }
  }
}

// 定时器中断
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
  if(htim == gps->watchdog_htim){
    // GPS看门狗, 周期50ms
    // 5s未收到有效数据认为失去信号
    if(gps->watchdog_count){
        gps->watchdog_count--;
    } else gps->online_count = 0;
  }
  if(htim == routeapp->update_htim){
    if(foreground_app == routeapp){
      routeapp->update_count++;
      routeapp->update_count %= 100;  // 20 -> 1s
      if(routeapp->update_count % 20 == 0) {
        Point pos = {gps->data.longitude, gps->data.latitude};
        if(RouteApp_IsOutRange(routeapp, pos)){
          routeapp->show_center = pos;
          RouteApp_SetRoute(routeapp);
        }
        else RouteApp_UpdateRoute(routeapp);
        RouteApp_UpdateStatus(routeapp);      
      }
    }
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
