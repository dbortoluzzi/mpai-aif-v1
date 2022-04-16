/**
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

#include "stm32l475e_iot01_audio.h"

HAL_StatusTypeDef MX_DFSDM1_ClockConfig(DFSDM_Channel_HandleTypeDef *hDfsdmChannel, uint32_t SampleRate) {
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hDfsdmChannel);

    HAL_StatusTypeDef        status = HAL_OK;
    RCC_PeriphCLKInitTypeDef RCC_ExCLKInitStruct;

    /* Configure the SAI PLL according to the requested audio frequency */
    /* Retrieve actual RCC configuration */
    HAL_RCCEx_GetPeriphCLKConfig(&RCC_ExCLKInitStruct);

    /* Set the PLL configuration according to the audio frequency */
    /* SAI1 clock config
       PLLSAI1_VCO = (48 MHz / PLLSAI1M) * PLLSAI1N = 48 / 6 * 43 = 344
       SAI_CK_x = PLLSAI1_VCO/PLLSAI1P = 2344 / 7 = 49.142 MHz */
    RCC_ExCLKInitStruct.PeriphClockSelection    = RCC_PERIPHCLK_SAI1;
    RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1Source   = RCC_PLLSOURCE_MSI;
    RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1M        = 6;
    RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1N        = 43;
    RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1P        = 7;
    RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_SAI1CLK;
    RCC_ExCLKInitStruct.Sai1ClockSelection      = RCC_SAI1CLKSOURCE_PLLSAI1;
    status = HAL_RCCEx_PeriphCLKConfig(&RCC_ExCLKInitStruct);

    return status;
}