/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lora_app.c
  * @author  MCD Application Team
  * @brief   Application of the LRWAN Middleware
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
#include "platform.h"
#include "sys_app.h"
#include "lora_app.h"
#include "stm32_seq.h"
#include "stm32_timer.h"
#include "utilities_def.h"
#include "app_version.h"
#include "lorawan_version.h"
#include "subghz_phy_version.h"
#include "lora_info.h"
#include "LmHandler.h"
#include "adc_if.h"
#include "CayenneLpp.h"
#include "sys_sensors.h"
#include "flash_if.h"

/* USER CODE BEGIN Includes */
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "usart.h"
#include "stm32_seq.h"    // for UTIL_SEQ_SetTask / Sleep routines
#include "stm32_lpm.h"    // for UTIL_LPM_EnterLowPower()
#include "stm32_lpm_if.h"  // or whichever header contains the extern

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */
extern SUBGHZ_HandleTypeDef hsubghz;
//#define RELAY_BUF_SIZE 64
#define BUFFSIZE 64
#define ASCII_TX_MAX 256


extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern uint8_t rx_buff[1];

uint8_t readings = 10;
uint16_t buf_co2[100];
uint16_t avg_co2 = 0;
uint8_t i = 0;
//uint8_t buf_uart[12];
//uint8_t TxBufferMode[5]="K 2\r\n";
//uint8_t TxBufferModeone[5]="K 1\r\n";
//uint8_t TxBufferModezero[5]="K 0\r\n";
uint8_t TxBufferFilteredReading[3]="Z\r\n";
//uint8_t RxBuffermode[9] = {0};
uint8_t RdBuffer[BUFFSIZE];    // Buffer circolare per dati dal sensore
uint8_t RdPCBuffer[BUFFSIZE];  // Buffer lineare per comandi dal PC
uint16_t InS = 0, PCPtr = 0;
uint8_t data[] = "Received\r\n";
//uint8_t dataerr[] = "Error\r\n";
uint8_t mode;
char out[80];
char outm[80];
uint16_t readf;
uint16_t readnf;
int len;
uint8_t lora = 1;
uint8_t count;
uint8_t buffer_index;
uint8_t selection = 2;
uint8_t period;
static uint8_t asciiTxBuf[ASCII_TX_MAX];
static volatile uint16_t asciiTxLen = 0;
volatile uint8_t tx_in_progress = 0;
uint16_t readingInterval = 500;

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/**
  * @brief LoRa State Machine states
  */
typedef enum TxEventType_e
{
  /**
    * @brief Appdata Transmission issue based on timer every TxDutyCycleTime
    */
  TX_ON_TIMER,
  /**
    * @brief Appdata Transmission external event plugged on OnSendEvent( )
    */
  TX_ON_EVENT
  /* USER CODE BEGIN TxEventType_t */

  /* USER CODE END TxEventType_t */
} TxEventType_t;

/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/**
  * LEDs period value of the timer in ms
  */
#define LED_PERIOD_TIME 500

/**
  * Join switch period value of the timer in ms
  */
#define JOIN_TIME 2000

/*---------------------------------------------------------------------------*/
/*                             LoRaWAN NVM configuration                     */
/*---------------------------------------------------------------------------*/
/**
  * @brief LoRaWAN NVM Flash address
  * @note last 2 sector of a 128kBytes device
  */
#define LORAWAN_NVM_BASE_ADDRESS                    ((void *)0x0803F000UL)

/* USER CODE BEGIN PD */
static const char *slotStrings[] = { "1", "2", "C", "C_MC", "P", "P_MC" };
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  LoRa End Node send request
  */
static void SendTxData(void);

/**
  * @brief  TX timer callback function
  * @param  context ptr of timer context
  */
static void OnTxTimerEvent(void *context);

/**
  * @brief  join event callback function
  * @param  joinParams status of join
  */
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams);

/**
  * @brief callback when LoRaWAN application has sent a frame
  * @brief  tx event callback function
  * @param  params status of last Tx
  */
static void OnTxData(LmHandlerTxParams_t *params);

/**
  * @brief callback when LoRaWAN application has received a frame
  * @param appData data received in the last Rx
  * @param params status of last Rx
  */
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);

/**
  * @brief callback when LoRaWAN Beacon status is updated
  * @param params status of Last Beacon
  */
static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params);

/**
  * @brief callback when system time has been updated
  */
static void OnSysTimeUpdate(void);

/**
  * @brief callback when LoRaWAN application Class is changed
  * @param deviceClass new class
  */
static void OnClassChange(DeviceClass_t deviceClass);

/**
  * @brief  LoRa store context in Non Volatile Memory
  */
static void StoreContext(void);

/**
  * @brief  stop current LoRa execution to switch into non default Activation mode
  */
static void StopJoin(void);

/**
  * @brief  Join switch timer callback function
  * @param  context ptr of Join switch context
  */
static void OnStopJoinTimerEvent(void *context);

/**
  * @brief  Notifies the upper layer that the NVM context has changed
  * @param  state Indicates if we are storing (true) or restoring (false) the NVM context
  */
static void OnNvmDataChange(LmHandlerNvmContextStates_t state);

/**
  * @brief  Store the NVM Data context to the Flash
  * @param  nvm ptr on nvm structure
  * @param  nvm_size number of data bytes which were stored
  */
