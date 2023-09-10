/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "defines.h"

/*
 * RF
 */
static char buff_msg[BUFF_SIZE];
rf_msg_t ricevuto;

/*
 * Alert flag
 */
 int alert;


/*
 * OLED Config
 */
static SSD1306Driver SSD1306D1;


/*
 * HC-SR04
 */
semaphore_t distanceSensor;
static float lastdistance = 0.0;

static void icuwidthcb(ICUDriver *icup) {

  icucnt_t width = icuGetWidthX(icup);
  lastdistance = (SPEED_OF_SOUND * width * M_TO_CM) / (ICU_TIM_FREQ * 2);
}

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


/*
 * Serial
 */
BaseSequentialStream * chp = (BaseSequentialStream *) &SD2;


/*
 * PWM CONFIG - MOTORS
 */
#define PWM_TIMER_FREQUENCY     10000
#define PWM_PERIOD              50

void pwmWidtchCb(PWMDriver *pwmp){
  (void)pwmp;
}

int pwm_power = 0;

static PWMConfig pwmcfg = {
  PWM_TIMER_FREQUENCY,
  PWM_PERIOD,
  pwmWidtchCb,
  {
   {PWM_OUTPUT_ACTIVE_HIGH, NULL},//canale 1 motore sx
   {PWM_OUTPUT_ACTIVE_HIGH, NULL},//canale 2 motore dx
   {PWM_OUTPUT_DISABLED, NULL},
   {PWM_OUTPUT_DISABLED, NULL}
  },
  0,
  0,
  0
};

/*
 * Motor Threads (Gestisce la potenza fornita ai motori mediante pwm ed effettua un controllo in retroazione
 * di tipo proporzionale per la decelerazione in caso di alert)
 */
#define WA_MOTOR 512
static THD_WORKING_AREA(waMotor, WA_MOTOR);
static THD_FUNCTION(MotorTh, arg) {

  (void)arg;
  chRegSetThreadName("Motor");
  float relative_distance = 0;
  float kp = 0.3;
  float kp_error = 0;
  pwm_power = 0;


  pwmStart(&PWMD3, &pwmcfg);
  pwmEnablePeriodicNotification(&PWMD3);

  /*
   * Fase di Start (Attesa del messaggio di "OK"
   */
  palSetLine(RED_LED_LINE);
  palSetLine(BLUE_LED_LINE);

  while(alert == ALERT_ON || alert == START_STATE){
    chprintf(chp,"ALERT %d\n\r",alert);
    chThdSleepMilliseconds(300);
  }
  pwm_power = DEFAULT_POWER; //Start value
  while (true) {
    /*Controllore proporzionale*/
    if(lastdistance < 40 && alert == ALERT_ON){
      relative_distance = SAFE_DISTANCE - lastdistance;
      kp_error = relative_distance * kp * 130;
      //chprintf(chp, "Distance: %.2f cm  kp_error: %.2f   power: %d\n\r", lastdistance,kp_error,pwm_power);
      pwm_power = pwm_power - kp_error;
    /*Rallentamento in caso di alert al di sopra dei 40cm dall'ostacolo*/
    }else if(lastdistance >= 40 && alert == ALERT_ON){
      pwm_power = DEFAULT_POWER*(3/4);
    /*Andamento in assenza di alert*/
    }else if(alert == ALERT_OFF){
      pwm_power = DEFAULT_POWER;
    }

    /*Reset della potenza in caso di arresto*/
    if(pwm_power<0){
      pwm_power = 0;
      alert = START_STATE;
    }

    pwmEnableChannel(&PWMD3, 1, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, pwm_power)); //Motore destro
    pwmEnableChannel(&PWMD3, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD3, pwm_power)); //Motore sinistro
    chThdSleepMilliseconds(100);
  }
}


/*
 * Distance Sensor Thread (Gestisce il rilevamento della distanza dall'ostacolo, esso esegue unicamente in presenza di alert)
 */
#define WA_DISTANCE 512
static THD_WORKING_AREA(waDistanceSens, WA_DISTANCE);
static THD_FUNCTION(DistanceSensTh, arg) {

  (void)arg;
  chRegSetThreadName("Distance Sensor");

    while(true){
      /* Triggering */
      if(alert == 0){
        chSemWait(&distanceSensor);
      }
      palWriteLine(LINE_TRIGGER, PAL_HIGH);
      chThdSleepMicroseconds(10);
      palWriteLine(LINE_TRIGGER, PAL_LOW);

      chThdSleepMilliseconds(100);
    }
}

