/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Neighborhood Database resident information
struct Residents {
	char fname[10];			// first name
	char lname[10];			// last name
	char vmake[10];			// vehicle make
	char vmodel[10];		// vehicle model
	char plate[6];			// license plate
	int row;				// data file row
} NHD[5];
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
// USB variables
extern ApplicationTypeDef Appli_state;			// USB drive's state

// FATFS Variables
FATFS usbFATFS;									// File system object
extern char USBHPath[4];						// USBH logical drive path

// Filename strings
char nhd_fn[10] = "Residents.csv";				// Residents data file
char img_fn[10];								// Variable for image writing

// File IO Variables
FIL fo;											// File object
FRESULT res;									// Function return result
UINT byteswritten, bytesread;					// Number bytes read/written
char rwbuf[100];								// read/write character buffer

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

// Neighborhood Database functions
bool update_NHD(int, int, int);		//
// TODO: find_resident					// Match plate detection result with NHD

// File read / write functions
bool USB_read(FIL, char*);			//
// TODO: bool USB_write(?);				// Write output images

// Communication functions
// TODO: SPI1_get_image					// receive image via SPI1
// TODO: SPI2_get_image					// receive image via SPI2
// TODO: SPI2_get_plate					// receive plate detection result via SPI2
// TODO: GPIO_sleep						// Set pins to tell RPi's to sleep
// TODO: GPIO_wake(?);					// Reset pins to tell RPi's to wake up

// Testing functions
bool USB_readT(void);				//
bool USB_writeT(void);				//
void twrite(void);					//
void tread(void);					//

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*******************************************************************************
 * Function:
 * 		void update_NHD(int row, int col, int cell_length)
 *
 * Description:
 * 		When reading the csv file, this function is called to choose
 * 		which field of the Residents structure to edit and write the
 * 		contents of the read/write buffer to it.
 *
 * Arguments:
 * 		- int row:
 * 			The row of csv file, each row representing a different resident.
 * 		- int col:
 * 			The column of csv file, each column representing a different
 * 			field of data for a given resident.
 * 		- int cell_length:
 * 			Number of characters read from the csv file, used to iterate
 * 			through the read write buffer.
 *
 * Return: bool
 * 		Represents whether or not there is an error with the Neighborhood
 * 		Database reference values. (row / col)
 ******************************************************************************/
bool update_NHD(int row, int col, int cell_length)
{
	// Check if column dimensions are valid
	if ((col < 0) || (col > 4))
		return 0;

	// Check if row dimensions are valid
	if ((row < 0) || (row > 4))
		return 0;

	// Check if cell length dimensions are valid
	if ((cell_length < 0) || (cell_length > 99))
		return 0;

	// Update NHD field(s)
	switch (col) {
	case 0:		// first name and row number
		for (uint8_t i = 0; i < cell_length; i++)
			NHD[row].fname[i] = rwbuf[i];
		NHD[row].row = row;
		break;

	case 1:		// last name
		for (uint8_t i = 0; i < cell_length; i++)
			NHD[row].lname[i] = rwbuf[i];
		break;

	case 2:		// vehicle make
		for (uint8_t i = 0; i < cell_length; i++)
			NHD[row].vmake[i] = rwbuf[i];
		break;

	case 3:		// vehicle model
		for (uint8_t i = 0; i < cell_length; i++)
			NHD[row].vmodel[i] = rwbuf[i];
		break;

	case 4:		// license plate
		for (uint8_t i = 0; i < cell_length; i++)
			NHD[row].plate[i] = rwbuf[i];
		break;
	}

	return 1;
}

/*******************************************************************************
 * Function: bool USB_read(FIL file_object, char * file_name)
 *
 * Description:
 * 		For the Residents.csv file input, this function will read the csv
 * 		file into the Residents data structure.
 *
 * Arguments:
 * 		- FIL file_object:
 * 			FatFs file object
 * 		- char * file_name:
 * 			character string of the filename to be read
 *
 * Return: bool
 * 		Represents whether or not file reading was successful.
 ******************************************************************************/
