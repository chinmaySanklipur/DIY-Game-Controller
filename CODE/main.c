#include <stdio.h>

void setup() {
    // Initialization code here
    pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
    // Main code here
}

void led_Indicator(int state) {
    digitalWrite(LED_BUILTIN, HIGH);
    
}