static void OnStoreContextRequest(void *nvm, uint32_t nvm_size);

/**
  * @brief  Restore the NVM Data context from the Flash
  * @param  nvm ptr on nvm structure
  * @param  nvm_size number of data bytes which were restored
  */
static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size);

/**
  * Will be called each time a Radio IRQ is handled by the MAC layer
  *
  */
static void OnMacProcessNotify(void);

/**
  * @brief Change the periodicity of the uplink frames
  * @param periodicity uplink frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnTxPeriodicityChanged(uint32_t periodicity);

/**
  * @brief Change the confirmation control of the uplink frames
  * @param isTxConfirmed Indicates if the uplink requires an acknowledgement
  * @note Compliance test protocol callbacks
  */
static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed);

/**
  * @brief Change the periodicity of the ping slot frames
  * @param pingSlotPeriodicity ping slot frames period in ms
  * @note Compliance test protocol callbacks
  */
static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity);

/**
  * @brief Will be called to reset the system
  * @note Compliance test protocol callbacks
  */
static void OnSystemReset(void);

/* USER CODE BEGIN PFP */

static void readCO2(void);
uint16_t average_u16_int(const uint16_t *arr, size_t len);
static void commUsart1(void);
static void commUsart2(void);
static void printOnUart(void);
static void OnReadTimerEvent(void *context);

/* USER CODE END PFP */

/* Private variables ---------------------------------------------------------*/
/**
  * @brief LoRaWAN default activation type
  */
static ActivationType_t ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;

/**
  * @brief LoRaWAN force rejoin even if the NVM context is restored
  */
static bool ForceRejoin = LORAWAN_FORCE_REJOIN_AT_BOOT;

/**
  * @brief LoRaWAN handler Callbacks
  */
static LmHandlerCallbacks_t LmHandlerCallbacks =
{
  .GetBatteryLevel =              GetBatteryLevel,
  .GetTemperature =               GetTemperatureLevel,
  .GetUniqueId =                  GetUniqueId,
  .GetDevAddr =                   GetDevAddr,
  .OnRestoreContextRequest =      OnRestoreContextRequest,
  .OnStoreContextRequest =        OnStoreContextRequest,
  .OnMacProcess =                 OnMacProcessNotify,
  .OnNvmDataChange =              OnNvmDataChange,
  .OnJoinRequest =                OnJoinRequest,
  .OnTxData =                     OnTxData,
  .OnRxData =                     OnRxData,
  .OnBeaconStatusChange =         OnBeaconStatusChange,
  .OnSysTimeUpdate =              OnSysTimeUpdate,
  .OnClassChange =                OnClassChange,
  .OnTxPeriodicityChanged =       OnTxPeriodicityChanged,
  .OnTxFrameCtrlChanged =         OnTxFrameCtrlChanged,
  .OnPingSlotPeriodicityChanged = OnPingSlotPeriodicityChanged,
  .OnSystemReset =                OnSystemReset,
};

/**
  * @brief LoRaWAN handler parameters
  */
static LmHandlerParams_t LmHandlerParams =
{
  .ActiveRegion =             ACTIVE_REGION,
  .DefaultClass =             LORAWAN_DEFAULT_CLASS,
  .AdrEnable =                LORAWAN_ADR_STATE,
  .IsTxConfirmed =            LORAWAN_DEFAULT_CONFIRMED_MSG_STATE,
  .TxDatarate =               LORAWAN_DEFAULT_DATA_RATE,
  .TxPower =                  LORAWAN_DEFAULT_TX_POWER,
  .PingSlotPeriodicity =      LORAWAN_DEFAULT_PING_SLOT_PERIODICITY,
  .RxBCTimeout =              LORAWAN_DEFAULT_CLASS_B_C_RESP_TIMEOUT
};

/**
  * @brief Type of Event to generate application Tx
  */
static TxEventType_t EventType = TX_ON_TIMER;

/**
  * @brief Timer to handle the application Tx
  */
static UTIL_TIMER_Object_t TxTimer;

/**
  * @brief Tx Timer period
  */
static UTIL_TIMER_Time_t TxPeriodicity = APP_TX_DUTYCYCLE;

/**
  * @brief Join Timer period
  */
static UTIL_TIMER_Object_t StopJoinTimer;

/* USER CODE BEGIN PV */

static UTIL_TIMER_Object_t ReadTimer;

/**
  * @brief User application buffer
  */
static uint8_t AppDataBuffer[LORAWAN_APP_DATA_BUFFER_MAX_SIZE];

/**
  * @brief User application data structure
  */
static LmHandlerAppData_t AppData = { 0, 0, AppDataBuffer };

/**
  * @brief Specifies the state of the application LED
  */
static uint8_t AppLedStateOn = RESET;

/* USER CODE END PV */

/* Exported functions ---------------------------------------------------------*/
/* USER CODE BEGIN EF */

/* USER CODE END EF */

