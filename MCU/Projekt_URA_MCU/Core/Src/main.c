/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2021 STMicroelectronics.
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
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "BMPXX80.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "SSD1306_OLED.h"
#include "GFX_BW.h"
#include "fonts/fonts.h"
#include "PID_Controller.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STATUS_GRZANIE 1
#define STATUS_CHLODZENIE 2
#define STATUS_ERROR 3
#define STATUS_STOP 4
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
//
//Dane z czujnika pomiarowego
//
int32_t pressure = 0;
float temperature = 0;
float temperature_set = 35;
//
// OLED Print
//
char Message[32];
uint8_t status = STATUS_STOP;
uint32_t SoftTimerOled;
//
//Regulator PID
//
pid_controller_t pid={.Kp = 0.52, .Ki = 0.003, .Kd = 0.257, .dt = 1, .calka_poprzedni = 0, .uchyb_aktualny = 0, .uchyb_poprzedni = 0};
float u;
uint16_t sterowanie;
//
//UART - wysyłanie danych
//
uint8_t Buffor[128], Buffor_Rx[6];
uint8_t zezwolenie = 0;
uint16_t length;
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
	MX_USART3_UART_Init();
	MX_USB_OTG_FS_PCD_Init();
	MX_I2C1_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
	MX_SPI1_Init();
	/* USER CODE BEGIN 2 */
	//
	//Włączenie TIM
	//
	HAL_TIM_Base_Start_IT(&htim2); //Podstawowy do dt = 1s
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); //Sterowanie transytorem 1kHz

	//
	//Ustawienie czujnika Temperatury
	//
	BMP280_Init(&hspi1, BMP280_TEMPERATURE_16BIT, BMP280_STANDARD, BMP280_FORCEDMODE);

	//
	// Ustawienie Wyświetlacza OLED
	//
	SSD1306_Init(&hi2c1);
	GFX_SetFont(font_8x5);
	GFX_SetFontSize(1);
	SSD1306_Clear(BLACK);
	SSD1306_Display();
	//
	//Pobranie danych z czujnika - wartości początkowe
	//
	BMP280_ReadTemperatureAndPressure(&temperature, &pressure);
	//
	//Otworzenie portu do nasłuchu na przychodzące komendy
	//
	HAL_UART_Receive_IT(&huart3, Buffor_Rx, 8);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		//
		//Wyświetlanie danych na wyświetlaczu OLED
		//
		if((HAL_GetTick() - SoftTimerOled) > 100){
			SoftTimerOled = HAL_GetTick();
			SSD1306_Clear(BLACK);
			sprintf(Message, "Regulacja Temperatury");
			GFX_DrawString(0, 0, Message, WHITE, 0);
			GFX_DrawLine(0, 9, 128, 9, WHITE);
			sprintf(Message, "Aktualna temp: %.2fC", temperature);
			GFX_DrawString(0, 12, Message, WHITE, 0);
			sprintf(Message, "Zadana temp: %.1f C", temperature_set);
			GFX_DrawString(0, 22, Message, WHITE, 0);
			sprintf(Message, "Cisnienie: %.1f HPa", ((float)pressure/100.0));
			GFX_DrawString(0, 32, Message, WHITE, 0);
			switch (status){
			case 1:
				sprintf(Message, "Status: Grzanie");
				GFX_DrawString(0, 42, Message, WHITE, 0);
				break;
			case 2:
				sprintf(Message, "Status: Chlodzenie");
				GFX_DrawString(0, 42, Message, WHITE, 0);
				break;
			case 3:
				sprintf(Message, "Status: ERROR!!!");
				GFX_DrawString(0, 42, Message, WHITE, 0);
				break;
			case 4:
				sprintf(Message, "Status: STOP");
				GFX_DrawString(0, 42, Message, WHITE, 0);
				break;
			}
			GFX_DrawLine(127, 53, 127, 64, WHITE);
			GFX_DrawRectangle(0, 53, (uint16_t)(128*temperature/temperature_set), 11, WHITE);
			SSD1306_Display();
		}
		if(zezwolenie){
			length = sprintf((char*)Buffor, "{\"Temp\":%.2f,\"t\":%.1f,\"Temp_set\":%.2f,\"Kp\":%.4f,\"Ki\":%.4f,\"Kd\":%.4f,\"u\":%.2f}\r\n",temperature,pid.dt,temperature_set,pid.Kp,pid.Ki,pid.Kd,u);
			HAL_UART_Transmit(&huart3, Buffor, length, 1000);
			zezwolenie = 0;
		}
		/* USER CODE END WHILE */

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

	/** Configure LSE Drive Capability
	 */
	HAL_PWR_EnableBkUpAccess();
	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 4;
	RCC_OscInitStruct.PLL.PLLN = 72;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 3;
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

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM2 && (status == STATUS_GRZANIE || status == STATUS_CHLODZENIE))
	{
		//Sygnalizacja rozpoczęcia
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		//
		//Pobranie danych z czujnika - aktualna temp i press
		//
		BMP280_ReadTemperatureAndPressure(&temperature, &pressure);
		//
		//Regulator
		//

		u = u_pid_calculate(&pid, temperature_set, temperature);
		sterowanie = saturation_pwm(u);
		if(sterowanie<100)
		{
			status = STATUS_CHLODZENIE;
		}else{
			status = STATUS_GRZANIE;
		}

		//
		//Wytworzenie sygnału
		//
		__HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_3,sterowanie);
		//
		//Wysłanie danych
		//
		zezwolenie = 1;
		//Sygnalizacja zakończenia
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	}
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART3)
	{
		//Sygnalizacja odebrania
		HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		//
		//Wykonanie akcji
		//
		//Możliwe komunikaty:
		//"TMP=27.5" lub dowolna inna temp
		//"STA=0001" lub "STA=0002" lub "STA=0003" lub "STA=0004"
		//
		if(Buffor_Rx[0] == 'S'){
			switch(Buffor_Rx[7]){
			case '1':
				status = STATUS_GRZANIE;
				break;
			case '2':
				status = STATUS_CHLODZENIE;
				break;
			case '3':
				status = STATUS_ERROR;
				break;
			case '4':
				status = STATUS_STOP;
				break;
			}
			}else if(Buffor_Rx[0] == 'T'){
				float temp_change;
				char temp_change_str[4];
				temp_change_str[0] = Buffor_Rx[4];
				temp_change_str[1] = Buffor_Rx[5];
				temp_change_str[2] = Buffor_Rx[6];
				temp_change_str[3] = Buffor_Rx[7];
				temp_change = atof(temp_change_str);
				temperature_set = temp_change;
			}

		//Sygnalizacja zakończenia
		HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		//
		//Ponowne ustawienie nasłuchu
		//
		HAL_UART_Receive_IT(&huart3, Buffor_Rx, 8);
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

