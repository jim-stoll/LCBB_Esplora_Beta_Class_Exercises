#include <Esplora.h>
#include <TFT.h>

#include "EsploraUtils.h"

//**COMMENT OUT the  next line, if working in the Arduino IDE
#include "BreakOut.h"

//** UNCOMMENT the next 2 lines, if working in the Arduino IDE
//enum paddleModeEnum {JOYSTICK, SLIDER, TILT} paddleMode = TILT;
//enum resultEnum {LOSS, WIN};

const char paddleModeStringJoystick[] = "Joystk";
const char paddleModeStringSlider[] =   "Slider";
const char paddleModeStringTilt[] =     "Tilt";
const char paddleModeStringAuto[] =     "Auto";

const char* modeStrings[] = {paddleModeStringJoystick, paddleModeStringSlider, paddleModeStringTilt, paddleModeStringAuto};

const char modeLbl[] = "";
const char scoreLbl[] = "Score:";
const char livesLbl[] = "x";
const char levelLbl[] = "Lvl:";
const char loseTxt[] = "GAME OVER";
const char winTxt[] = "YOU WIN!!!";

struct modeParamsStruct {
	unsigned long initialSpeedDelayMillis;		//initial 'speed' of ball (this is actually a millis delay between ball moves)
	int paddleW;								//paddle width in pixels
	int paddleSections;							//each *half* of the paddle is divided into this many sections, with the innermost section of each side being combined into one center section that is twice the width of the other sections. The center section imparts zero influence on the X travel of a ball. Each succeeding outer section imparts one (positive or negative, depending on ball direction) unit to the ball's horizontal direction, up to the max value of an outermost section.
	int speedIncreaseHitCount;					//number of ball/paddle hits between progressive speed increases
	unsigned long speedIncreaseIncrementMillis; //number of millis by which to increase ball speed at each progressive increase (actually decrease in ball delay)
	unsigned long maxSpeedDelayMillis;			//'max' delay millis (actually minimum...), which sets max speed for ball
	int perLevelPaddleShrinkPx;
	int scoreMultiplier;
};

struct modeParamsStruct modeParams[4];

const int screenW = EsploraTFT.width();		//convenience const for screen width
const int screenH = EsploraTFT.height();	//convenience const for screen height
const int screenTopY = 10;					//top of the play area of the screen (above this is the status bar info)

const int ballW = 4;					 	//ball size in px (ball is assumed to be square)
int ballX = 0;								//horizontal position on screen of top left corner of ball
int ballY = 0;								//vertical position on screen of top left corner of ball
int ballXComp = 1;							//X component of ball vector (negative = movement left, positive = movement right)
int ballYComp = 2;							//Y component of ball vector (negative = movement up the screen, positive = movement down the screen)
unsigned long ballProcessDelay = 25;					 //milliseconds between processing of ball - controls ball speed (set from mode params)

const int paddleH = 4;				//height in px of paddle
int paddleW = 20;				//paddle width in px - set from mode params, then shrinks at each level progression (if/as specified in mode params)
int paddleX = 0;				//horizontal position on screen of top left corner of paddle
const int paddleY = screenH - paddleH;		//vertical position on screen of top left corner of paddle
int lastPaddleX = 0;				//used to erase last paddle position, and determine if paddle has moved (for redraw)
int paddleDivisionW = 0;			//width in px of each division of the paddle, as determined by paddle width and paddle sections mode params, and dynamically as paddle shrinks at each level progression

const int numBricksW = 16;					//number of bricks on screen horizontally
const int maxBricksH = 16;					//max number of bricks on screen vertically - this is a physical limitation of how many bricks will fit on screen, and still allow paddle and ball movement below. (if exceeded (due to increased brick rows per screen, know that game is over)
const int initialNumBrcksH = 10;				//starting number of brick rows
int numBricksH = initialNumBrcksH;						//number of bricks on screen vertically, at present (grows at each level progression)
const int brickW = 10;						//width of bricks in pixels
const int brickH = 5;				 		//height of brick in pixels
const int marginW = (screenW - numBricksW * brickW)/2;	//space on sides of screen, between bricks and edge (if any) - auto calculated based on screenW and brickW
int numBricks = 0;							//tracks how many bricks are drawn for a level
int numBricksHit = 0;						//track how many bricks have been hit in current level (compare num to numHit, to determine when level is complete)
int score = 0;								//track score for game
int bricks[numBricksW][maxBricksH];			//2D array of bricks - tracks if each individual brick is active, and its point value, if so (0 = inactive, >0 = point value
int colBrickCount[numBricksW];				//track number of bricks remaining in each brick column

