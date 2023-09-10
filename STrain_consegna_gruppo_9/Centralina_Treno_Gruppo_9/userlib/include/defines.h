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
#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "chscanf.h"
#include "stdio.h"
#include "rf.h"
//#include "image.c"


enum system_state{
  SAFE_S,
  MISURATION_S,
  SLOW_S,
  STOP_S
};

/*
 * ALERT
 */
#define ALERT_ON  1
#define ALERT_OFF 0
#define START_STATE 2
#define RED_LED_LINE PAL_LINE(GPIOC,1)
#define BLUE_LED_LINE PAL_LINE(GPIOC,0)

/*
 * RF Message Define
 */
#define SLOW_MSG "SLOW"
#define STOP_MSG "STOP"
#define OK_MSG "OK"
#define MAX_RETRY 3
#define FRAME_LEN 5
#define BUFF_SIZE 5


/*
 * Motors Define
 */
#define MOTOR_SX PAL_LINE(GPIOB,4U)
#define MOTOR_DX PAL_LINE(GPIOB,5U)
#define MOTOR_OFF 0
#define MOTOR_ON 1
#define DEFAULT_POWER 10000

/*
 * HC-SR04 Define
 */
#define LINE_TRIGGER                PAL_LINE(GPIOB,10U)
#define LINE_ECHO                   PAL_LINE(GPIOA,8U)
#define ANSI_ESCAPE_CODE_ALLOWED    TRUE
#define ICU_TIM_FREQ                1000000
#define M_TO_CM                     100.0f
#define SPEED_OF_SOUND              343.2f
#define SAFE_DISTANCE               40.0f



/*
 * NRF PINOUT
 */
#define NRF_CE_LINE PAL_LINE(GPIOA,9U)
#define NRF_IRQ_LINE PAL_LINE(GPIOC,7U)
#define NRF_MISO_LINE PAL_LINE(GPIOA,6U)
#define NRF_MOSI_LINE PAL_LINE(GPIOA,7U)
#define NRF_SCK_LINE PAL_LINE(GPIOA,5U)
#define NRF_SPI_CS PAL_LINE(GPIOB,6U)

/*
 * SPI Config
 */
static const SPIConfig std_spi_cfg = {
  .circular = FALSE,
  .slave = FALSE,
  .data_cb = NULL,
  .error_cb = NULL,
  .ssline = PAL_LINE(GPIOB,6U),
  SPI_CR1_BR_1 | SPI_CR1_BR_0,                    /* CR1 register */
  0                                               /* CR2 register */
};

/*
 * RF Config
 */
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




/*
 * SSD1306 Config
 */
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

/*
 * Function prototypes
 */
void printSTLogo(SSD1306Driver *devp);
void printAlertMsg(SSD1306Driver *devp);


#endif /* DEFINES_H_ */