void LoRaWAN_Init(void)
{
  /* USER CODE BEGIN LoRaWAN_Init_LV */
  uint32_t feature_version = 0UL;
  /* USER CODE END LoRaWAN_Init_LV */

  /* USER CODE BEGIN LoRaWAN_Init_1 */

  /* Get LoRaWAN APP version*/
  APP_LOG(TS_OFF, VLEVEL_M, "APPLICATION_VERSION: V%X.%X.%X\r\n",
          (uint8_t)(APP_VERSION_MAIN),
          (uint8_t)(APP_VERSION_SUB1),
          (uint8_t)(APP_VERSION_SUB2));

  /* Get MW LoRaWAN info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_LORAWAN_VERSION:  V%X.%X.%X\r\n",
          (uint8_t)(LORAWAN_VERSION_MAIN),
          (uint8_t)(LORAWAN_VERSION_SUB1),
          (uint8_t)(LORAWAN_VERSION_SUB2));

  /* Get MW SubGhz_Phy info */
  APP_LOG(TS_OFF, VLEVEL_M, "MW_RADIO_VERSION:    V%X.%X.%X\r\n",
          (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
          (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

  /* Get LoRaWAN Link Layer info */
  LmHandlerGetVersion(LORAMAC_HANDLER_L2_VERSION, &feature_version);
  APP_LOG(TS_OFF, VLEVEL_M, "L2_SPEC_VERSION:     V%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8));

  /* Get LoRaWAN Regional Parameters info */
  LmHandlerGetVersion(LORAMAC_HANDLER_REGION_VERSION, &feature_version);
  APP_LOG(TS_OFF, VLEVEL_M, "RP_SPEC_VERSION:     V%X-%X.%X.%X\r\n",
          (uint8_t)(feature_version >> 24),
          (uint8_t)(feature_version >> 16),
          (uint8_t)(feature_version >> 8),
          (uint8_t)(feature_version));

  if (FLASH_IF_Init(NULL) != FLASH_IF_OK)
  {
    Error_Handler();
  }

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_ReadFilteredCO2), UTIL_SEQ_RFU, readCO2);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_USART1), UTIL_SEQ_RFU, commUsart1);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_USART2), UTIL_SEQ_RFU, commUsart2);


  mode = 2;

  /* USER CODE END LoRaWAN_Init_1 */

  UTIL_TIMER_Create(&StopJoinTimer, JOIN_TIME, UTIL_TIMER_ONESHOT, OnStopJoinTimerEvent, NULL);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LmHandlerProcess), UTIL_SEQ_RFU, LmHandlerProcess);

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), UTIL_SEQ_RFU, SendTxData);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaStoreContextEvent), UTIL_SEQ_RFU, StoreContext);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LoRaStopJoinEvent), UTIL_SEQ_RFU, StopJoin);

  /* Init Info table used by LmHandler*/
  LoraInfo_Init();

  /* Init the Lora Stack*/
  LmHandlerInit(&LmHandlerCallbacks, APP_VERSION);

  LmHandlerConfigure(&LmHandlerParams);

  /* USER CODE BEGIN LoRaWAN_Init_2 */
  //UTIL_TIMER_Start(&JoinLedTimer);

  /* USER CODE END LoRaWAN_Init_2 */

  LmHandlerJoin(ActivationType, ForceRejoin);

  if (EventType == TX_ON_TIMER)
  {
    /* send every time timer elapses */
    UTIL_TIMER_Create(&TxTimer, TxPeriodicity, UTIL_TIMER_ONESHOT, OnTxTimerEvent, NULL);
    UTIL_TIMER_Start(&TxTimer);
  }
  else
  {
    /* USER CODE BEGIN LoRaWAN_Init_3 */

    /* USER CODE END LoRaWAN_Init_3 */
  }

  /* USER CODE BEGIN LoRaWAN_Init_Last */
  UTIL_TIMER_Create(&ReadTimer, readingInterval, UTIL_TIMER_ONESHOT, OnReadTimerEvent, NULL);
  /* USER CODE END LoRaWAN_Init_Last */
}

/* USER CODE BEGIN PB_Callbacks */

/*void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        tx_in_progress = 0;
    }
}
*/

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	if (huart->Instance == USART1){
		UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_USART1), CFG_SEQ_Prio_0);
    }



	if (huart->Instance == USART2){
		 UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_USART2), CFG_SEQ_Prio_0);
	 }

}


uint16_t average_u16_int(const uint16_t *arr, size_t len) {
    if (len == 0) {
        return 0;
    }
    // Use a wider accumulator to avoid overflow.
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += arr[i];
    }
    // Integer division will truncate toward zero.
    return (uint16_t)(sum / len);
}

void readCO2(void)
{
    // 3) Richiedo il dato “Z\r\n”
    HAL_UART_Transmit(&huart1, TxBufferFilteredReading, 3, 0xFFFF);
    //HAL_Delay(30);
}

