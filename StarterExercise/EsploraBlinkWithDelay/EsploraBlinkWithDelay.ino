#include <Esplora.h>

bool red = true;

void setup() {

}

void loop() {
  Esplora.writeRed(red);
  delay(1000);
  red = !red;

}
