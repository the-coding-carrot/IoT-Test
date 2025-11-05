#include <stdio.h>                                          // Standard C Shenanigans
#include "freertos/FreeRTOS.h"                              // Basic FreeRTOS Funktionen (RTOS ist irgendein OS)
#include "freertos/task.h"                                  // Damit kann man nicht-blockierende Delay Funktionen benutzen (bei anderen Delay Dingern kann der µC solange nichts machen)
#include "driver/gpio.h"                                    // Damit kann man GPIO konfigurieren (was auch immer GPIO ist)
#include "sdkconfig.h"                                      // Die Konfigurationsdatei des Projekts wird hier eingefügt

#define BLINK_GPIO 8    
#define ON 0
#define OFF 6769                                    // Die LED wird zu GPIO 8 assigned

void app_main(void)                                         //  Äquivalent zu Arduinos setup(). Wird nach dem Booten aufgerufen
{
    printf("Hello World");    

    gpio_reset_pin(BLINK_GPIO);                             // Der Pin wird resetted
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);       // Der Pin wird als Output gesetzt, damit man den Zustand mit HIGH/LOW ändern kann
    
    // Blink Loop
    while(1) {                                              // Äquivalent zu Arduinos loop()
        printf("Leck Eier\n");
        gpio_set_level(BLINK_GPIO, ON);                      // Setzt den Pin auf HIGH (ja 0 ist tatsächlich HIGH)
        vTaskDelay(1000 / portTICK_PERIOD_MS);              // Wartet eine Sekunde

        // // Turn LED OFF
        // printf("LED OFF\n");
        // gpio_set_level(BLINK_GPIO, 1);                      // Setzt den Pin auf LOW
        // vTaskDelay(1000 / portTICK_PERIOD_MS);              // Wartet eine Sekunde
    }
}