void printOnUart(void){
	// 1) Reset tx buffer length (important!)
	int pos = 0;
	asciiTxLen = 0;

	// 2) Header
	pos += snprintf((char*)asciiTxBuf + pos, ASCII_TX_MAX - pos, "AppDataBuffer (%u bytes): ", (unsigned)buffer_index);

	// 3) Convert each LSB/MSB pair to a decimal number separated by commas
	for (int bi_local = 0; bi_local + 1 < buffer_index && pos < (ASCII_TX_MAX - 16); bi_local += 2)
	{
		uint16_t val = (uint16_t)AppDataBuffer[bi_local] | ((uint16_t)AppDataBuffer[bi_local+1] << 8);
		pos += snprintf((char*)asciiTxBuf + pos, ASCII_TX_MAX - pos, "%u", (unsigned)val);
		if (bi_local + 2 < buffer_index)
		{
			pos += snprintf((char*)asciiTxBuf + pos, ASCII_TX_MAX - pos, ",");
		}
	}

	// 4) Terminatore di riga
	pos += snprintf((char*)asciiTxBuf + pos, ASCII_TX_MAX - pos, "\r\n");
	if (pos < ASCII_TX_MAX) asciiTxBuf[pos] = '\0';
	asciiTxLen = (uint16_t)pos;

	// 5) Avvia trasmissione non-bloccante solo se non ce n'è già una in corso
	if (!tx_in_progress)
	{
		tx_in_progress = 1;
		if (HAL_UART_Transmit(&huart2, asciiTxBuf, asciiTxLen,100) != HAL_OK)
		{
			// errore nel lancio: libera la flag così possiamo provare dopo
			tx_in_progress = 0;
		}
	}
}

void commUsart1(void)
{
	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////
	// Modalità Manuale
	// Attesa dei comandi da parte dell'utente
	// Di default modalità 2 (no continua stampa su uart)
	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////


	if (selection == 1){

		HAL_UART_Receive_IT(&huart1, rx_buff, 1);
		RdBuffer[InS] = rx_buff[0];
		if (RdBuffer[InS++] == '\n' || InS == BUFFSIZE){
			if (RdBuffer[1] == 'Z'){
				if (mode == 1){
					readf = atoi((char*)RdBuffer + 3);
					readnf = atoi((char*)RdBuffer + 11);
					//float t = HAL_GetTick() / 1000.0f;
					len = sprintf(out, ",%d,%d\r\n", readf, readnf);
					//len = sprintf(out, "Filtered CO2 concentration: %d ppm\r\nNon filtered CO2 concentration: %d ppm\r\n\n", readf, readnf);
					HAL_UART_Transmit(&huart2, (uint8_t*)out, len, 100);
					InS = 0;
				}
				else{
					readf = atoi((char*)RdBuffer + 3);
					len = sprintf(out, "Filtered CO2 concentration: %d ppm\r\n", readf);
					HAL_UART_Transmit(&huart2, (uint8_t*)out, len, 100);
					InS = 0;
				}
			}
			else if (RdBuffer[1] == 'z'){
				readnf = atoi((char*)RdBuffer + 3);
				len = sprintf(out, "Raw CO2 concentration: %d ppm\r\n", readnf);
				HAL_UART_Transmit(&huart2, (uint8_t*)out, len, 100);
				InS = 0;
			}
			else if (RdBuffer[1] == 'K'){
				readf = atoi((char*)RdBuffer + 3);
				mode = readf;
				len = sprintf(out, "Inserted mode %d \r\n", readf);
				HAL_UART_Transmit(&huart2, (uint8_t*)out, len, 100);
				InS = 0;
			}
			else{
				HAL_UART_Transmit(&huart2, RdBuffer, InS, 100);
				InS = 0;
			}
		}
	}


	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////
	// Modalità Automatica
	// Raccolta di 10 misure, media, accumulo della media su un buffer
	// Trasmissione del buffer delle medie quando è pieno
	///////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////



	else if (selection == 2){
		// 1) Re‐armo subito per la prossima singola lettura
		HAL_UART_Receive_IT(&huart1, rx_buff, 1);

		//int lenm = snprintf(outm, sizeof(outm), "%u\r\n", (unsigned)rx_buff[0]);
		//HAL_UART_Transmit(&huart2, (uint8_t*)outm, lenm, 100);

		// 2) Metto il byte ricevuto in RdBuffer[InS]
		RdBuffer[InS] = rx_buff[0];

		// 3) Se ho chiuso la riga ("\n") o ho saturato il buffer:
		if (RdBuffer[InS++] == '\n' || InS == BUFFSIZE)
		{
			InS = 0;
			//  3a) Se è una riga di risposta CO2 (secondo carattere = 'Z')
			if (RdBuffer[1] == 'Z' && mode == 2)
			{
		        //HAL_UART_Transmit(&huart2, (uint8_t*)("Extracting new value\r\n"), 22, 0xFFFF);

				// Estraggo il valore numerico
				uint16_t newValue = (uint16_t)atoi((char*)RdBuffer + 3);
				//len = sprintf(out, "Filtered CO2 concentration: %d ppm\r\n", newValue);
				//HAL_UART_Transmit(&huart2, (uint8_t*)out, len, 100);
				//InS = 0; // azzero indice RdBuffer subito

				// Accumulo su buf_co2 e conto fino a 10 letture
				buf_co2[i++] = newValue;
				if (i == readings)
				{
					//UTIL_TIMER_Stop(&ReadTimer);


					// Calcolo media e la scrivo nel buffer LoRa
					avg_co2 = average_u16_int(buf_co2, readings);
					len = sprintf(out, "%d samples average: %d ppm\r\n", readings, avg_co2);
					HAL_UART_Transmit(&huart2, (uint8_t*)out, len, 100);
					i = 0;

					// Metto i due byte di avg_co2 in AppDataBuffer
					AppDataBuffer[buffer_index++] = (uint8_t)(avg_co2 & 0xFF);
					AppDataBuffer[buffer_index++] = (uint8_t)((avg_co2 >> 8) & 0xFF);
					//AppData.BufferSize = buffer_index;

			        HAL_UART_Transmit(&huart1, (uint8_t*)("K 0\r\n"), 5, 0xFFFF);
					count = 0;

					// Se ho raccolto x medie invio tramite lora (se lora == 1) o tramite UART (se lora == 0)
					if (buffer_index >= 10)
					{
						if(lora){
						    AppData.BufferSize = buffer_index;
						    LmHandlerSetDutyCycleEnable(false);
						    //UTIL_TIMER_Stop(&TxTimer);
						    //UTIL_TIMER_Stop(&ReadTimer);
						    //HAL_UART_Transmit(&huart2, (uint8_t*)"LoRaWAN transmission incoming...\r\n", 35, 100);
							//SendTxData();
						    //UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);
						}

						else{
							printOnUart();

							UTIL_TIMER_Start(&TxTimer);
						}

						// 6) Resetta il buffer di raccolta così possiamo continuare a popolarlo
						buffer_index = 0;
					}

					return;
				}

				UTIL_TIMER_Start(&ReadTimer);
				return;
			}

			//  3b) Se è un comando di cambio modalità (“K …”)
			else if (RdBuffer[1] == 'K')
			{
				//HAL_UART_Transmit(&huart2, (uint8_t*)"K\r\n", 3, 100);

				uint16_t newMode = (uint16_t)atoi((char*)RdBuffer + 2);
				mode = (uint8_t)newMode;
				len = sprintf(out, "Inserted mode %hu\r\n", newMode);
				//HAL_UART_Transmit(&huart2, (uint8_t*)out, len, 0xFFFF);
				if(mode == 0){
					if (AppData.BufferSize >= 10){
						UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent), CFG_SEQ_Prio_0);
					}
					UTIL_LPM_SetStopMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_ENABLE);
				}
				else if (mode == 2){
					//HAL_UART_Transmit(&huart2, (uint8_t*)"2\r\n", 3, 100);
					UTIL_TIMER_Start(&ReadTimer);
				}

				//InS = 0;
				return;
			}

			//  3c) Se arriva qualsiasi altra cosa
			else
			{
				//if(InS>0) InS--;
				//UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_ReadFilteredCO2), CFG_SEQ_Prio_0);
				return;
			}
		}
    }
}

