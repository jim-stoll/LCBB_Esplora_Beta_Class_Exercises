#include <Esplora.h>

bool red = true;
unsigned long lastMillis = 0;

void setup() {

}

void loop() {
  if (millis() - lastMillis > 1000) {
    red = !red;
    Esplora.writeRed(red);
    lastMillis = millis();
  }

  if (Esplora.readButton(SWITCH_DOWN) == LOW) {
    Esplora.writeGreen(true);
  } else {
    Esplora.writeGreen(false);
  }
}
