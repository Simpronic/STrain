/*
    NeaPolis Innovation Summer Campus Examples
    Copyright (C) 2020-2023 Salvatore Dello Iacono [delloiaconos@gmail.com]
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

/*
 * [NISC2023-SERIAL00] - ChibiOS/HAL SERIAL Driver Example 00.
 * DESCRIPTION: Simple Echoback example!
 */

#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include "string.h"
#include "defines.h"
#include "ssd1306.h"
#include "rf.h"

/*GLOBAL DEFINITIONS*/

enum system_state state = SAFE_S; //Stato globale del sistema
enum m_state misur_state = N_DONE_S;
BaseSequentialStream * chp = (BaseSequentialStream *) &SD2; //Driver per la scrittura a terminale
static SSD1306Driver SSD1306D1; //Driver del display oled
char buff[BUFF_SIZE]; //Buffer di scrittura sull'oled
float lastdistance = 0.0f;

/*Utilty functions*/
void create_msg_from_state(char msg[],uint8_t dim){ //Crea il messaggio in base allo stato
  switch(state){
  case SLOW_S:
    strcpy(msg,SLOW_MSG);
    break;
  case STOP_S:
    strcpy(msg,STOP_MSG);
    break;
  case SAFE_S:
    strcpy(msg,OK_MSG);
    break;
  default:
    strcpy(msg,"NO_MSG");
    break;
  }
  msg[dim-1] = '\n';
}
/*End Utility functions*/

/*Threads*/
THD_WORKING_AREA( waMIS, WA_MIS_SIZE);
THD_FUNCTION( thdMIS, arg ){
  (void) arg;
  uint8_t critical_misuration = 0;
  uint8_t misurations = 0;
  float distance = 0.0f;

  while(misurations < MAX_MISURATION){
      misur_state = N_DONE_S;
      palWriteLine(LINE_TRIGGER, PAL_HIGH);
      chThdSleepMicroseconds(TIME_MISURATION_MICROSECONDS);
      palWriteLine(LINE_TRIGGER, PAL_LOW);
      while(misur_state == N_DONE_S){}
      distance = lastdistance;
      chprintf(chp, "Distance: %.2f cm\n\r", distance);
      misurations++;
      if(distance <= TOLLERANZA){
        critical_misuration++;
      }
  }

  if(critical_misuration >= CRITICAL_M_SLOW && critical_misuration < CRITICAL_M_STOP){
    chprintf(chp, "Slow State \n\r");
    state = SLOW_S;
  }else if(critical_misuration >= CRITICAL_M_STOP){
    chprintf(chp, "Stop State \n\r");
    state = STOP_S;
  }else{
    chprintf(chp, "Safe State \n\r");
    state = SAFE_S;
  }
}

THD_WORKING_AREA( waNRF, WA_NRF_SIZE);
THD_FUNCTION( thdNRF, arg ) {
  (void) arg;
  rfStart(&RFD1, &nrf24l01_cfg);
  rf_msg_t msg;
  char msg_to_send[6];
  uint8_t retries = 0;
  while(true){
    retries = 0;
    if(state == SLOW_S || state == STOP_S || state == SAFE_S){
      while(retries < MAX_RETRY ){
          create_msg_from_state(msg_to_send,6);
          ssd1306FillScreen(&SSD1306D1,SSD1306_COLOR_WHITE);
          msg = rfTransmitString(&RFD1, msg_to_send, "TavB5", TIME_INFINITE);
          if(msg == RF_OK){
            ssd1306GotoXy(&SSD1306D1, 0, 1);
            chsnprintf(buff, 7, "OK MSG");
            ssd1306Puts(&SSD1306D1,buff,&ssd1306_font_7x10, SSD1306_COLOR_WHITE);
            chprintf(chp, "MESSAGGIO INVIATO \n\r");
            retries = MAX_RETRY;
          }
          else if(msg == RF_ERROR){
            ssd1306GotoXy(&SSD1306D1, 0, 1);
            chsnprintf(buff, 7, "ERROR");
            ssd1306Puts(&SSD1306D1,buff,&ssd1306_font_7x10, SSD1306_COLOR_WHITE);
            chprintf(chp, "MESSAGGIO NON INVIATO \n\r");
            retries++;
          }
          else{
            ssd1306GotoXy(&SSD1306D1, 0, 1);
            chsnprintf(buff, 7, "TM_OUT");
            ssd1306Puts(&SSD1306D1,buff,&ssd1306_font_7x10, SSD1306_COLOR_WHITE);
            chprintf(chp, "MESSAGGIO NON INVIATA TIMEOUT \n\r");
          }
        }
      if(retries == MAX_RETRY && msg == RF_ERROR){
         ssd1306GotoXy(&SSD1306D1, 0, 1);
         chsnprintf(buff, 18, "NON RAGGIUNGIBILE");
         ssd1306Puts(&SSD1306D1,buff,&ssd1306_font_7x10, SSD1306_COLOR_WHITE);
         chprintf(chp, "Periferica non raggiungibile \n\r");
         
       }
      if(state == STOP_S){
         ssd1306GotoXy(&SSD1306D1, 0, 20);
         chsnprintf(buff, 12, "Pericolo !!");
         ssd1306Puts(&SSD1306D1,buff,&ssd1306_font_7x10, SSD1306_COLOR_WHITE);
      }
      state = MISURATION_S;
      ssd1306UpdateScreen(&SSD1306D1);
    }
    chThdSleepMilliseconds(100);
  }
  rfStop(&RFD1);
}