const int startLives = 3;					//number of lives that a game starts with
int lives = startLives;						//counter for number of lives left
int level = 0;								//counter for level number (creation of a new level increments this, so start at zero, as first level will immediately increment it to 1)

boolean speakerEnabled = false;
int ballHits = 0;							//keep track of number of times ball is hit - used to increase speed after each number of paddle hits (as specified in mode param)

//constants for text positioning of labels and values
// these are defined as global consts because they will be used at other times in the program
const int statusY = 2;						//all status bar text and values will be at the same Y position (at top of screen)
const int modeLblX = 2;
const int livesLblX = 41;
const int levelLblX = 63;
const int scoreLblX = screenW - 61;
const int loseTxtX = 30;
const int loseTxtY = 65;
const int winTxtX = 30;
const int winTxtY = 65;
const int belowBricksTextMarginH = 10;		//how far below current lowest level of bricks, the upper left corner of the countdown text should be placed
const int countdownTxtX = screenW/2 - 5;	//center countdown text (assuming countdown text font is 10 px wide)

//calculated constants for text positions of status values (these assume a font that is 6 px wide per character)
const int modeX = modeLblX + strlen(modeLbl) * 6 + 2;
const int scoreX = scoreLblX + strlen(scoreLbl) * 6 + 2;
const int levelX = levelLblX + strlen(levelLbl) * 6;
const int livesX = livesLblX + strlen(livesLbl) * 6 + 2;

int tiltZeroOffset = 0;							//level offset used to compensate for a non-level 'zero' position when tilt mode is selected

void setupModeParams() {
	modeParams[JOYSTICK].initialSpeedDelayMillis = 30;
	modeParams[JOYSTICK].paddleW = 25;
	modeParams[JOYSTICK].paddleSections = 3;
	modeParams[JOYSTICK].speedIncreaseHitCount = 3;
	modeParams[JOYSTICK].speedIncreaseIncrementMillis = 2;
	modeParams[JOYSTICK].maxSpeedDelayMillis = 24;
	modeParams[JOYSTICK].perLevelPaddleShrinkPx = 2;
	modeParams[JOYSTICK].scoreMultiplier = 2;
	modeParams[SLIDER].initialSpeedDelayMillis = 25;
	modeParams[SLIDER].paddleW = 20;
	modeParams[SLIDER].paddleSections = 3;
	modeParams[SLIDER].speedIncreaseHitCount = 2;
	modeParams[SLIDER].speedIncreaseIncrementMillis = 2;
	modeParams[SLIDER].maxSpeedDelayMillis = 16;
	modeParams[SLIDER].perLevelPaddleShrinkPx = 2;
	modeParams[SLIDER].scoreMultiplier = 1;
	modeParams[TILT].initialSpeedDelayMillis = 40;
	modeParams[TILT].paddleW = 30;
	modeParams[TILT].paddleSections = 2;
	modeParams[TILT].speedIncreaseHitCount = 2;
	modeParams[TILT].speedIncreaseIncrementMillis = 1;
	modeParams[TILT].maxSpeedDelayMillis = 30;
	modeParams[TILT].perLevelPaddleShrinkPx = 2;
	modeParams[TILT].scoreMultiplier = 1;
	modeParams[AUTO].initialSpeedDelayMillis = 15;
	modeParams[AUTO].paddleW = 20;
	modeParams[AUTO].paddleSections = 3;
	modeParams[AUTO].speedIncreaseHitCount = 2;
	modeParams[AUTO].speedIncreaseIncrementMillis = 1;
	modeParams[AUTO].maxSpeedDelayMillis = 2;
	modeParams[AUTO].perLevelPaddleShrinkPx = 2;
	modeParams[AUTO].scoreMultiplier = 1;

}

//the setup method of the program - runs just once, when power is first applied (or after reset)
void setup() {
//	Serial.begin(115200);

	//seed the random generator from as random a source as possible on the Esplora (no unused Analog inputs - the usual approach)
	randomSeed(getRandomSeed());

	EsploraTFT.begin();

	setupModeParams();

	newGame();
}

