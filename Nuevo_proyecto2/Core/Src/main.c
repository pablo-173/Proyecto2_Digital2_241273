/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>

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
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t TxData[4] = {0};
uint8_t RxData[4] = {0};
typedef enum{
	MENU_PRINCIPAL,
	RECIBIR_COMANDO_SPI,
	LED,
	ESPERA,
} estado;
volatile estado modo=MENU_PRINCIPAL;
uint8_t led=0;
uint16_t tiempo=0;
char comando[20];
uint8_t indice=0;


volatile uint8_t dato;
uint8_t MostrarMenu=1;
#define UART_RX_SIZE 64
volatile uint8_t rxBuffer[UART_RX_SIZE];
volatile uint8_t rxHead = 0;
volatile uint8_t rxTail = 0;

uint8_t datosI2C[4] = {0};


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
int LeerUART(uint8_t *caracter);
void ComandoSPI(char*comando);
void SolicitarMedicionI2C(void);
void ActualizarTimer(uint16_t tiempoMs);
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
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(Esp32_GPIO_Port, Esp32_Pin, GPIO_PIN_SET);
  HAL_UART_Receive_IT(&huart2, (uint8_t*)&dato,1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	 if (modo==MENU_PRINCIPAL && MostrarMenu){
		 uint8_t TEST[]="\r\n===== MENU PRINCIPAL =====\r\n"
					 "1. Controlar dispositivo SPI\r\n"
					 "2. Obtener medicion sensor I2C\r\n"
					 "Seleccione una opcion: ";
		 HAL_UART_Transmit(&huart2,TEST, sizeof(TEST),1000);
		 MostrarMenu=0;
	 }
		 uint8_t caracter;
		 while (LeerUART(&caracter)){
			 switch(modo){

			 case MENU_PRINCIPAL:
				 if (caracter == '1'){
				 uint8_t TEST[]="\r\n===== CONTROL SPI =====\r\n"
							 "Ingrese Led y Tiempo (LED, TIEMPO):\r\n"
							 "EJEMPLO 1,500\r\n"
							 "Comando";
				 HAL_UART_Transmit(&huart2,TEST, sizeof(TEST),1000);
				 modo = RECIBIR_COMANDO_SPI;

				 }
				 else if (caracter=='2'){
					 SolicitarMedicionI2C();

					    modo = MENU_PRINCIPAL;
					    MostrarMenu = 1;
				 }
				 break;
			 case RECIBIR_COMANDO_SPI:
				 if (caracter == '\r' || caracter == '\n')
				                {
					if (indice > 0)
					{
						comando[indice] = '\0';

						ComandoSPI(comando);

						indice = 0;
						MostrarMenu=1;
						modo=MENU_PRINCIPAL;
						uint8_t TEST[] =
							"\r\nIngrese otro comando (LED,TIEMPO): ";

						HAL_UART_Transmit(&huart2, TEST, sizeof(TEST)-1, 1000);
					}
				}
				 	 else
				 	 	 {
				 		 if (indice < sizeof(comando)-1)
				 		 	 {
				 			 comando[indice] = caracter;
				 			 indice++;
				 		 	 }
				 		 else
				 		 {
				 			 indice = 0;

						uint8_t error[] =
							"\r\nComando demasiado largo\r\n";

						HAL_UART_Transmit(&huart2, error, sizeof(error)-1, 1000);
					}
				}

				        break;
			 case LED:
				 break;
			 case ESPERA:
				 break;

		 }
		 }


	 }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LedC_GPIO_Port, LedC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Esp32_GPIO_Port, Esp32_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LedC_Pin Esp32_Pin */
  GPIO_InitStruct.Pin = LedC_Pin|Esp32_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int LeerUART(uint8_t *caracter)
{
    if (rxTail == rxHead)
    {
        return 0;
    }

    *caracter = rxBuffer[rxTail];

    rxTail = (rxTail + 1) % UART_RX_SIZE;

    return 1;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        uint8_t siguiente =
            (rxHead + 1) % UART_RX_SIZE;

        if (siguiente != rxTail)
        {
            rxBuffer[rxHead] = dato;
            rxHead = siguiente;
        }

        // Volver a habilitar la recepcion
        HAL_UART_Receive_IT( &huart2,(uint8_t *)&dato,1);
    }
}
void ComandoSPI(char *comando)
{
    unsigned int ledTemp;
    unsigned int tiempoTemp;
    char extra;
    char mensaje[100];

    if (sscanf(comando, " %u , %u %c",
               &ledTemp, &tiempoTemp, &extra) != 2)
    {
        uint8_t error[] = "\r\nComando invalido\r\n";

        HAL_UART_Transmit(&huart2, error,
                          sizeof(error)-1, 1000);
        return;
    }

    if (ledTemp < 1 || ledTemp > 4 ||
        tiempoTemp < 1 || tiempoTemp > 60000)
    {
        uint8_t error[] = "\r\nLED o tiempo invalido\r\n";

        HAL_UART_Transmit(&huart2, error,
                          sizeof(error)-1, 1000);
        return;
    }

    // Guardar los valores recibidos
    led = (uint8_t)ledTemp;
    tiempo = (uint16_t)tiempoTemp;

    // Preparar trama SPI de 4 bytes
    TxData[0] = 0xAA;
    TxData[1] = led;
    TxData[2] = (tiempo >> 8) & 0xFF;
    TxData[3] = tiempo & 0xFF;

    // Mostrar los datos antes de transmitir
    snprintf(mensaje, sizeof(mensaje),
             "\r\nSPI TX: %u, %u, %u, %u\r\n",
             TxData[0], TxData[1],
             TxData[2], TxData[3]);

    HAL_UART_Transmit(&huart2, (uint8_t*)mensaje,
                      strlen(mensaje), 1000);

    // Activar esclavo
    HAL_GPIO_WritePin(Esp32_GPIO_Port,
                      Esp32_Pin, GPIO_PIN_RESET);

    // Transmitir los 4 bytes
    HAL_StatusTypeDef resultado =
        HAL_SPI_TransmitReceive(&hspi1,
                                TxData,
                                RxData,
                                4,
                                1000);

    // Desactivar esclavo
    HAL_GPIO_WritePin(Esp32_GPIO_Port,
                      Esp32_Pin, GPIO_PIN_SET);

    if (resultado == HAL_OK)
    {
        snprintf(mensaje, sizeof(mensaje),
                 "SPI enviado: LED %u, Tiempo %u ms\r\n",
                 led, tiempo);
        ActualizarTimer(tiempo);
        HAL_UART_Transmit(&huart2, (uint8_t*)mensaje,
                          strlen(mensaje), 1000);
    }
    else
    {
        uint8_t error[] = "\r\nError al transmitir SPI\r\n";

        HAL_UART_Transmit(&huart2, error,
                          sizeof(error)-1, 1000);
    }
}
void SolicitarMedicionI2C(void)
{
    uint8_t comando = 0x01;
    uint8_t datoViejo[4] = {0};
    char mensaje[100];

    // Solicitar nueva medicion al ESP32
    if (HAL_I2C_Master_Transmit(&hi2c1, 0x28 << 1,
                                &comando, 1, 100) != HAL_OK)
    {
        uint8_t error[] = "\r\nError solicitando I2C\r\n";

        HAL_UART_Transmit(&huart2, error,
                          sizeof(error)-1, 1000);
        return;
    }

    // Primera lectura: descartar respuesta anterior
    if (HAL_I2C_Master_Receive(&hi2c1, 0x28 << 1,
                               datoViejo, 4, 100) != HAL_OK)
    {
        uint8_t error[] = "\r\nError primera lectura I2C\r\n";

        HAL_UART_Transmit(&huart2, error,
                          sizeof(error)-1, 1000);
        return;
    }

    // Esperar a que el ESP32 actualice la medicion
    for (int intento = 0; intento < 10; intento++)
    {
        HAL_Delay(20);

        // Recibir la medicion actualizada
        if (HAL_I2C_Master_Receive(&hi2c1, 0x28 << 1,
                                   datosI2C, 4, 100) != HAL_OK)
        {
            uint8_t error[] = "\r\nError recibiendo I2C\r\n";

            HAL_UART_Transmit(&huart2, error,
                              sizeof(error)-1, 1000);
            return;
        }

        // Verificar que la medicion este lista
        if (datosI2C[0] == 1)
        {
            uint8_t lectura = datosI2C[1];

            uint16_t mv =
                ((uint16_t)datosI2C[2] << 8) |
                datosI2C[3];

            snprintf(mensaje, sizeof(mensaje),
                     "\r\n===== MEDICION I2C =====\r\n"
                     "Valor ADC (0-255): %u\r\n"
                     "Voltaje estimado: %u mV\r\n",
                     lectura, mv);

            HAL_UART_Transmit(&huart2,
                              (uint8_t*)mensaje,
                              strlen(mensaje),
                              1000);

            return;
        }
    }

    uint8_t error[] = "\r\nTiempo de espera I2C agotado\r\n";

    HAL_UART_Transmit(&huart2, error,
                      sizeof(error)-1, 1000);
}
void ActualizarTimer(uint16_t tiempoMs)
{
    if (tiempoMs == 0 || tiempoMs > 60000)
    {
        return;
    }

    // Detener el timer anterior
    HAL_TIM_Base_Stop_IT(&htim2);

    // Calcular periodo
    uint32_t ticks = (uint32_t)tiempoMs * 10U;

    // Actualizar el timer
    __HAL_TIM_SET_AUTORELOAD(&htim2, ticks - 1U);

    // Reiniciar contador
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    // Limpiar interrupcion pendiente
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);

    // Comenzar con salida en HIGH
    HAL_GPIO_WritePin(LedC_GPIO_Port,
                      LedC_Pin,
                      GPIO_PIN_SET);

    // Iniciar timer con interrupciones
    HAL_TIM_Base_Start_IT(&htim2);
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        HAL_GPIO_TogglePin(LedC_GPIO_Port,
                           LedC_Pin);
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
#ifdef USE_FULL_ASSERT
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