void commUsart2(void)
{
	 HAL_UART_Receive_IT(&huart2, rx_buff, 1);
	 RdPCBuffer[PCPtr] = rx_buff[0];
	 //HAL_UART_Transmit(&huart2, (uint8_t*)"rec2\r\n", 6, 100);

	 if (RdPCBuffer[PCPtr] == 'c'){
	   if (selection == 1){
		   selection = 2;
		   HAL_UART_Transmit(&huart2, (uint8_t*)"Inserted automatic mode\r\n", 24, 100);
		   UTIL_TIMER_Start(&TxTimer);
	   }
	   else{
		   selection = 1;
		   HAL_UART_Transmit(&huart2, (uint8_t*)"Inserted manual mode\r\n", 21, 100);
	   }

	   PCPtr = 0;
	   return;
	 }

	 if (RdPCBuffer[PCPtr] == 'l'){
	 	lora = !lora;
		if (lora){
			//LoRaWAN_Init();
			HAL_UART_Transmit(&huart2, (uint8_t*)"Inserted LoRaWAN transmission mode\r\n", 36, 100);
		}
	 	else HAL_UART_Transmit(&huart2, (uint8_t*)"Inserted UART transmission mode\r\n", 33, 100);
	 	PCPtr = 0;
	 	return;
	 }

	 if (RdPCBuffer[PCPtr++] == '\n' || PCPtr == BUFFSIZE){
	   if (RdPCBuffer[0] == 't'){
	     period = atoi((char*)RdPCBuffer + 1);
	     OnTxPeriodicityChanged(period*1000);
	     len = sprintf(out, "Periodicity set to %lu ms\r\n", TxPeriodicity);
	     HAL_UART_Transmit(&huart2, (uint8_t*)out, len, 100);
	   }

	   else if (RdPCBuffer[0] == 'n'){
	     readings = atoi((char*)RdPCBuffer + 1);
	     len = sprintf(out, "Now you will read %d data every cycle\r\n", readings);
	     HAL_UART_Transmit(&huart2, (uint8_t*)out, len, 100);
	   }

	   else HAL_UART_Transmit(&huart1, RdPCBuffer, PCPtr, 100);



	   PCPtr = 0;
	 }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

}

/* USER CODE END PB_Callbacks */

