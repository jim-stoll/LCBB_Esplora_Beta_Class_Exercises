#include <Esplora.h>
#include <TFT.h>

#include "EsploraUtils.h"

//** UNCOMMENT the next 2 lines, if working in the Arduino IDE
enum paddleModeEnum {JOYSTICK, SLIDER, TILT} paddleMode = TILT;

const char paddleModeStringJoystick[] = "Joystk";
const char paddleModeStringSlider[] =   "Slider";
const char paddleModeStringTilt[] =     "Tilt";

const char* modeStrings[] = {paddleModeStringJoystick, paddleModeStringSlider, paddleModeStringTilt};

const char modeLbl[] = "";

const int screenW = EsploraTFT.width();		//convenience const for screen width
const int screenH = EsploraTFT.height();	//convenience const for screen height
const int screenTopY = 10;					//top of the play area of the screen (above this is the status bar info)

const int ballW = 4;					 	//ball size in px (ball is assumed to be square)
int ballX = 0;								//horizontal position on screen of top left corner of ball
int ballY = 0;								//vertical position on screen of top left corner of ball
int ballXComp = 1;							//X component of ball vector (negative = movement left, positive = movement right)
int ballYComp = 2;							//Y component of ball vector (negative = movement up the screen, positive = movement down the screen)
unsigned long ballProcessDelay = 25;					 //milliseconds between processing of ball - controls ball speed (set from mode params)

const int paddleH = 4;						//height in px of paddle
int paddleW = 20;							//paddle width in px - set from mode params, then shrinks at each level progression (if/as specified in mode params)
int paddleX = 0;							//horizontal position on screen of top left corner of paddle
const int paddleY = screenH - paddleH;		//vertical position on screen of top left corner of paddle
int lastPaddleX = 0;						//used to erase last paddle position, and determine if paddle has moved (for redraw)

//constants for text positioning of labels and values
// these are defined as global consts because they will be used at other times in the program
const int statusY = 2;						//all status bar text and values will be at the same Y position (at top of screen)
const int modeLblX = 2;

//calculated constants for text positions of status values (these assume a font that is 6 px wide per character)
const int modeX = modeLblX + strlen(modeLbl) * 6 + 2;

int tiltZeroOffset = 0;							//level offset used to compensate for a non-level 'zero' position when tilt mode is selected

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
	readPaddle();
	drawPaddle();

	processBall();
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
	switch (paddleMode) {
		case JOYSTICK:
			readPaddleJoystick();
			break;

		case SLIDER:
			readPaddleSlider();
			break;

		case TILT:
			readPaddleTilt();
			break;
	}

}

//update paddle position from slider
void readPaddleSlider() {
	//read the slider, map it to the screen width, then subtract the width of the paddle
	//this gives us the position relative the left corner of the paddle
	paddleX = map(Esplora.readSlider(), 0, 1023, screenW - paddleW, 0);
}

//update paddle position from X Axis tilt
void readPaddleTilt() {
	static const int tiltDeadZone = 2;					//amount of wiggle room allowed, in which tilt movement is ignored (to prevent a hyper-sensitive tilt response)
	static const unsigned long tiltDelayMillis = 2;		//milliseconds delay between processing tilt reading (again, to de-tune the tilt response a bit)
	static const int maxRange = 4;						//max amount to add to paddle position on each read (smaller value leads to less drastic paddle movement)
	static const float smoothAlpha = 0.67;				//smoothing factor, > 0 and < 1 - larger value dampens movement more, by weighting prior value more than current value
	static int smoothVal = 0;							//smoothed value, calculated from current value, last value (because its static) and smoothing factor
	static unsigned long lastMillis = 0;				//last time the tilt value was read
	int tiltVal = Esplora.readAccelerometer(X_AXIS) - tiltZeroOffset;	//the actual, 'raw' value of the tilt sensor (-512 to 512)

	//only read the tilt sensor every so often (too often results in fast/jerky movement)
	if (millis() - lastMillis > tiltDelayMillis) {

		//if haven't moved out of the 'dead' zone (right in the middle), don't move the paddle - provides some stability while trying to hold still
		if (abs(tiltVal) > tiltDeadZone) {

			//keep tiltVal within +- maxRange
			if (abs(tiltVal) > maxRange) {
				//(val > 0) - (val < 0) provides a 'sign' value, where val< 0=>-1, val>0=>1
				tiltVal = (int)((tiltVal > 0) - (tiltVal < 0)) * maxRange;
			}

			//because smoothVal is static, this calculation uses the prior smooth value in calculating the new smooth value
			smoothVal = smoothAlpha * smoothVal + (1 - smoothAlpha) * tiltVal;
			paddleX = paddleX - smoothVal;

			//prevent paddle from moving off left or right sides of screen
			if (paddleX < 0) {
				paddleX = 0;
			} else if (paddleX > screenW - paddleW) {
				paddleX = screenW - paddleW;
			}

		}

		//keep track of when this processed, so can wait the appropriate time before processing again
		lastMillis = millis();
	}
}