/*
 * NRF Thread (Esso gesisce la comunicazione RF con la centralina di rilevamento ostacoli,
 * gestisce la stampa su OLED del'eventuale avviso di allerta, gestisce l'accensione dei led di allerta)
 */
#define WA_NRF_SIZE 1024
THD_WORKING_AREA( waNRF, WA_NRF_SIZE);
THD_FUNCTION( NRFTh, arg ) {
  (void) arg;
  /*
   * OLED
   */
  ssd1306ObjectInit(&SSD1306D1);
  ssd1306Start(&SSD1306D1, &ssd1306cfg);
  ssd1306FillScreen(&SSD1306D1, 0x00);

  rfInit();
  rfStart(&RFD1, &nrf24l01_cfg);

  printSTLogo(&SSD1306D1);
  while(true){

    ricevuto = rfReceiveString(&RFD1, buff_msg, "TavB5", TIME_INFINITE);

    if(ricevuto == RF_OK){
      if(strcmp(buff_msg,STOP_MSG) == 0 && alert == 0){
        chprintf(chp,"ALERT %d \n\r",alert);
        chprintf(chp, "%s \n\r",buff_msg);
        alert = 1;
        palSetLine(RED_LED_LINE);
        palClearLine(BLUE_LED_LINE);
        printAlertMsg(&SSD1306D1);
        chSemSignal(&distanceSensor);

      }else if(strcmp(buff_msg,OK_MSG) == 0 ){
        chprintf(chp,"OK %d \n\r",alert);
        alert = 0;
        palClearLine(RED_LED_LINE);
        palSetLine(BLUE_LED_LINE);
        printSTLogo(&SSD1306D1);
      }
    }else{
      chprintf(chp, "NO MESSAGE \n\r");
    }
    chThdSleepMilliseconds(100);
  }

}



/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * LED init
   */
  palSetLineMode(RED_LED_LINE,PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(BLUE_LED_LINE,PAL_MODE_OUTPUT_PUSHPULL);
  palClearLine(RED_LED_LINE);
  palClearLine(BLUE_LED_LINE);

  /*
   * Initializes the ICU driver 1. The ICU input i PA8.
   */
  palSetLineMode(LINE_ECHO, PAL_MODE_ALTERNATE(6));
  icuStart(&ICUD1, &icucfg);
  icuStartCapture(&ICUD1);
  icuEnableNotifications(&ICUD1);
  palSetLineMode(LINE_TRIGGER, PAL_MODE_OUTPUT_PUSHPULL);

  /*
   * Starting Serial Driver n.2
   */
  palSetPadMode( GPIOA, 2, PAL_MODE_ALTERNATE(7) );
  palSetPadMode( GPIOA, 3, PAL_MODE_ALTERNATE(7) );
  sdStart( &SD2, NULL );


  /*
   * Configuring I2C related PINs
   */
  palSetLineMode(PAL_LINE(GPIOB, 8U), PAL_MODE_ALTERNATE(4) |
                 PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                 PAL_STM32_PUPDR_PULLUP);

  palSetLineMode(PAL_LINE(GPIOB, 9U), PAL_MODE_ALTERNATE(4) |
                 PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                 PAL_STM32_PUPDR_PULLUP);

  /*
   * MOTOR PWM LINES SETUP
   */
  palSetLineMode(MOTOR_SX,PAL_MODE_ALTERNATE(2));
  palSetLineMode(MOTOR_DX,PAL_MODE_ALTERNATE(2));

  /*
   * RF
   */
  palSetLineMode(NRF_SCK_LINE,  PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(NRF_MISO_LINE, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(NRF_MOSI_LINE, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(NRF_SPI_CS,   PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  /*
   * CE and IRQ pins setup.
   */
  palSetLineMode(NRF_CE_LINE,  PAL_MODE_OUTPUT_PUSHPULL |PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(NRF_IRQ_LINE, PAL_MODE_INPUT | PAL_STM32_OSPEED_HIGHEST);

  /*
   * Semaphore init
   */
  chSemObjectInit(&distanceSensor, 0);

  /*
   * Initialize Alert flag
   */
  alert = START_STATE;

  /*
   * Create threads.
   */
  chThdCreateStatic(waMotor, sizeof(waMotor), NORMALPRIO, MotorTh, NULL);
  chThdCreateStatic(waDistanceSens, sizeof(waDistanceSens), NORMALPRIO, DistanceSensTh, NULL);
  chThdCreateStatic(waNRF, sizeof(waNRF), NORMALPRIO, NRFTh, NULL);

  while (true) {
    chThdSleepMilliseconds(500);
  }
}