/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PrFD */
static void OnReadTimerEvent(void *context)
{
	  if(selection == 2){
		  //HAL_UART_Transmit(&huart2, (uint8_t *)"read\r\n", 6, 100);
		  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_ReadFilteredCO2), CFG_SEQ_Prio_0);
	  }

	  //UTIL_TIMER_Start(&ReadTimer);
}
/* USER CODE END PrFD */

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
  /* USER CODE BEGIN OnRxData_1 */
  uint8_t RxPort = 0;

  if (params != NULL)
  {

    if (params->IsMcpsIndication)
    {
      if (appData != NULL)
      {
        RxPort = appData->Port;
        if (appData->Buffer != NULL)
        {
          switch (appData->Port)
          {
            case LORAWAN_SWITCH_CLASS_PORT:
              /*this port switches the class*/
              if (appData->BufferSize == 1)
              {
                switch (appData->Buffer[0])
                {
                  case 0:
                  {
                    LmHandlerRequestClass(CLASS_A);
                    break;
                  }
                  case 1:
                  {
                    LmHandlerRequestClass(CLASS_B);
                    break;
                  }
                  case 2:
                  {
                    LmHandlerRequestClass(CLASS_C);
                    break;
                  }
                  default:
                    break;
                }
              }
              break;
            case LORAWAN_USER_APP_PORT:
              if (appData->BufferSize == 1)
              {
                AppLedStateOn = appData->Buffer[0] & 0x01;
                if (AppLedStateOn == RESET)
                {
                  APP_LOG(TS_OFF, VLEVEL_H, "LED OFF\r\n");
                }
                else
                {
                  APP_LOG(TS_OFF, VLEVEL_H, "LED ON\r\n");
                }
              }
              break;

            default:

              break;
          }
        }
      }
    }
    if (params->RxSlot < RX_SLOT_NONE)
    {
      APP_LOG(TS_OFF, VLEVEL_H, "###### D/L FRAME:%04d | PORT:%d | DR:%d | SLOT:%s | RSSI:%d | SNR:%d\r\n",
              params->DownlinkCounter, RxPort, params->Datarate, slotStrings[params->RxSlot],
              params->Rssi, params->Snr);
    }
  }
  /* USER CODE END OnRxData_1 */
}

static void SendTxData(void)
{
  /* USER CODE BEGIN SendTxData_1 */
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;
  UTIL_TIMER_Time_t nextTxIn = 0;

  if (LmHandlerIsBusy() == false)
  {
	//HAL_UART_Transmit(&huart2, (uint8_t*)"LmHandler is not busy\r\n", 23, 100);
    AppData.Port = LORAWAN_USER_APP_PORT;

    status = LmHandlerSend(&AppData, LmHandlerParams.IsTxConfirmed, false);
    if (LORAMAC_HANDLER_SUCCESS == status)
    {
      APP_LOG(TS_ON, VLEVEL_L, "SEND REQUEST\r\n");
    }
    else if (LORAMAC_HANDLER_BUSY_ERROR == status)
    {
      APP_LOG(TS_ON, VLEVEL_L, "busy error\r\n");
    }
    else if (LORAMAC_HANDLER_NO_NETWORK_JOINED == status)
    {
      APP_LOG(TS_ON, VLEVEL_L, "no network joined\r\n");
    }
    else if (LORAMAC_HANDLER_COMPLIANCE_RUNNING == status)
    {
      APP_LOG(TS_ON, VLEVEL_L, "compliance running\r\n");
    }
    else if (LORAMAC_HANDLER_CRYPTO_ERROR == status)
    {
      APP_LOG(TS_ON, VLEVEL_L, "crypto error\r\n");
    }
    else if (LORAMAC_HANDLER_DUTYCYCLE_RESTRICTED == status)
    {
    	APP_LOG(TS_ON, VLEVEL_L, "dutycycle restricted\r\n");
      //nextTxIn = LmHandlerGetDutyCycleWaitTime();
     // if (nextTxIn > 0)
     // {
     //   APP_LOG(TS_ON, VLEVEL_L, "Next Tx in  : ~%d second(s)\r\n", (nextTxIn / 1000));
     // }
    }
  }

  if (EventType == TX_ON_TIMER)
  {
    UTIL_TIMER_Stop(&TxTimer);
    UTIL_TIMER_SetPeriod(&TxTimer, MAX(nextTxIn, TxPeriodicity));
    UTIL_TIMER_Start(&TxTimer);
  }

  AppData.BufferSize = 0;

  /* USER CODE END SendTxData_1 */
}

static void OnTxTimerEvent(void *context)
{
  /* USER CODE BEGIN OnTxTimerEvent_1 */
	  //UTIL_SEQ_ENTER_CRITICAL_SECTION();
	  if(selection == 2){
		  UTIL_LPM_SetStopMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_DISABLE);
		  HAL_UART_Receive_IT(&huart1, rx_buff, 1);

		  InS = 0;

		  //HAL_UART_Transmit(&huart2, (uint8_t *)"tim\r\n", 5, 0xFFFF); // arriva senza problemi

		  //if (mode != 2)
		  //{
			  //HAL_UART_Transmit(&huart2, (uint8_t*)("Changing mode to 2\r\n"), 20, 0xFFFF);
			  HAL_UART_Transmit(&huart1, (uint8_t*)("K 2\r\n"), 5, 0xFFFF);
		  //}


		  //UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_ReadFilteredCO2), CFG_SEQ_Prio_0);
		  //UTIL_TIMER_Start(&ReadTimer);
	  }


  /* USER CODE END OnTxTimerEvent_1 */

  /*Wait for next tx slot*/
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxTimerEvent_2 */

  /* USER CODE END OnTxTimerEvent_2 */
}

/* USER CODE BEGIN PrFD_LedEvents */


