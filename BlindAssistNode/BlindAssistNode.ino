#include "blefind.h"
/* Basic Multi Threading Arduino Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
// Please read file README.md in the folder containing this example.

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define ANALOG_INPUT_PIN 1

#ifndef LED_BUILTIN
  #define LED_BUILTIN 2 // Specify the on which is your LED
#endif

// Define two tasks for Blink & AnalogRead.
void TaskBlink( void *pvParameters );

// The setup function runs once when you press reset or power on the board.
void setup() {
  // Initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  delay(500);
inti_fs(); // setup file system
init_blefind(); // enable ble to advertise and also scan
delay(500);
// create task
  xTaskCreatePinnedToCore(
    Blescan
    ,  "Blescan" // A name just for humans
    ,  2048        // The stack size can be checked by calling `uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);`
    ,  NULL // Task parameter which can modify the task behavior. This must be passed as pointer to void.
    ,  1  // Priority
    ,  &Blescan_task_handle // Task handle
    , ARDUINO_RUNNING_CORE
    );


  vTaskDelete(NULL);
}

void loop(){
 vTaskDelete(NULL);
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void Blescan(void *pvParameters){  // This is a task.
// UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
// debug("Ble scan memory at begining ");
// debugln(uxHighWaterMark);
//init_blefind(); // enable ble to advertise and also scan

  for (;;){ // A Task shall never return or exit.
start_scan();
  }
}


