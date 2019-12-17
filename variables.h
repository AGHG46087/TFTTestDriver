#ifndef MY_APP_VARS_H_
#define MY_APP_VARS_H_   242


//(p.x>60 && p.x<(60+360) && p.y>110 && p.y<(110+40))
#define IS_WITHIN_RANGE(x,a,b) ((x>=a) && (x<=b))

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define IS_POINT_IN_TARGET(p,startX,xPxWidth,startY,yPxHeight) ((startX <= p.x && p.x < (startX+xPxWidth)) && (startY <= p.y && p.y < (startY+yPxHeight)))
// Define some TFT readable colour codes to human readable names
#define MAX_PATTERN_NAME_LEN 30

#define BLACK   0x0000
#define BLUE    0x001F
#define BLUE1   0x077F
#define BLUE2   0x071F
#define BLUE3   0x065F
#define BLUE4   0x057F
#define BLUE5   0x04BF
#define BLUE6   0x03FF
#define BLUE7   0x033F
#define BLUE8   0x025F
#define BLUE9   0x019F
#define BLUE10  0x00DF
#define BLUE11  0x001F
#define RED     0xF800
#define WHITE   0xFFFF
#define GRAY    0x8410
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define LTGREEN 0x87F0

#define YP A1                     // A2, must be an analog pin, use "An" notation!
#define XM A2                     // A3, must be an analog pin, use "An" notation!
#define YM 7                      // 8, can be a digital pin
#define XP 6                      // 9, can be a digital pin

// https://platformio.org/lib/show/1426/MCUFRIEND_kbv/examples Diagnose Touchpins
// from Kuman_SC3A
#define TS_MINX 85
#define TS_MAXX 925
#define TS_MINY 135
#define TS_MAXY 910

#define MINPRESSURE 10
#define MAXPRESSURE 1000

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

bool firstScreenSaverUpdate = true;
unsigned char paletteIndex = 5;   // Default starting value
const int MAX_PALETTE_INDEX = 8;


enum { PG_HOME, PG_VU, PG_VU_2, PG_STANDBY, PG_STANDBY_2, PG_SETTING, PG_BRIGHTVOL, PG_RGBCONTROL, PG_CRED, PG_NITELITE, PG_PALLETTE, PG_SCREENSAVER, PG_SCREENSAVERTIME, PG_CYCLETIME, PG_RESTART};

#define TIMER 3000

unsigned char palletteIndex = 5; 
bool bTouchLast = false;
bool bTouch = false;
bool bNeedRedraw = true;       // Start by needing to redraw
int lastPage = PG_HOME;
int currentPage = PG_HOME;
bool updateDisplay = false;
int bluemin = 100;
int oldblue2 = 0;
int blue1 = 181;
int oldblue1 = 0;
int greenmin = 100;
int oldgreen1 = 0;
int green1 = 181;
int red1 = 181;
int oldred1 = 0;
int redmin = 100;
int bluemin2 = 255;
int blue2 = 420;
int greenmin2 = 255;
int oldgreen2 = 0;
int green2 = 420;
int red2 = 420;
int oldred2 = 0;
int redmin2 = 255;
int savePointX1 = 77; // saverPointX[77] => pageSaverSeconds[20]  
int savePointX2 = 125; // saverPointX[125] => meterCycleSeconds[30]  
int oldSavePointX = 0;
int pageSaverSeconds = 20;
int oldSaverSeconds = pageSaverSeconds;
int meterCycleSeconds = 30;
int oldMeterCycleSeconds = meterCycleSeconds;
int dampnervol7 = 7.5;
int dampnervol5 = 5;
int dampnervol3 = 30;
int dampnervol2 = 3;
int volPos = 280;
int oldVolPos = volPos;
unsigned char BRIGHTNESS = 64;
int brightnessPos = 126; // ==> BRIGHTNESS = 64
int oldBrightnessPos = 0;
long lastRun = millis();
uint8_t vuCurrentPatternNumber = 0; // Index number of which pattern is current
long currPageSaverRGB = BLACK;
int pageSaverXPos = random(5,265); // MIN = 5, MAX = 300
int pageSaverYPos = random(5,285); // MIN = 5, MAX = 295
int lastPageSaverXPos = pageSaverXPos;
int lastPageSaverYPos = pageSaverYPos;
int hueval = 0;
int buttonPressID = 0;        // counter for the number of button presses
long tempo;
char patternName[MAX_PATTERN_NAME_LEN + 1];
char wipePatternName[MAX_PATTERN_NAME_LEN + 1];

struct Detail {
   const char* title;
//   const __FlashStringHelper *title;
   int fontSize;
   int fontColor;
   int fontStartX;
   int fontStartY;
   int fontGap;
   int startX;
   int startY;
   int charRectWidth;
   int charRectHeight;
   int theGap;
   int delayTime;
};



#endif
