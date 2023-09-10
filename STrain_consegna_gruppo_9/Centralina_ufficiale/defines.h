/*
 * defines.h
 *
 *  Created on: 7 set 2023
 *      Author: Marco
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#include "ssd1306.h"
#include "rf.h"

enum system_state{
  SAFE_S,
  MISURATION_S,
  SLOW_S,
  STOP_S
};

enum m_state{
  N_DONE_S,
  DONE_S
};

#define SLOW_MSG "SLOW"
#define STOP_MSG "STOP"
#define OK_MSG "OK"
#define MAX_RETRY 5

#define CRITICAL_M_STOP 3
#define CRITICAL_M_SLOW 2
#define TOLLERANZA  14

#define TIME_MISURATION_MICROSECONDS 1000

/*PIN DEFINES*/
#define NRF_CE_LINE PAL_LINE(GPIOA,9U)
#define NRF_IRQ_LINE PAL_LINE(GPIOC,7U)
#define NRF_MISO_LINE PAL_LINE(GPIOA,6U)
#define NRF_MOSI_LINE PAL_LINE(GPIOA,7U)
#define NRF_SCK_LINE PAL_LINE(GPIOA,5U)
#define NRF_SPI_CS PAL_LINE(GPIOB,6U)

#define SHOCK_SENSOR PAL_LINE(GPIOC,8U)

#define LINE_TRIGGER                PAL_LINE(GPIOB, 10U)
#define LINE_ECHO                   PAL_LINE(GPIOA, 8U)

/*Other defines*/

#define VOLTAGE_RES            ((float)3.3/4096)

#define ADC_GRP_NUM_CHANNELS   1
#define ADC_GRP_BUF_DEPTH      16

#define FRAME_LEN 5
#define BUFF_SIZE 20

#define ICU_TIM_FREQ                1000000
#define M_TO_CM                     100.0f
#define SPEED_OF_SOUND              343.2f

#define MAX_MISURATION              5

#define ANSI_ESCAPE_CODE_ALLOWED    TRUE
/*WorkingArea defines*/
#define WA_NRF_SIZE 1024
#define WA_MIS_SIZE 1024

/*Utility function definition*/
static void icuwidthcb(ICUDriver *icup);
//static void icuwidthcb(ICUDriver *icup);

/*Cnfig structures*/

static const SPIConfig std_spi_cfg = {
  .circular = FALSE,
  .slave = FALSE,
  .data_cb = NULL,
  .error_cb = NULL,
  .ssport = GPIOB,
  .sspad = 6U,                               /* Line of CS. */
  SPI_CR1_BR_1 | SPI_CR1_BR_0,                    /* CR1 register */
  0                                               /* CR2 register */
};

static RFConfig nrf24l01_cfg = {
  .line_ce = NRF_CE_LINE,
  .line_irq = NRF_IRQ_LINE,
  .spip = &SPID1,
  .spicfg = &std_spi_cfg,
  .auto_retr_count = NRF24L01_ARC_15_times,     /* auto_retr_count */
  .auto_retr_delay = NRF24L01_ARD_4000us,       /* auto_retr_delay */
  .address_width = NRF24L01_AW_5_bytes,       /* address_width */
  .channel_freq = 42,                       /* channel_freq 2.4 + 0.13 GHz */
  .data_rate = NRF24L01_ADR_2Mbps,        /* data_rate */
  .out_pwr = NRF24L01_PWR_0dBm,         /* out_pwr */
  .lna = NRF24L01_LNA_disabled,     /* lna */
  .en_dpl = NRF24L01_DPL_enabled ,     /* en_dpl */
  .en_ack_pay = NRF24L01_ACK_PAY_disabled, /* en_ack_pay */
  .en_dyn_ack = NRF24L01_DYN_ACK_disabled  /* en_dyn_ack */
};

static const I2CConfig i2ccfg = {
  // I2C_TIMINGR address offset
  .timingr = 0x10,
  .cr1 = 0,
  .cr2 = 1,
};

static const SSD1306Config ssd1306cfg = {
  &I2CD1,
  &i2ccfg,
  SSD1306_SAD_0X78,
};


static ICUConfig icucfg = {
  ICU_INPUT_ACTIVE_HIGH,
  ICU_TIM_FREQ,                                /* 1MHz ICU clock frequency.   */
  icuwidthcb,
  NULL,
  NULL,
  ICU_CHANNEL_1,
  0,
  0xFFFFFFFFU
};



/*static const ADCConversionGroup adcgrpcfg1 = {
          .circular     = false,
          .num_channels = ADC_GRP_NUM_CHANNELS,
          .end_cb       = NULL,
          .error_cb     = NULL,
          .cfgr         = ADC_CFGR_CONT,
          .cfgr2        = 0U,
          .tr1          = ADC_TR_DISABLED,
          .tr2          = ADC_TR_DISABLED,
          .tr3          = ADC_TR_DISABLED,
          .awd2cr       = 0U,
          .awd3cr       = 0U,
          .smpr         = {
            ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_247P5),
            0U
          },
          .sqr          = {
            ADC_SQR1_SQ1_N(ADC_CHANNEL_IN1),
            0U,
            0U,
            0U
          }
        };*/

#endif /* DEFINES_H_ */