bool USB_read(FIL file_object, char * file_name)
{
	// Initialize variables for reading csv file.
	bool reading = 1;	// tells while loop when cell reading is over
	bool update_res;	// result from updating a value in NHD
	int cell_length;	// number of characters per cell
	int col = -1;		// column of csv file (leftmost is 0)
	int row = -1;		// row of csv file (topmost is 0

	// Open a file for reading
	if (f_open(&file_object, file_name, FA_READ) != FR_OK)
		return 0;		// error opening file

	// Read csv file one cell at a time.
	while (reading)
	{
		// set/reset/update indexing variables
		cell_length = 0;
		row += 1;
		col = -1;

		// Read cell one byte at a time to check for identifier characters.
		for (uint8_t i = 0; i < 100; i++)
		{
			res = f_read(&file_object, (uint8_t *)&rwbuf[i], 1, &bytesread);

			if (rwbuf[i] == 0x2C)			// comma
			{
				cell_length = i + 1;
				col += 1;
				break;
			}
			else if (rwbuf[i] == "\n")		// new line
			{
				cell_length = i + 1;
				break;
			}
			else if (rwbuf[i] == 0x00)		// NULL
			{
				cell_length = i + 1;
				reading = 0;
				break;
			}
		}

		if (!update_NHD(row, col, cell_length))
			return 0;		// error updating NHD

	}

	// Check for read error
	if ((row == 0) || (col == 0))
		return 0;		// error reading file

	f_close(&file_object);
	return 1;
}

// Test USB reading function
bool USB_readT(void)
{
	// Open a file for reading
	if (f_open(&myFile, "TEST.txt", FA_READ) != FR_OK)
	{
		return 0;		// error opening file
	}

	// Read text from file (stop at NULL)
	for (uint8_t i = 0; i < 100; i++)
	{
		res = f_read(&myFile, (uint8_t *)&rwbuf[i], 1, &bytesread);
		if (rwbuf[i] == 0x00)
		{
			bytesread = i;
			break;
		}
	}

	// Check for file read error
	if (bytesread == 0)
	{
		return 0;		// error reading file
	}

	// Close file
	f_close(&myFile);
	return 1;			// file reading successful
}

// Test USB writing function
bool USB_writeT(void)
{
	// Open a file for writing
	if (f_open(&myFile, "TEST.txt", FA_WRITE | FA_CREATE_ALWAYS) != FF_OK)
	{
		return 0;		// error opening file
	}

	// Write text to file
	sprintf(rwbuf, "Hello?\n*TAPTAPTAP*\nIs this thing on?");		// test text
	res = f_write(&myFile, (uint8_t *)rwbuf, strlen(rwbuf), &byteswritten);

	// Check for file write error
	if ((byteswritten == 0) || (res != FR_OK))
	{
		return 0;		// error writing to file
	}

	// Close file
	f_close(&myFile);
	return 1;			// file writing successful
}

// if statement for testing USB reading function
void tread(void)
{
	if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, Button_Pin) == GPIO_PIN_SET)
	{
		if (USB_read())	// blue LED on success
			HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
		else			// red LED on error
			HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);

		HAL_Delay(1000);
	}
}

// if statement for testing USB writing function
void twrite(void)
{
	if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, Button_Pin) == GPIO_PIN_SET)
	{
		if (USB_write())	// blue LED on success
			HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
		else				// red LED on error
			HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);

		HAL_Delay(1000);
	}
}
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
  MX_FATFS_Init();
  MX_USB_HOST_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  int update_NHD = 1;			// flag for when NHD needs to be updated

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
    // USB drive status
    switch (Appli_state)
    {
    case APPLICATION_IDLE:
    	break;

    case APPLICATION_START:
    	if (f_mount(&usbFATFS, (TCHAR const*)USBHPath, 0) == FR_OK)
    	{
    		// Turn green LED on
    		HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    	}
    	break;

    case APPLICATION_READY:
    	// Check if Neighborhood Database needs to be updated
    	if (update_NHD)
    	{	// Update Neighborhood Database
    		if (USB_read(fo, nhd_fn))
    		{	// csv file read successfully
    			HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
    			update_NHD = 0;
    		}
    		else
    		{	// error reading csv file
    			HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
    		}
    	}
    	//tread();		// Test USB_write() when pressing push button
    	//twrite();		// Test USB_write() when pressing push button
    	break;

    case APPLICATION_DISCONNECT:
    	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);	// green LED off
    	HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);		// red LED on
    	update_NHD = 1;													// update_NHD flag on
    	break;
    }
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
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
  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi1.Init.Direction = SPI_DIRECTION_1LINE;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : Button_Pin */
  GPIO_InitStruct.Pin = Button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Button_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin LED2_Pin LED3_Pin LED4_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin|LED3_Pin|LED4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
