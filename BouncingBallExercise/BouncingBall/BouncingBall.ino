#include <Esplora.h>
#include <TFT.h>

#include "EsploraUtils.h"

const int screenW = EsploraTFT.width();		//convenience const for screen width
const int screenH = EsploraTFT.height();	//convenience const for screen height
const int screenTopY = 0;					//top of the play area of the screen (above this is the status bar info)

const int ballW = 4;					 	//ball size in px (ball is assumed to be square)
int ballX = 0;								//horizontal position on screen of top left corner of ball
int ballY = 0;								//vertical position on screen of top left corner of ball
int ballXComp = 1;							//X component of ball vector (negative = movement left, positive = movement right)
int ballYComp = 2;							//Y component of ball vector (negative = movement up the screen, positive = movement down the screen)
unsigned long ballProcessDelay = 25;					 //milliseconds between processing of ball - controls ball speed (set from mode params)

//the setup method of the program - runs just once, when power is first applied (or after reset)
void setup() {
//	Serial.begin(115200);
	//seed the random generator from as random a source as possible on the Esplora (no unused Analog inputs - the usual approach)
	randomSeed(getRandomSeed());

	EsploraTFT.begin();

	newGame();
}

//the main loop of the program - executes continuously, as long as the device is powered
void loop() {
	processBall();
}

void processBall(void) {
	static unsigned long lastProcessMillis = millis();
	static int lastBallX = 0;							//prior X position of ball - used for erasing ball when it moves to a new position
	static int lastBallY = screenH - ballW;


	if (millis() - lastProcessMillis > ballProcessDelay) {
		lastProcessMillis = millis();

		//check if the ball hits the side walls
		if ((ballX < ballXComp * -1) || (ballX > screenW - ballXComp - ballW)) {
			ballXComp = -ballXComp;
		}
		//check if the ball hits the top of the screen
		if (ballY <= screenTopY) {
			//at top of screen ball direction will always be postive (ran into bug trying to just complement the direction)
			ballYComp = abs(ballYComp);
		}

		//check if the ball hits the bottom of the screen
		if (ballY >= screenH) {
			ballYComp = -ballYComp;
		}

		//calculate the new position for the ball
		ballX = ballX + ballXComp;	//move the ball x
		ballY = ballY + ballYComp;	//move the ball y

		//erase the old ball
		EsploraTFT.fill(0, 0, 0);
		EsploraTFT.rect(lastBallX, lastBallY, ballW, ballW);

		// draw the new ball
		EsploraTFT.fill(255, 255, 255);
		EsploraTFT.rect(ballX, ballY, ballW, ballW);

		//update the last ball position to the new ball position
		lastBallX = ballX;
		lastBallY = ballY;

	}
}

void newGame() {
	//clear the screen
	EsploraTFT.background(0, 0, 0);

	newScreen();

}

//setup screen for next ball
void newScreen(void) {
	newBall();
}

void newBall() {
	ballX = random(screenW - ballW);
	ballY = screenTopY;
	ballXComp = pow(-1, random(2));
	ballYComp = abs(ballYComp);
}

