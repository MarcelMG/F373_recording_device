/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <string.h>
#include "fifo.h" // my custom "library" for FIFO ring-buffer
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//#define DEBUG // enable debug output
#define BUF_LEN 1000 // length (in Bytes) of a USB packet

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SDADC_HandleTypeDef hsdadc1;
SDADC_HandleTypeDef hsdadc2;
SDADC_HandleTypeDef hsdadc3;
DMA_HandleTypeDef hdma_sdadc1;
DMA_HandleTypeDef hdma_sdadc2;
DMA_HandleTypeDef hdma_sdadc3;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
// adc data buffers
uint16_t sdadc1_data_buf[BUF_LEN];
uint16_t sdadc2_data_buf[BUF_LEN];
uint16_t sdadc3_data_buf[BUF_LEN];
// flags
volatile bool sdadc1_half_complete = false;
volatile bool sdadc1_full_complete = false;
volatile bool sdadc2_half_complete = false;
volatile bool sdadc2_full_complete = false;
volatile bool sdadc3_half_complete = false;
volatile bool sdadc3_full_complete = false;
volatile bool recording_trigger = false;
// register configuration values for the ADA4350 amplifier
const uint32_t ADA4350_SDO_DISABLE = (1<<14);
const uint32_t ADA4350_M1_DISABLE = (1<<15);
const uint32_t ADA4350_FB0_INT_CAP = 0x00002041;
const uint32_t ADA4350_FB0 = 0x00000041;
const uint32_t ADA4350_FB1 = 0x00000082;
const uint32_t ADA4350_FB2 = 0x00000104;
const uint32_t ADA4350_FB3 = 0x00000208;
const uint32_t ADA4350_FB4 = 0x00000410;
const uint32_t ADA4350_FB5 = 0x00000820;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SDADC1_Init(void);
static void MX_SDADC2_Init(void);
static void MX_SDADC3_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// select one of the 6 gain paths (gain factors) of the ADA4350 amplifier (gain path 0-5)
// return the 24bit configuration that has to be written to the ADA4350 register
// to configure the gain path
uint32_t ADA4350_parse_config(uint8_t gain_path){
	switch(gain_path){
	case 0:
		return ADA4350_FB0_INT_CAP;
	case 1:
		return ADA4350_FB1;
	case 2:
		return ADA4350_FB2;
	case 3:
		return ADA4350_FB3;
	case 4:
		return ADA4350_FB4;
	case 5:
		return ADA4350_FB5;
	default:
		return ADA4350_FB0_INT_CAP;
	}
}
// write a configuration to the ADA4350 amplifier via SPI
// first two parameters specify the slave-select (CS) GPIO pin for the amplifier
// (since we have 3 amplifiers connected to the SPI bus)
// 3rd parameter is the register configuration to be written
void ADA4350_set_config(GPIO_TypeDef* CS_gpio_port, uint16_t CS_pin, uint32_t data){
	// CS low to select slave
	HAL_GPIO_WritePin(CS_gpio_port, CS_pin, GPIO_PIN_RESET);
	uint8_t data_byte[3];
	data_byte[0] = (uint8_t) ( (data>>16) & 0xFF );
	data_byte[1] = (uint8_t) ( (data>>8) & 0xFF );
	data_byte[2] = (uint8_t) (data & 0xFF);
	HAL_SPI_Transmit(&hspi1, data_byte, 3, 100);
	// CS high to deselect slave
	HAL_GPIO_WritePin(CS_gpio_port, CS_pin, GPIO_PIN_SET);
}
// read back the configuration of the ADA4350 amplifier via SPI
// first two parameters specify the slave-select (CS) GPIO pin for the amplifier
// (since we have 3 amplifiers connected to the SPI bus)
// return value is the register configuration
uint32_t ADA4350_read_config(GPIO_TypeDef* CS_gpio_port, uint16_t CS_pin){
	// set 23rd bit to 1 to read register content
	uint8_t data_byte[3] = {(1<<7), 0x00, 0x00};
	uint8_t result_buf[3];
	// CS low to select slave
	HAL_GPIO_WritePin(CS_gpio_port, CS_pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(&hspi1, data_byte, result_buf, 3, HAL_TIMEOUT);
	// CS high to deselect slave
	HAL_GPIO_WritePin(CS_gpio_port, CS_pin, GPIO_PIN_SET);
	uint32_t register_content = (result_buf[0]<<16) | (result_buf[1]<<8) | result_buf[2];
	return register_content;
}

// process AD-conversion and data transmission
void adc_dma_usb_tx(){
		// check if USB is ready to transmit a new packet
		if( CDC_get_tx_flag() == CDC_TX_READY ){
			/* check each DMA transfer for completion
			 * if a half transfer is complete, send first half of buffer
			 * if a (full) transfer is complete, send second half of buffer
			 * NOTE: because CDC_Transmit() sends an array of bytes (uint8_t) and our samples are 2bytes (uint16_t) each
			 * we need to cast the pointer and choose the length correctly, i.e. we have BUF_LEN/2 uint16_t samples
			 * which equals to BUF_LEN uint8_t bytes
			 */
			if( sdadc1_half_complete ){
				// clear flag
				sdadc1_half_complete = false;
				// transmit packet via USB
				CDC_Transmit_FS((uint8_t*)sdadc1_data_buf, BUF_LEN);
			}
			else if( sdadc2_half_complete ){
				sdadc2_half_complete = false;
				CDC_Transmit_FS((uint8_t*)sdadc2_data_buf, BUF_LEN);
			}
			else if( sdadc3_half_complete ){
				sdadc3_half_complete = false;
				CDC_Transmit_FS((uint8_t*)sdadc3_data_buf, BUF_LEN);
			}
			else if( sdadc1_full_complete ){
				sdadc1_full_complete = false;
				CDC_Transmit_FS( (uint8_t*)(&sdadc1_data_buf[BUF_LEN/2]), BUF_LEN);
			}
			else if( sdadc2_full_complete ){
				sdadc2_full_complete = false;
				CDC_Transmit_FS( (uint8_t*)(&sdadc2_data_buf[BUF_LEN/2]), BUF_LEN);
			}
			else if( sdadc3_full_complete ){
				sdadc3_full_complete = false;
				CDC_Transmit_FS( (uint8_t*)(&sdadc3_data_buf[BUF_LEN/2]), BUF_LEN);
			}
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
  MX_DMA_Init();
  MX_USB_DEVICE_Init();
  MX_SDADC1_Init();
  MX_SDADC2_Init();
  MX_SDADC3_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  // disable external interrupt (trigger input from SLM)
  HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
  // turn LED on
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
  // do offset calibration procedure for all 3 SDADCs
  HAL_SDADC_CalibrationStart(&hsdadc1, SDADC_CALIBRATION_SEQ_1);
  HAL_SDADC_PollForCalibEvent(&hsdadc1, 10);
  HAL_SDADC_CalibrationStart(&hsdadc2, SDADC_CALIBRATION_SEQ_1);
  HAL_SDADC_PollForCalibEvent(&hsdadc2, 10);
  HAL_SDADC_CalibrationStart(&hsdadc3, SDADC_CALIBRATION_SEQ_1);
  HAL_SDADC_PollForCalibEvent(&hsdadc3, 10);
  // turn LED off
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
  // re-enable external interrupt
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // check if there's something in the USB RX Reception buffer
	  if( FIFO_8_get_num_elements(&usb_cdc_rx_fifo) > 0 ){
		  /* 'G' character to set gain path (gain factor) of the 3 amplifiers
		   * command format is 'G' + gain path1 + gain path2 + gain path3 +'\n'
		   * e.g. 'G123\n' to set gain path 1 for amplifier1, gain path 2 for amplifier2 etc.
		   * gain path number can be from 0 to 5
		   */
		  // read one received character from the RX fifo buffer
		  uint8_t c = 0;
		  FIFO_8_get(&usb_cdc_rx_fifo, &c);
		  if( c == 'G'){
			  // read another 3 characters and convert the characters to integers
			  uint8_t gain_path[3] = {0, 0, 0};
			  for(uint8_t i=0; i<3; ++i){
				  FIFO_8_get(&usb_cdc_rx_fifo, &gain_path[i]);
				  gain_path[i] -= '0'; // ASCII to integer conversion
			  }
			  // write the configuration to each of the 3 amplifiers via SPI interface
			  ADA4350_set_config(SPI1_CS1_GPIO_Port, SPI1_CS1_Pin, ADA4350_parse_config(gain_path[0]));
			  ADA4350_set_config(SPI1_CS2_GPIO_Port, SPI1_CS2_Pin, ADA4350_parse_config(gain_path[1]));
			  ADA4350_set_config(SPI1_CS3_GPIO_Port, SPI1_CS3_Pin, ADA4350_parse_config(gain_path[2]));
			  // wait until USB is ready
			  while( CDC_get_tx_flag() == CDC_TX_BUSY );
			  // send back "OK"+gain configuration+\n, e.g. "OK123\n"
			  uint8_t msg[6] = { 'O', 'K', gain_path[0]+'0', gain_path[1]+'0', gain_path[2]+'0', '\n'};
			  // transmit message over USB
			  CDC_Transmit_FS(msg, 6);
		  }
	  }

	  // check trigger flag
	  if( recording_trigger ){
		  // turn LED on
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
		  // start all 3 SDADCs + DMA
		  HAL_SDADC_Start_DMA(&hsdadc1, (uint32_t*)sdadc1_data_buf, BUF_LEN);
		  HAL_SDADC_Start_DMA(&hsdadc2, (uint32_t*)sdadc2_data_buf, BUF_LEN);
		  HAL_SDADC_Start_DMA(&hsdadc3, (uint32_t*)sdadc3_data_buf, BUF_LEN);
		  // record and stream data while trigger signal is high
		  while( recording_trigger ){
			  // stream data over USB
			  adc_dma_usb_tx();
		  }
		  // trigger has gone low now
		  // stop SDADCs and DMA
		  HAL_SDADC_Stop_DMA(&hsdadc1);
		  HAL_SDADC_Stop_DMA(&hsdadc2);
		  HAL_SDADC_Stop_DMA(&hsdadc3);
		  // reset all flags
		  sdadc1_half_complete = false;
		  sdadc1_full_complete = false;
		  sdadc2_half_complete = false;
		  sdadc2_full_complete = false;
		  sdadc3_half_complete = false;
		  sdadc3_full_complete = false;
		  // turn LED off
		  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
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
  /** Initializes the CPU, AHB and APB busses clocks 
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_SDADC;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  PeriphClkInit.SdadcClockSelection = RCC_SDADCSYSCLK_DIV12;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_PWREx_EnableSDADC(PWR_SDADC_ANALOG1);
  HAL_PWREx_EnableSDADC(PWR_SDADC_ANALOG2);
  HAL_PWREx_EnableSDADC(PWR_SDADC_ANALOG3);
}

/**
  * @brief SDADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDADC1_Init(void)
{

  /* USER CODE BEGIN SDADC1_Init 0 */

  /* USER CODE END SDADC1_Init 0 */

  SDADC_ConfParamTypeDef ConfParamStruct = {0};

  /* USER CODE BEGIN SDADC1_Init 1 */

  /* USER CODE END SDADC1_Init 1 */
  /** Configure the SDADC low power mode, fast conversion mode,
  slow clock mode and SDADC1 reference voltage 
  */
  hsdadc1.Instance = SDADC1;
  hsdadc1.Init.IdleLowPowerMode = SDADC_LOWPOWER_NONE;
  hsdadc1.Init.FastConversionMode = SDADC_FAST_CONV_ENABLE;
  hsdadc1.Init.SlowClockMode = SDADC_SLOW_CLOCK_DISABLE;
  hsdadc1.Init.ReferenceVoltage = SDADC_VREF_VDDA;
  if (HAL_SDADC_Init(&hsdadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure The Regular Mode 
  */
  if (HAL_SDADC_SelectRegularTrigger(&hsdadc1, SDADC_SOFTWARE_TRIGGER) != HAL_OK)
  {
    Error_Handler();
  }
  /** Set parameters for SDADC configuration 0 Register 
  */
  ConfParamStruct.InputMode = SDADC_INPUT_MODE_SE_ZERO_REFERENCE;
  ConfParamStruct.Gain = SDADC_GAIN_1;
  ConfParamStruct.CommonMode = SDADC_COMMON_MODE_VSSA;
  ConfParamStruct.Offset = 0;
  if (HAL_SDADC_PrepareChannelConfig(&hsdadc1, SDADC_CONF_INDEX_0, &ConfParamStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the Regular Channel 
  */
  if (HAL_SDADC_AssociateChannelConfig(&hsdadc1, SDADC_CHANNEL_4, SDADC_CONF_INDEX_0) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_SDADC_ConfigChannel(&hsdadc1, SDADC_CHANNEL_4, SDADC_CONTINUOUS_CONV_ON) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDADC1_Init 2 */

  /* USER CODE END SDADC1_Init 2 */

}

/**
  * @brief SDADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDADC2_Init(void)
{

  /* USER CODE BEGIN SDADC2_Init 0 */

  /* USER CODE END SDADC2_Init 0 */

  SDADC_ConfParamTypeDef ConfParamStruct = {0};

  /* USER CODE BEGIN SDADC2_Init 1 */

  /* USER CODE END SDADC2_Init 1 */
  /** Configure the SDADC low power mode, fast conversion mode,
  slow clock mode and SDADC1 reference voltage 
  */
  hsdadc2.Instance = SDADC2;
  hsdadc2.Init.IdleLowPowerMode = SDADC_LOWPOWER_NONE;
  hsdadc2.Init.FastConversionMode = SDADC_FAST_CONV_ENABLE;
  hsdadc2.Init.SlowClockMode = SDADC_SLOW_CLOCK_DISABLE;
  hsdadc2.Init.ReferenceVoltage = SDADC_VREF_VDDA;
  if (HAL_SDADC_Init(&hsdadc2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure The Regular Mode 
  */
  if (HAL_SDADC_SelectRegularTrigger(&hsdadc2, SDADC_SOFTWARE_TRIGGER) != HAL_OK)
  {
    Error_Handler();
  }
  /** Set parameters for SDADC configuration 0 Register 
  */
  ConfParamStruct.InputMode = SDADC_INPUT_MODE_SE_ZERO_REFERENCE;
  ConfParamStruct.Gain = SDADC_GAIN_1;
  ConfParamStruct.CommonMode = SDADC_COMMON_MODE_VSSA;
  ConfParamStruct.Offset = 0;
  if (HAL_SDADC_PrepareChannelConfig(&hsdadc2, SDADC_CONF_INDEX_0, &ConfParamStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the Regular Channel 
  */
  if (HAL_SDADC_AssociateChannelConfig(&hsdadc2, SDADC_CHANNEL_7, SDADC_CONF_INDEX_0) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_SDADC_ConfigChannel(&hsdadc2, SDADC_CHANNEL_7, SDADC_CONTINUOUS_CONV_ON) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDADC2_Init 2 */

  /* USER CODE END SDADC2_Init 2 */

}

/**
  * @brief SDADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDADC3_Init(void)
{

  /* USER CODE BEGIN SDADC3_Init 0 */

  /* USER CODE END SDADC3_Init 0 */

  SDADC_ConfParamTypeDef ConfParamStruct = {0};

  /* USER CODE BEGIN SDADC3_Init 1 */

  /* USER CODE END SDADC3_Init 1 */
  /** Configure the SDADC low power mode, fast conversion mode,
  slow clock mode and SDADC1 reference voltage 
  */
  hsdadc3.Instance = SDADC3;
  hsdadc3.Init.IdleLowPowerMode = SDADC_LOWPOWER_NONE;
  hsdadc3.Init.FastConversionMode = SDADC_FAST_CONV_ENABLE;
  hsdadc3.Init.SlowClockMode = SDADC_SLOW_CLOCK_DISABLE;
  hsdadc3.Init.ReferenceVoltage = SDADC_VREF_VDDA;
  if (HAL_SDADC_Init(&hsdadc3) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure The Regular Mode 
  */
  if (HAL_SDADC_SelectRegularTrigger(&hsdadc3, SDADC_SOFTWARE_TRIGGER) != HAL_OK)
  {
    Error_Handler();
  }
  /** Set parameters for SDADC configuration 0 Register 
  */
  ConfParamStruct.InputMode = SDADC_INPUT_MODE_SE_ZERO_REFERENCE;
  ConfParamStruct.Gain = SDADC_GAIN_1;
  ConfParamStruct.CommonMode = SDADC_COMMON_MODE_VSSA;
  ConfParamStruct.Offset = 0;
  if (HAL_SDADC_PrepareChannelConfig(&hsdadc3, SDADC_CONF_INDEX_0, &ConfParamStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the Regular Channel 
  */
  if (HAL_SDADC_AssociateChannelConfig(&hsdadc3, SDADC_CHANNEL_8, SDADC_CONF_INDEX_0) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_SDADC_ConfigChannel(&hsdadc3, SDADC_CHANNEL_8, SDADC_CONTINUOUS_CONV_ON) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDADC3_Init 2 */

  /* USER CODE END SDADC3_Init 2 */

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
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel3_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel3_IRQn);
  /* DMA2_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel4_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel4_IRQn);
  /* DMA2_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel5_IRQn);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SPI1_CS2_Pin|SPI1_CS3_Pin|SPI1_CS1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin SPI1_CS2_Pin SPI1_CS3_Pin SPI1_CS1_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|SPI1_CS2_Pin|SPI1_CS3_Pin|SPI1_CS1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
// redirect stdout to SWO Trace interface for debugging messages
int _write(int file, char* ptr, int len) {
	int DataIdx;
	for(DataIdx=0; DataIdx<len; DataIdx++){
		ITM_SendChar(*ptr++);
	}
	return len;
}
// SDADC DMA Half Transfer Complete ISR
void HAL_SDADC_ConvHalfCpltCallback(SDADC_HandleTypeDef* hsdadc){
	// check which ADC has triggered the interrupt and set the corresponding flag
	if( hsdadc == &hsdadc1 ){
		sdadc1_half_complete = true;
	}else if( hsdadc == &hsdadc2 ){
		sdadc2_half_complete = true;
	}else if( hsdadc == &hsdadc3 ){
		sdadc3_half_complete = true;
	}
}

// SDADC DMA (Full) Transfer Complete ISR
void HAL_SDADC_ConvCpltCallback(SDADC_HandleTypeDef* hsdadc){
	// check which ADC has triggered the interrupt and set the corresponding flag
	if( hsdadc->Instance == SDADC1 ){
		sdadc1_full_complete = true;
	}else if( hsdadc->Instance == SDADC2 ){
		sdadc2_full_complete = true;
	}else if( hsdadc->Instance == SDADC3 ){
		sdadc3_full_complete = true;
	}
}
// external pin interrupt (trigger signal)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	// check if indeed pin PC13 has triggered the interrupt
	if( GPIO_Pin == GPIO_PIN_13 ){
		// if pin level is high, it has to be a rising edge
		if( HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET ){
			// set flag
			recording_trigger = true;
		}
		// else it´s a falling edge
		else if( HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET ){
			// set flag
			recording_trigger = false;
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
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