//the main loop of the program - executes continuously, as long as the device is powered
void loop() {
	readPaddle();
	drawPaddle();

	checkSpeakerEnableButton();

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

		EsploraTFT.stroke(192, 192, 192);							//set stroke color to grey (to draw paddle section dividers)

		//for each paddle section, draw a single-pixel-wide vertical line at its outermost horizontal position
		for (int x = paddleDivisionW/2; x < paddleW/2; x = x + paddleDivisionW) {
			EsploraTFT.line(paddleX + paddleW/2 + x, paddleY, paddleX + paddleW/2 + x, paddleY + paddleH);			//line on right half of paddle
			EsploraTFT.line(paddleX + paddleW/2 - x - 1, paddleY, paddleX + paddleW/2 - x - 1, paddleY + paddleH);	//line on left half of paddle
		}

		//turn off stroke, so that next item drawn doesn't have an outline
		EsploraTFT.noStroke();

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

		case AUTO:
			readPaddleAuto();
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

//determine brick column from ball position (used by autoplay mode)
//- ball can possibly span 2 columns (positioned in space 'between' columns), so alters both
//passed pointer values. If only in one column, both values are the same.
//- if ball is in a margin column along the edges (depending on brick and field width), then return -1 for both values
void mapBallToCol(int* col1, int* col2) {
	//make sure the ball is actually within the width of the bricks
	if (ballX >= brickW && ballX <= brickW*numBricksW + brickW - 1) {
		//determine first column
		*col1 = ballX/brickW - 1;

		//if ball spans into another column, set value for 2nd column
		if (ballX >= *col1 * brickW + brickW + ballW + 1) {
			*col2 = *col1 + 1;

			//if 2nd col is outside of width of bricks, set 2nd value to first value (ie, doesn't actually span)
			if (*col2 > numBricksW - 1) {
				*col2 = *col1;
			}
		//if doesn't span columns, return same value for both pointer values
		} else {
			*col2 = *col1;
		}
	//if outside of width of bricks, return -1 for both pointer values
	} else {
		*col1 = -1;
		*col2 = -1;
	}
}

//update paddle position when in auto mode
void readPaddleAuto() {
	int col1;
	int col2;
	mapBallToCol(&col1, &col2);

	//keep the paddle under the ball
	paddleX = ballX - paddleW/2 + ballW/2;

	//make adjustments to avoid missing ball at edges, and to introduce some randomness to the hits (vs paddle always centered under ball)

	//if ball is moving downward, and ballY < paddleY (ie, above paddle), and ball is within a ball width of the paddle (ie, about to hit paddle)
	if (ballYComp > 0 && (ballY < paddleY && ballY > paddleY - ballYComp - ballW - 1)) {

		//if ball is along far left edge, be sure paddle is all the way over to the left
		if (ballX <= ballW/2) {
			paddleX = 0;
		//if ball is along far right edge, be sure paddle is all the way over to the right (right edge of paddle on right edge of screen)
		} else if (ballX >= screenW - ballW/2) {
			paddleX = screenW - paddleW;
		} else {

			//get the brick column that the ball is in
			// (ball can span 2 columns, so returns 2 column numbers - same value if not spanning column, negative value if in left or right margin area)
			mapBallToCol(&col1, &col2);

			//if in one of the end columns, move the paddle one division inward, so that it'll cause a bounce off of that side wall
			if (col1 < 0) {
				if (paddleX < screenW/2) {
					// 1 paddle division from left edge
					paddleX = paddleDivisionW;//paddleW/3;
				} else {
					// 1 paddle division from right edge
					paddleX = screenW - paddleW - paddleDivisionW;
				}
			//otherwise, ball is in general play
			} else {
				//if in an empty playable column, force paddle 1 paddle division left or right, to avoid sending straight back up an empty column repeatedly
				if (colBrickCount[col1] == 0 && colBrickCount[col2] == 0) {
					paddleX += pow(-1, random(2)) * paddleDivisionW;		//map random value of 0 or 1 to -1 or +1
				//if not an empty column, then randomly offset one section width left or right, or send straight back up
				} else {
					paddleX += map(random(3), 0, 2, -1, 1) * paddleDivisionW;
				}
			}
		}
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
	int effectivePaddleW = 0;

	//erase the entire paddle area
	EsploraTFT.fill(0, 0, 0);
	EsploraTFT.rect(0, paddleY, screenW, paddleH);
	EsploraTFT.noStroke();

	effectivePaddleW = paddleW/2 + ballW - 1;

	paddleDivisionW = effectivePaddleW/modeParams[paddleMode].paddleSections + 1;

	lastPaddleX = -1;

}

void checkSpeakerEnableButton() {
	if (Esplora.readButton(SWITCH_RIGHT) == LOW) {
		speakerEnabled = !speakerEnabled;
		delay(250);
	}
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

		//wait a moment for any movement associated w/ button press to stablize
		delay(250);

		//take a series of readings of the tilt, then average them, and assume this is the 'level' position
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

	if (Esplora.readButton(SWITCH_RIGHT) == LOW) {
		paddleMode = AUTO;

		return true;
	}

	//if no/valid button hit, return false, indicating no selection made
	return false;
}

void gameEnd(enum resultEnum result) {
	if (result == LOSS) {
		if (speakerEnabled) {
			Esplora.tone(130, 1000);
		}
		for (int i = 0; i < 75; i++) {
			EsploraTFT.stroke(255, 0, 0);
			EsploraTFT.setTextSize(2);
			EsploraTFT.text(loseTxt, loseTxtX, numBricksH * brickH + screenTopY + belowBricksTextMarginH);
			EsploraTFT.stroke(0, 0, 255);
			EsploraTFT.text(loseTxt, loseTxtX, numBricksH * brickH + screenTopY + belowBricksTextMarginH);
		}
	} else {
		EsploraTFT.fill(0, 0, 0);
		EsploraTFT.rect(ballX, ballY, ballW, ballW);
		if (speakerEnabled) {
			Esplora.tone(400, 250);
			delay(250);
			Esplora.tone(415, 250);
			delay(250);
			Esplora.tone(430, 500);
		}
		for (int  i= 0; i < 10; i++) {
			EsploraTFT.stroke(255, 0, 0);
			EsploraTFT.setTextSize(2);
			EsploraTFT.text(winTxt, winTxtX, numBricksH * brickH + screenTopY + belowBricksTextMarginH);
			EsploraTFT.stroke(0, 255, 0);
			EsploraTFT.text(winTxt, winTxtX, numBricksH * brickH + screenTopY + belowBricksTextMarginH);
			delay(10);
		}
	}

	EsploraTFT.stroke(0, 0, 0);
	delay(1000);
	newGame();
}

void processBall(void) {
	static unsigned long lastProcessMillis = millis();
	static int lastBallX = 0;							//prior X position of ball - used for erasing ball when it moves to a new position
	static int lastBallY = screenH - paddleH - ballW;	//prior Y position of ball - used for erasing ball when it moves to a new position (initialize to bottom row, so that redraw of 'last' position on startup is not on top of anything visible (bricks, status bar, etc)

	int oldHits = numBricksHit;

	if (millis() - lastProcessMillis > ballProcessDelay) {
		lastProcessMillis = millis();

		//check if the ball hits the side walls
		if ((ballX < ballXComp * -1) || (ballX > screenW - ballXComp - ballW)) {
			ballXComp = -ballXComp;
			if (speakerEnabled) {
				Esplora.tone(230, 10);
			}
		}

		//check if the ball hits the top of the screen
		if (ballY <= screenTopY) {
			//at top of screen ball direction will always be postive (ran into bug trying to just complement the direction)
			ballYComp = abs(ballYComp);
			if (speakerEnabled) {
				Esplora.tone(530, 10);
			}
		}

		//check if ball hits bottom of bricks
		//we run through the array, if the brick is active, check its position against the balls position
		for (int a = 0; a < numBricksW; a++) {
			for (int b = 0; b < numBricksH; b++) {
				if (bricks[a][b] > 0) {
					if (ballX > a * brickW + marginW - ballW
							&& ballX < a * brickW + brickW + marginW
							&& ballY > b * brickH + screenTopY - ballW
							&& ballY < b * brickH + brickH + screenTopY) {
						//we determined that a brick was hit
						EsploraTFT.fill(0, 0, 0);				//erase the brick
						EsploraTFT.rect(a * brickW + marginW, b * brickH + screenTopY, brickW, brickH);
						ballYComp = -ballYComp;					//change ball direction
						numBricksHit = numBricksHit+1;				 //add to the bricks hit count
						score = score + bricks[a][b];
						bricks[a][b] = 0;							 //set the brick inactive in the array
						if (speakerEnabled) {
							Esplora.tone(330, 10);
						}
						if (colBrickCount[a] > 0) {
							colBrickCount[a]--;				//reduce count of bricks in this column
						}
					}
				}
			}
		}

		if (oldHits != numBricksHit) {
			showScore();
		}

		//check if the ball hits the paddle
		if (ballX > paddleX - ballXComp - ballW
				&& ballX < paddleX + paddleW + ballXComp*-1
				&& ballY > paddleY - ballYComp - ballW
				&& ballY < paddleY) {

			int ballOnPaddleX = paddleX + paddleW/2 - ballW/2 - ballX;
			int paddleSection = ballOnPaddleX/paddleDivisionW;

			ballXComp = ballXComp - paddleSection;

			//don't allow the ball to exceed the max pos or neg direction of the outermost section of the paddle
			if (ballXComp < -1 * paddleW/2 / paddleDivisionW) {
				ballXComp = -1 * paddleW/2 / paddleDivisionW;
			} else if (ballXComp > paddleW/2 / paddleDivisionW) {
				ballXComp = paddleW/2 / paddleDivisionW;
			}

			ballYComp = -ballYComp;				 //change direction up/down

			if (speakerEnabled == HIGH) {
				Esplora.tone(730, 10);
			}

			ballHits++;

			if (ballHits % modeParams[paddleMode].speedIncreaseHitCount == 0) {
				ballProcessDelay = ballProcessDelay - modeParams[paddleMode].speedIncreaseIncrementMillis;
				if (ballProcessDelay < modeParams[paddleMode].maxSpeedDelayMillis) {
					ballProcessDelay = modeParams[paddleMode].maxSpeedDelayMillis;
				}
			}
		}

		//check if the ball went past the paddle
		if (ballY >= paddleY + paddleH) {
			if (lives > 1) {
				lives--;
				delay(1000);
				newScreen();
			} else {
				lives--;
				showLives();
				gameEnd(LOSS);
			}
		}

		//check if we're out of bricks
		if (numBricksHit == numBricks) {
			//erase the old ball
			EsploraTFT.fill(0, 0, 0);
			EsploraTFT.rect(lastBallX, lastBallY, ballW, ballW);

			//set the last position to off screen, so that subsequent attempt to erase it won't leave artifact on screen (ratty bug fix...)
			lastBallX = screenW;
			lastBallY = screenH;

			numBricksH = numBricksH + 2;

			if (numBricksH <= maxBricksH) {
				delay(1000);
				newLevel();
			} else {
				gameEnd(WIN);
			}
		} else {

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
}

void showLabels() {
	EsploraTFT.stroke(0, 255, 0);
	EsploraTFT.text(modeLbl, modeLblX, statusY);
	EsploraTFT.text(livesLbl, livesLblX, statusY);
	EsploraTFT.text(levelLbl, levelLblX, statusY);
	EsploraTFT.text(scoreLbl, scoreLblX, statusY);
	EsploraTFT.noStroke();

}

void showMode() {
	EsploraTFT.fill(0, 0, 0);
	EsploraTFT.rect(modeX, statusY, 6*5, 7);
	EsploraTFT.stroke(0, 255, 0);
	EsploraTFT.text(modeStrings[paddleMode], modeX, statusY);
	EsploraTFT.noStroke();

}

void showLives() {
	char sLives[2];
	EsploraTFT.fill(0, 0, 0);
	EsploraTFT.rect(livesX - 1, statusY, 2*5 - 2, 7);
	EsploraTFT.stroke(0, 255, 0);
	itoa(lives, sLives, 10);
	EsploraTFT.text(sLives, livesX, statusY);
	EsploraTFT.noStroke();

}

void showLevel() {
	char sLevel[2];
	EsploraTFT.fill(0, 0, 0);
	EsploraTFT.rect(levelX - 1, statusY, 2*5, 7);
	EsploraTFT.stroke(0, 255, 0);
	itoa(level, sLevel, 10);
	EsploraTFT.text(sLevel, levelX, statusY);
	EsploraTFT.noStroke();

}

void showScore() {
	char sScore[5];
	EsploraTFT.fill(0, 0, 0);
	EsploraTFT.rect(scoreX - 1, statusY, screenW - (scoreX - 1), 7);
	EsploraTFT.stroke(0, 255, 0);
	itoa(score * modeParams[paddleMode].scoreMultiplier, sScore, 10);
	EsploraTFT.text(sScore, scoreLblX + 5*7 + 2, statusY);
	EsploraTFT.noStroke();

}

void getMode() {
	EsploraTFT.stroke(0, 255, 0);
	EsploraTFT.textSize(2);
	EsploraTFT.text("Select Mode", 15, 5);
	EsploraTFT.text("1: Slider", 20, 25);
	EsploraTFT.text("2: Joystick", 20, 45);
	EsploraTFT.text("3: Tilt", 20, 65);
	EsploraTFT.text("4: Auto", 20, 85);
	EsploraTFT.textSize(1);
	EsploraTFT.stroke(0, 255, 255);
	EsploraTFT.text("Press 4 during play to", 15, 105);
	EsploraTFT.text("toggle speaker", 40, 115);
	EsploraTFT.noStroke();

	//wait for a mode button to be pressed
	while (!checkModeButtons()) {
	}

	paddleW = modeParams[paddleMode].paddleW;

	newPaddle();
	delay(250);

	EsploraTFT.background(0, 0, 0);	//set the screen black
	EsploraTFT.stroke(0, 0, 0);

}

//delay, but allow processing of paddle during delay (right before ball released)
void delayWithPaddle(unsigned long delayMillis) {
	unsigned long startMillis = millis();

	while (millis() - startMillis < delayMillis) {
		readPaddle();
		drawPaddle();
	}
}

void showCountdown() {
	char secsBuff[2];

	for (int secs = 3; secs > 0; secs--) {
		itoa(secs, secsBuff, 10);

		EsploraTFT.stroke(0, 0, 255);
		EsploraTFT.textSize(2);
		EsploraTFT.text(secsBuff, countdownTxtX, numBricksH * brickH + screenTopY + belowBricksTextMarginH);
		EsploraTFT.noStroke();
		delayWithPaddle(1000);
		EsploraTFT.noStroke();
		EsploraTFT.fill(0, 0, 0);
		EsploraTFT.rect(countdownTxtX, numBricksH * brickH + screenTopY + belowBricksTextMarginH, 15, 15);
	}

	EsploraTFT.setTextSize(1);

}

void newGame() {
	//clear the screen
	EsploraTFT.background(0, 0, 0);

	getMode();
	numBricksH = initialNumBrcksH;
	score = 0;
	level = 0;
	lives = startLives;

	newLevel();

}

void newLevel() {
	level++;
	numBricksHit = 0;

	setupBricks();

	paddleW = paddleW - (level - 1) * modeParams[paddleMode].perLevelPaddleShrinkPx;

	//always make sure there is enough room to render all paddle sections, at least 3 px wide (2px for display, 1px for dividing line)
	if (paddleW < modeParams[paddleMode].paddleSections * 3) {
		paddleW = modeParams[paddleMode].paddleSections * 3;
	}

	//newLevel is called on new game, as well as at, well, level change, and a new screen is needed at that time, so just calling newScreen from here
	newScreen();

}

//setup screen for next ball
void newScreen(void) {
	newBall();
	newPaddle();
	readPaddle();
	drawPaddle();
	ballHits = 0;

	showLabels();
	showMode();
	showLives();
	showLevel();
	showScore();
	showCountdown();

}

void newBall() {
	ballX = random(screenW - ballW);
	ballY = screenTopY + numBricksH * brickH + screenTopY;
	ballXComp = map(random(3), 0, 2, -1, 1);
	ballYComp = abs(ballYComp);
	ballProcessDelay = modeParams[paddleMode].initialSpeedDelayMillis;
}

void setupBricks(void) {
	//assign the individual bricks to active in an array
	numBricks = 0;
	numBricksHit = 0;

	//array of BGR colors for bricks - items at top of array (ie, brickColors[col][0] will be at bottom of display)
	// so that purple is always bottom 2 rows, blue is always next up, etc, with new pairs of rows added at each new level
	//maxBricksH/2 assumes 2 rows of each color in brick display
	const unsigned char brickColors[maxBricksH/2][3] = {
			{255, 51, 153},		//purple
			{255, 51, 51},		//blue
			{255, 204, 229},	//lavender
			{0, 255, 0},		//green
			{0, 255, 255},		//yellow
			{225, 0, 255},		//pink
			{0, 100, 255},		//orange
			{0, 0, 255}			//red
	};

	//clear out the column brick count array - used for auto mode
	for (int x = 0; x < numBricksW; x++) {
		colBrickCount[x] = 0;
	}

	EsploraTFT.stroke(0, 0, 0);

	for (int a = 0; a < numBricksW; a++) {
		for (int b = 0; b < maxBricksH; b++) {
			if (b < numBricksH) {
				int i = numBricksH/2 - (b/2);
				bricks[a][b] = i;
				EsploraTFT.fill(brickColors[i-1][0], brickColors[i-1][1], brickColors[i-1][2]);
				EsploraTFT.rect(a * brickW + marginW, b*brickH + screenTopY, brickW, brickH);
				numBricks += 1;
			} else {
				bricks[a][b] = 0;
			}
		}
		colBrickCount[a] = numBricksH;		//update the auto mode column brick count
	}

}