//update paddle position from joystick
void readPaddleJoystick() {
	static const unsigned long joystickDelayMillis = 7;	//milliseoncs delay beteen processing joystick reading (to also de-tune the joystick response)
	static unsigned long lastMillis = 0;

	if (millis() - lastMillis > joystickDelayMillis) {
		paddleX = map(Esplora.readJoystickX(), -512, 512, screenW - paddleW, 0);
		lastMillis = millis();
	}
}


//Setup a new paddle (called at start of game, after lost ball and between levels)
void newPaddle() {

	//erase the entire paddle area
	EsploraTFT.fill(0, 0, 0);
	EsploraTFT.rect(0, paddleY, screenW, paddleH);
	EsploraTFT.noStroke();

	lastPaddleX = -1;

}

bool checkModeButtons(void) {
	if (Esplora.readButton(SWITCH_LEFT) == LOW) {
		paddleMode = JOYSTICK;

		return true;
	}

	if (Esplora.readButton(SWITCH_UP) == LOW) {
		int tiltSum = 0;
		int tiltSamples = 20;
		paddleMode = TILT;
		delay(250);
		for (int x = 0; x < tiltSamples; x++) {
			tiltSum += Esplora.readAccelerometer(X_AXIS);
			delay(50);
		}
		tiltZeroOffset = tiltSum/tiltSamples;

		return true;
	}

	if (Esplora.readButton(SWITCH_DOWN) == LOW) {
		paddleMode = SLIDER;

		return true;
	}

	//if no/valid button hit, return false, indicating no selection made
	return false;
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


void showLabels() {
	EsploraTFT.stroke(0, 255, 0);
	EsploraTFT.noStroke();
}

void showMode() {
	EsploraTFT.fill(0, 0, 0);
	EsploraTFT.rect(modeX, statusY, 6*5, 7);
	EsploraTFT.stroke(0, 255, 0);
	EsploraTFT.text(modeStrings[paddleMode], modeX, statusY);
	EsploraTFT.noStroke();

}

void getMode() {
	EsploraTFT.stroke(0, 255, 0);
	EsploraTFT.textSize(2);
	EsploraTFT.text("Select Mode", 15, 5);
	EsploraTFT.text("1: Slider", 20, 25);
	EsploraTFT.text("2: Joystick", 20, 45);
	EsploraTFT.text("3: Tilt", 20, 65);
	EsploraTFT.textSize(1);
	EsploraTFT.noStroke();

	//wait for a mode button to be pressed
	while (!checkModeButtons()) {
	}

	paddleW = 20;

	newPaddle();
	delay(250);

	EsploraTFT.background(0, 0, 0);	//set the screen black
	EsploraTFT.stroke(0, 0, 0);

}

void newGame() {
	//clear the screen
	EsploraTFT.background(0, 0, 0);

	getMode();

	newScreen();
}

//setup screen for next ball
void newScreen(void) {
	newBall();
	newPaddle();
	readPaddle();			//routine draws the paddle on the screen
	drawPaddle();

	showLabels();
	showMode();

}

void newBall() {
	ballX = random(screenW - ballW);
	ballY = screenTopY;
	ballXComp = pow(-1, random(2));
	ballYComp = abs(ballYComp);
}