/* USER CODE END PrFD_LedEvents */

static void OnTxData(LmHandlerTxParams_t *params)
{
  /* USER CODE BEGIN OnTxData_1 */
  if ((params != NULL))
  {
    /* Process Tx event only if its a mcps response to prevent some internal events (mlme) */
    if (params->IsMcpsConfirm != 0)
    {
      //UTIL_TIMER_Start(&TxLedTimer);

      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### ========== MCPS-Confirm =============\r\n");
      APP_LOG(TS_OFF, VLEVEL_M, "###### U/L FRAME:%04d | PORT:%d | DR:%d | PWR:%d", params->UplinkCounter,
              params->AppData.Port, params->Datarate, params->TxPower);

      APP_LOG(TS_OFF, VLEVEL_M, " | MSG TYPE:");
      if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG)
      {
        APP_LOG(TS_OFF, VLEVEL_M, "CONFIRMED [%s]\r\n", (params->AckReceived != 0) ? "ACK" : "NACK");
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_M, "UNCONFIRMED\r\n");
      }
    }
  }
  /* USER CODE END OnTxData_1 */
}

static void OnJoinRequest(LmHandlerJoinParams_t *joinParams)
{
  /* USER CODE BEGIN OnJoinRequest_1 */
  if (joinParams != NULL)
  {
    if (joinParams->Status == LORAMAC_HANDLER_SUCCESS)
    {
      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOINED = ");
      if (joinParams->Mode == ACTIVATION_TYPE_ABP)
      {
        APP_LOG(TS_OFF, VLEVEL_M, "ABP ======================\r\n");
      }
      else
      {
        APP_LOG(TS_OFF, VLEVEL_M, "OTAA =====================\r\n");
      }
    }
    else
    {
      APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### = JOIN FAILED\r\n");
    }

    APP_LOG(TS_OFF, VLEVEL_H, "###### U/L FRAME:JOIN | DR:%d | PWR:%d\r\n", joinParams->Datarate, joinParams->TxPower);
  }
  /* USER CODE END OnJoinRequest_1 */
}

static void OnBeaconStatusChange(LmHandlerBeaconParams_t *params)
{
  /* USER CODE BEGIN OnBeaconStatusChange_1 */
  if (params != NULL)
  {
    switch (params->State)
    {
      default:
      case LORAMAC_HANDLER_BEACON_LOST:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON LOST\r\n");
        break;
      }
      case LORAMAC_HANDLER_BEACON_RX:
      {
        APP_LOG(TS_OFF, VLEVEL_M,
                "\r\n###### BEACON RECEIVED | DR:%d | RSSI:%d | SNR:%d | FQ:%d | TIME:%d | DESC:%d | "
                "INFO:02X%02X%02X %02X%02X%02X\r\n",
                params->Info.Datarate, params->Info.Rssi, params->Info.Snr, params->Info.Frequency,
                params->Info.Time.Seconds, params->Info.GwSpecific.InfoDesc,
                params->Info.GwSpecific.Info[0], params->Info.GwSpecific.Info[1],
                params->Info.GwSpecific.Info[2], params->Info.GwSpecific.Info[3],
                params->Info.GwSpecific.Info[4], params->Info.GwSpecific.Info[5]);
        break;
      }
      case LORAMAC_HANDLER_BEACON_NRX:
      {
        APP_LOG(TS_OFF, VLEVEL_M, "\r\n###### BEACON NOT RECEIVED\r\n");
        break;
      }
    }
  }
  /* USER CODE END OnBeaconStatusChange_1 */
}

static void OnSysTimeUpdate(void)
{
  /* USER CODE BEGIN OnSysTimeUpdate_1 */

  /* USER CODE END OnSysTimeUpdate_1 */
}

static void OnClassChange(DeviceClass_t deviceClass)
{
  /* USER CODE BEGIN OnClassChange_1 */
  APP_LOG(TS_OFF, VLEVEL_M, "Switch to Class %c done\r\n", "ABC"[deviceClass]);
  /* USER CODE END OnClassChange_1 */
}

static void OnMacProcessNotify(void)
{
  /* USER CODE BEGIN OnMacProcessNotify_1 */

  /* USER CODE END OnMacProcessNotify_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LmHandlerProcess), CFG_SEQ_Prio_0);

  /* USER CODE BEGIN OnMacProcessNotify_2 */

  /* USER CODE END OnMacProcessNotify_2 */
}

static void OnTxPeriodicityChanged(uint32_t periodicity)
{
  /* USER CODE BEGIN OnTxPeriodicityChanged_1 */

  /* USER CODE END OnTxPeriodicityChanged_1 */
  TxPeriodicity = periodicity;

  if (TxPeriodicity == 0)
  {
    /* Revert to application default periodicity */
    TxPeriodicity = APP_TX_DUTYCYCLE;
  }

  /* Update timer periodicity */
  UTIL_TIMER_Stop(&TxTimer);
  UTIL_TIMER_SetPeriod(&TxTimer, TxPeriodicity);
  UTIL_TIMER_Start(&TxTimer);
  /* USER CODE BEGIN OnTxPeriodicityChanged_2 */

  /* USER CODE END OnTxPeriodicityChanged_2 */
}

