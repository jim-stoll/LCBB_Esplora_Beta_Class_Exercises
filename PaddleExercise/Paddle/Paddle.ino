#include <Esplora.h>
#include <TFT.h>

#include "EsploraUtils.h"

const int screenW = EsploraTFT.width();		//convenience const for screen width
const int screenH = EsploraTFT.height();	//convenience const for screen height
const int paddleH = 4;						//height in px of paddle

int paddleW = 20;							//paddle width in px - set from mode params, then shrinks at each level progression (if/as specified in mode params)
int paddleX = 0;							//horizontal position on screen of top left corner of paddle
const int paddleY = screenH - paddleH;		//vertical position on screen of top left corner of paddle
int lastPaddleX = 0;						//used to erase last paddle position, and determine if paddle has moved (for redraw)

//the setup method of the program - runs just once, when power is first applied (or after reset)
void setup() {
//	Serial.begin(115200);

	EsploraTFT.begin();

	newGame();
}

//the main loop of the program - executes continuously, as long as the device is powered
void loop() {
	readPaddle();
	drawPaddle();
}

//erase the old paddle position, and draw new paddle at new position
void drawPaddle() {
	//only draw the paddle if it has moved from its last position
	// (if redraw every time, the paddle flashes/strobes due to the continual rapid redrawing)
	if (paddleX != lastPaddleX) {

		EsploraTFT.fill(0, 0, 0);									//set fill color to black
		EsploraTFT.rect(lastPaddleX, paddleY, paddleW, paddleH);	//erase the paddle at its last position (by drawing a black paddle-sized rectangle at its old position)
		EsploraTFT.fill(255, 255, 255);								//set fill color to white
		EsploraTFT.rect(paddleX, paddleY, paddleW, paddleH);		//draw the paddle at its new position

		//save the current position as the 'last' position, so can compare next time, to see if it has moved
		lastPaddleX = paddleX;
	}

}

//obtain paddle position from appropriate input device
void readPaddle() {
	readPaddleSlider();

	//prevent paddle from moving off left or right sides of screen
	if (paddleX < 0) {
		paddleX = 0;
	} else if (paddleX > screenW - paddleW) {
		paddleX = screenW - paddleW;
	}

}

//update paddle position from slider
void readPaddleSlider() {
	//read the slider, map it to the screen width, then subtract the width of the paddle
	//this gives us the position relative the left corner of the paddle
	paddleX = map(Esplora.readSlider(), 0, 1023, screenW - paddleW, 0);
}

//REFACTOR - examine the *paddle methods, and make sure they're being used/called sensibly
//Setup a new paddle (called at start of game, after lost ball and between levels)
void newPaddle() {

	//erase the entire paddle area
	EsploraTFT.fill(0, 0, 0);
	EsploraTFT.rect(0, paddleY, screenW, paddleH);
	EsploraTFT.noStroke();

	lastPaddleX = -1;		//set to impossible last position, so sure to draw the new paddle

}

void newGame() {
	//clear the screen
	EsploraTFT.background(0, 0, 0);

	newScreen();
}

//setup screen for next ball
void newScreen(void) {
	readPaddle();
	newPaddle();
	drawPaddle();
}

