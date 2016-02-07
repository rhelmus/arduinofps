#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>
#include <SdFat.h>
#include <virtmem.h>
#include <serialram.h>

#include "game.h"

void setup()
{
    pinMode(6, OUTPUT); digitalWrite(6, HIGH);
    pinMode(7, OUTPUT); digitalWrite(7, HIGH);
    pinMode(8, OUTPUT); digitalWrite(8, HIGH);
    pinMode(9, OUTPUT); digitalWrite(9, HIGH);
    pinMode(10, OUTPUT); digitalWrite(10, HIGH);

    delay(2000);

    Serial.begin(115200);

    game.setup();
}

void loop()
{
    game.update();
    delay(5);
}
