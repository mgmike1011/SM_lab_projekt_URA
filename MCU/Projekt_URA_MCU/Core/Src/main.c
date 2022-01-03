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
float temperature_set = 0;
//
// OLED Print
//
char Message[32];
uint8_t status = 4;
uint32_t SoftTimerOled;
//
//Regulator PID
//
pid_controller_t pid={.Kp = 1, .Ki = 0.002, .Kd = 0, .dt = 1, .calka_poprzedni = 0, .uchyb_aktualny = 0, .uchyb_poprzedni = 0};
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
	/* USER CODE BEGIN 2 */
	//
	//Ustawienie czujnika Temperatury
	//
	//BMP280_Init(&hi2c1, BMP280_TEMPERATURE_16BIT, BMP280_STANDARD, BMP280_FORCEDMODE);

	//
	// Ustawienie Wyświetlacza OLED
	//
	SSD1306_Init(&hi2c1);
	GFX_SetFont(font_8x5);
	GFX_SetFontSize(1);
	SSD1306_Clear(BLACK);
	SSD1306_Display();
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		//
		//Pobranie danych z czujnika - aktualna temp i press
		//		(DOCELOWO W CALLBACK)
		//	  BMP280_ReadTemperatureAndPressure(&temperature, &pressure);

		//
		//Wyświetlanie danych na wyświetlaczu OLED
		//
		if((HAL_GetTick() - SoftTimerOled) > 100){
			SoftTimerOled = HAL_GetTick();
			SSD1306_Clear(BLACK);
			sprintf(Message, "Regulacja Temperatury");
			GFX_DrawString(0, 0, Message, WHITE, 0);
			GFX_DrawLine(0, 9, 128, 9, WHITE);
			sprintf(Message, "Aktualna temp: %.2f C", temperature);
			GFX_DrawString(0, 12, Message, WHITE, 0);
			sprintf(Message, "Zadana temp: %.2f C", temperature_set);
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
			SSD1306_Display();
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

