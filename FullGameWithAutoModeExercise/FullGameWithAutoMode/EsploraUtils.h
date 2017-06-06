/*
 * Utils.h
 *
 *  Created on: Mar 15, 2017
 *      Author: jstoll
 */
#ifndef ESPLORA_UTILS_H_
#define ESPLORA_UTILS_H_

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  }

char *getFreeRamAsText() {
	static char sBuf[11] = {'\0'};
	sprintf_P(sBuf, PSTR("%d bytes"), freeRam());
	return sBuf;
}

const byte RED_PIN    = 5;
const byte BLUE_PIN   = 9;
const byte GREEN_PIN  = 10;

void rgbWrite(byte r, byte g, byte b, byte i) {
	static byte lastR = 0;
	static byte lastG = 0;
	static byte lastB = 0;
	byte iR = map(r, 0, 255, 0, i);
	byte iG = map(g, 0, 255, 0, i);
	byte iB = map(b, 0, 255, 0, i);

	if (r != lastR) {
		analogWrite(RED_PIN, iR);
		lastR = iR;
	}
	if (g != lastG) {
		analogWrite(GREEN_PIN, iG);
		lastG = iG;
	}
	if (b != lastB) {
		analogWrite(BLUE_PIN, iB);
		lastB = iB;
	}
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

#endif // ESPLORA_UTILS_H_
