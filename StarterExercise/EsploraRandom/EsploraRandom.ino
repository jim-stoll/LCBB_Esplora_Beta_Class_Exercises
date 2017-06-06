#include <Esplora.h>

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("begin");

  getRandoms();
//  seededRandoms();
//  analogSeededRandoms();
//  esploraSeededRandoms();
}

void getRandoms() {
  for (int x = 0; x < 5; x++) {
    Serial.println(random(10));
    delay(200);
  }
}

void seededRandoms() {
  randomSeed(5);
  getRandoms();
}

void analogSeededRandoms() {
  randomSeed(analogRead(0));
  getRandoms();
}

void esploraSeededRandoms() {
  randomSeed(getRandomSeed());
  getRandoms();
}

unsigned long getRandomSeed() {
  // return value with 32 random bits set
  byte bitNum = 32;
  unsigned long seed=0;
  while (bitNum--) {
    seed = (seed<<1) | ((Esplora.readLightSensor() * Esplora.readAccelerometer(X_AXIS) * Esplora.readAccelerometer(Y_AXIS) * Esplora.readAccelerometer(Z_AXIS))&1);
  }

  return seed;
}

void loop() {

}