/*Callbacks*/

static void shock_cb(void *arg) {

  (void)arg;
  chSysLockFromISR();
  if(state == SAFE_S ){
    while(!palReadLine(SHOCK_SENSOR)){}
    //chprintf(chp, "Misuration State \n\r");
    state = MISURATION_S;
  }
  chSysUnlockFromISR();
}

static void icuwidthcb(ICUDriver *icup) {
    icucnt_t width = icuGetWidthX(icup);
    lastdistance = (SPEED_OF_SOUND * width * M_TO_CM) / (ICU_TIM_FREQ * 2);
    misur_state = DONE_S;
}

/*End Threads */
/*
 * Application entry point.
 */
int main(void) {

  thread_t *th_w;

  halInit();
  chSysInit();

  palSetPadMode( GPIOA, 2, PAL_MODE_ALTERNATE(7) );
  palSetPadMode( GPIOA, 3, PAL_MODE_ALTERNATE(7) );

  palSetLineMode(NRF_SCK_LINE,  PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(NRF_MISO_LINE, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(NRF_MOSI_LINE, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(NRF_SPI_CS,PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);

  palSetLineMode(SHOCK_SENSOR,PAL_MODE_INPUT);
  palEnableLineEvent( SHOCK_SENSOR, PAL_EVENT_MODE_BOTH_EDGES);
  palSetLineCallback( SHOCK_SENSOR, shock_cb, NULL);
    /*
     * CE and IRQ pins setup.
     */
  palSetLineMode(NRF_CE_LINE,  PAL_MODE_OUTPUT_PUSHPULL |PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(NRF_IRQ_LINE, PAL_MODE_INPUT | PAL_STM32_OSPEED_HIGHEST);


  palSetLineMode(PAL_LINE(GPIOB, 8U), PAL_MODE_ALTERNATE(4) |
                    PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                    PAL_STM32_PUPDR_PULLUP);

  palSetLineMode(PAL_LINE(GPIOB, 9U), PAL_MODE_ALTERNATE(4) |
                    PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                    PAL_STM32_PUPDR_PULLUP);

  sdStart( &SD2, NULL );
  rfInit();

  ssd1306ObjectInit(&SSD1306D1);
  ssd1306Start(&SSD1306D1, &ssd1306cfg);
  ssd1306FillScreen(&SSD1306D1, SSD1306_COLOR_WHITE);
  chThdCreateStatic( waNRF, sizeof(waNRF), NORMALPRIO + 1, thdNRF, NULL);
  palSetLineMode(LINE_ECHO, PAL_MODE_ALTERNATE(6));
  icuStart(&ICUD1, &icucfg);
  icuStartCapture(&ICUD1);
  icuEnableNotifications(&ICUD1);

  palSetLineMode(LINE_TRIGGER, PAL_MODE_OUTPUT_PUSHPULL);

  while(true){
    if(state == MISURATION_S){
      th_w = chThdCreateStatic( waMIS, sizeof(waMIS), NORMALPRIO + 2, thdMIS, NULL);
      chThdWait(th_w);
    }
    chThdSleepMilliseconds(500);
  }
  icuStopCapture(&ICUD1);
  icuStop(&ICUD1);
  ssd1306Stop(&SSD1306D1);
  sdStop(&SD2);
}