static void OnTxFrameCtrlChanged(LmHandlerMsgTypes_t isTxConfirmed)
{
  /* USER CODE BEGIN OnTxFrameCtrlChanged_1 */

  /* USER CODE END OnTxFrameCtrlChanged_1 */
  LmHandlerParams.IsTxConfirmed = isTxConfirmed;
  /* USER CODE BEGIN OnTxFrameCtrlChanged_2 */

  /* USER CODE END OnTxFrameCtrlChanged_2 */
}

static void OnPingSlotPeriodicityChanged(uint8_t pingSlotPeriodicity)
{
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_1 */

  /* USER CODE END OnPingSlotPeriodicityChanged_1 */
  LmHandlerParams.PingSlotPeriodicity = pingSlotPeriodicity;
  /* USER CODE BEGIN OnPingSlotPeriodicityChanged_2 */

  /* USER CODE END OnPingSlotPeriodicityChanged_2 */
}

static void OnSystemReset(void)
{
  /* USER CODE BEGIN OnSystemReset_1 */

  /* USER CODE END OnSystemReset_1 */
  if ((LORAMAC_HANDLER_SUCCESS == LmHandlerHalt()) && (LmHandlerJoinStatus() == LORAMAC_HANDLER_SET))
  {
    NVIC_SystemReset();
  }
  /* USER CODE BEGIN OnSystemReset_Last */

  /* USER CODE END OnSystemReset_Last */
}

static void StopJoin(void)
{
  /* USER CODE BEGIN StopJoin_1 */
  /* USER CODE END StopJoin_1 */

  UTIL_TIMER_Stop(&TxTimer);

  if (LORAMAC_HANDLER_SUCCESS != LmHandlerStop())
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stop on going ...\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "LmHandler Stopped\r\n");
    if (LORAWAN_DEFAULT_ACTIVATION_TYPE == ACTIVATION_TYPE_ABP)
    {
      ActivationType = ACTIVATION_TYPE_OTAA;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to OTAA mode\r\n");
    }
    else
    {
      ActivationType = ACTIVATION_TYPE_ABP;
      APP_LOG(TS_OFF, VLEVEL_M, "LmHandler switch to ABP mode\r\n");
    }
    LmHandlerConfigure(&LmHandlerParams);
    LmHandlerJoin(ActivationType, true);
    UTIL_TIMER_Start(&TxTimer);
  }
  UTIL_TIMER_Start(&StopJoinTimer);
  /* USER CODE BEGIN StopJoin_Last */

  /* USER CODE END StopJoin_Last */
}

static void OnStopJoinTimerEvent(void *context)
{
  /* USER CODE BEGIN OnStopJoinTimerEvent_1 */

  /* USER CODE END OnStopJoinTimerEvent_1 */
  if (ActivationType == LORAWAN_DEFAULT_ACTIVATION_TYPE)
  {
    UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LoRaStopJoinEvent), CFG_SEQ_Prio_0);
  }
  /* USER CODE BEGIN OnStopJoinTimerEvent_Last */
  /* USER CODE END OnStopJoinTimerEvent_Last */
}

static void StoreContext(void)
{
  LmHandlerErrorStatus_t status = LORAMAC_HANDLER_ERROR;

  /* USER CODE BEGIN StoreContext_1 */

  /* USER CODE END StoreContext_1 */
  status = LmHandlerNvmDataStore();

  if (status == LORAMAC_HANDLER_NVM_DATA_UP_TO_DATE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA UP TO DATE\r\n");
  }
  else if (status == LORAMAC_HANDLER_ERROR)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORE FAILED\r\n");
  }
  /* USER CODE BEGIN StoreContext_Last */

  /* USER CODE END StoreContext_Last */
}

static void OnNvmDataChange(LmHandlerNvmContextStates_t state)
{
  /* USER CODE BEGIN OnNvmDataChange_1 */

  /* USER CODE END OnNvmDataChange_1 */
  if (state == LORAMAC_HANDLER_NVM_STORE)
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA STORED\r\n");
  }
  else
  {
    APP_LOG(TS_OFF, VLEVEL_M, "NVM DATA RESTORED\r\n");
  }
  /* USER CODE BEGIN OnNvmDataChange_Last */

  /* USER CODE END OnNvmDataChange_Last */
}

static void OnStoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnStoreContextRequest_1 */

  /* USER CODE END OnStoreContextRequest_1 */
  /* store nvm in flash */
  if (FLASH_IF_Erase(LORAWAN_NVM_BASE_ADDRESS, FLASH_PAGE_SIZE) == FLASH_IF_OK)
  {
    FLASH_IF_Write(LORAWAN_NVM_BASE_ADDRESS, (const void *)nvm, nvm_size);
  }
  /* USER CODE BEGIN OnStoreContextRequest_Last */

  /* USER CODE END OnStoreContextRequest_Last */
}

static void OnRestoreContextRequest(void *nvm, uint32_t nvm_size)
{
  /* USER CODE BEGIN OnRestoreContextRequest_1 */

  /* USER CODE END OnRestoreContextRequest_1 */
  FLASH_IF_Read(nvm, LORAWAN_NVM_BASE_ADDRESS, nvm_size);
  /* USER CODE BEGIN OnRestoreContextRequest_Last */

  /* USER CODE END OnRestoreContextRequest_Last */
}

