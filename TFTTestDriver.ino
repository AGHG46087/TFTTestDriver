#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>

#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

#include "variables.h"

// Function to check a map conversion values,  when deciding on targets
void testAndPrintMapRange(int fromLow, int fromHigh, int toLow, int toHigh) {
  int temp = 0;
  for ( int i = fromLow; i < fromHigh; i++ ) {
    temp = map(i, fromLow, fromHigh, toLow, toHigh );
    Serial.print(F("point=["));Serial.print(i);Serial.print(F("[, value=["));Serial.print(temp);Serial.println(F("]"));
  }
  
}
// This is my init function
void setup() {
    // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.print(F("Finding out the type of TFT LCD... ID is : "));
  tft.reset();
  uint16_t identifier = tft.readID();
  Serial.print(identifier, HEX);
  tft.begin(identifier);
  Serial.print(". TFT size is : "); Serial.print(tft.width()); Serial.print(" x "); Serial.println(tft.height());
  tft.setRotation(1);
  tft.invertDisplay(1);             // UNCOMMENT THIS IF COLORS get inverted

  myBootloader();

  bTouchLast = bTouch = false;
  
  savePointX1 = 77; // saverPointX[77] => pageSaverSeconds[20]  
  savePointX2 = 125; // saverPointX[125] => meterCycleSeconds[30]  
  oldSavePointX = 0;

  oldSaverSeconds = pageSaverSeconds  = 20;
  oldMeterCycleSeconds = meterCycleSeconds = 30;
  
}
void myBootloader() {
  Serial.println("MyBootLoader...");
  
  tft.fillScreen(BLACK);
  
  Serial.println("Drawing Init Screen...");
  StartupScreen();
  delay(700);

  
  currentPage = lastPage = currentPage = PG_HOME;
//  buttonState = lastButtonState = buttonPressID = 0;        // counter for the number of button presses
  buttonPressID = 0;
  bNeedRedraw = true; 
  updateDisplay = false;
  
}

void loop() {

  if (currentPage != PG_SCREENSAVER) {
    tempo = millis();
    if(tempo%20000==0){
      lastPage = currentPage;
      currentPage = PG_SCREENSAVER;
      bNeedRedraw = true; 
    }
  }

  setPatternName((char*)PSTR(" "));
    // Draw current screen
    // - But only do it if we need to redraw (eg. changed screen or first presentation)

  if (bNeedRedraw) {
    bNeedRedraw = false;
    switch(currentPage) {
      case PG_HOME:
        drawHomeScreen();
        break;
      case PG_VU:
        drawPageVU();
        break;
      case PG_VU_2:
        drawPageVU2();
        break;
      case PG_STANDBY:
        drawPageStandby();
        break;
      case PG_STANDBY_2:
        drawPageStandby2();
        break;
      case PG_SETTING:
        drawPageSettings();
        break;
      case PG_BRIGHTVOL:
        drawPageBriteVol();
        break;
      case PG_RGBCONTROL:
        drawPageRGBControl();
        break;
      case PG_PALLETTE:
        drawPagePalettePlusMinus();
        break;
      case PG_SCREENSAVER:
        drawPageSaver();
        bNeedRedraw = true; // We want this one to refresh every so often. set to redraw
        break;
      case PG_SCREENSAVERTIME:
        drawPageSaverStartTime();
        break;
      case PG_CYCLETIME:
        drawPageCycleTime();
        break;
      case PG_RESTART:
        myBootloader(); // Lets Reset Everything, user decided.
        break;
      default:
        break;
    }
  }
    // GEEK: It all starts here

  TSPoint p = getTouchStatus();

    // Touch handling specific to each page
    // - Look for touch releases only
  if (bTouch && !bTouchLast) {
    switch(currentPage) {
        // ----------------------
      case PG_HOME:
        handleHomeScreenEvents(p);
        break;
      case PG_VU:
        handleVUMeterEvents(p);
        break;
      case PG_VU_2:
        handleVUMeterPage2Events(p);
        break;
      case PG_STANDBY:
        handleStandbyEvents(p);
        break;
      case PG_STANDBY_2:
        handleStandbyPage2Events(p);
        break;
      case PG_SETTING:
        handleSettingsPageEvents(p);
        break;
      case PG_BRIGHTVOL:
        handleBriteVolPageEvents(p);
        break;
      case PG_RGBCONTROL:
        handlePageRGBVControl(p);
        break;
      case PG_PALLETTE:
        handlePaletteEvents(p);
        break;
      case PG_SCREENSAVER:
        handlePageSaverEvents(p);
        break;
      case PG_CYCLETIME:
        handleCycleTimeEvents(p);
        break;
      case PG_SCREENSAVERTIME:
        handlePageSaverSartTimeEvents(p);
        break;
      case PG_RESTART:
          // Do nothing here,  the draw page should have involed all required steps
        break;
      default:
        break;
    } // switch
  } // if touch

}

/* GEEK Set Pattern Name
 * sePatternName(); this is going to be wierd to listen up.
 * The plan here is to place string liternals into flash memory to save space. and is IMMUTABLE
 * To do that all string liternal must take the following form:
 *        setPatternName((char*)PSTR("Your string here"));
 * The reason for this is <pgmspace.h> there is the macro definition of PSTR which will place the literal in flash
 * in flash memory. BUT. we need to now cast that to a const char* for the function call.
 * With that said, the patternname, which we normaly just use strcpy, we need to use strcpy_P.
 * strcpy_P states the string will come from Flashmemory and copy it into SRAM space. 
 * Once it is in SRAM sapce we can maniputate it all we want.
 */
void setPatternName(const char* text) {
  memset(patternName, MAX_PATTERN_NAME_LEN + 1, 0 );
  strcpy_P(patternName, text);
  int length = strlen(patternName);
  Serial.print(F("patternName =["));Serial.print(patternName);Serial.print(F("], length = ["));Serial.print(length);Serial.println(F("] "));
}

TSPoint getTouchStatus() {
    // Fetch touch status
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

/*  Stupid Rabbit,  the trix are for kids,  the board maps things really weird.
 *  May be due to tft.setRotation() and how TSPoint believes it is.
 *  On draw planes Landscape painting, Cartesian plane IV with an ABS(Y-Axis) ...
 *  however on point plane. it is a different story: A 90 degree turn AND appears to be Cartesion plane III.
 *  Not sure how this happens, but need to deal with it.
 *  The coordinate system for what would be the point coordinate for X, is really the point coordinate Y in a mirror image
 *       RESOLVE: Map Y coordinate and reverse direction to a temp Variable for X then reassign to the point X coordinate.
 *  The coordinate system for what would be the point coordinate for Y, is really the point coordinate X
 *       RESOLVE Map X coordinate and to a temp Varibale for Y then reassign to the Point Y corrdinate.
 *  Lets map the y point into a TempX and reverse the direction, Solve Rotation and Mirroring of X values
 *  Lets map the x point into a TempY, direction is ok - Solves rotation.
 *  Once we have the temp values, reassign them to the proper point values.
 */
  int16_t tempX = map( p.y, TS_MINY, TS_MAXY, 480, 0 );
  int16_t tempY = map( p.x, TS_MINX, TS_MAXX, 0, 320 );
  p.x = tempX;
  p.y = tempY;


    /******  Something is really wierd here
             p.x = map( p.x, TS_MINX, TS_MAXX,0,  480 );
             p.y = map( p.y, TS_MINY, TS_MAXY, 0, 320 );
    *******/
    // Detect current touch state
  bTouchLast = bTouch;
  bTouch = IS_WITHIN_RANGE(p.z, MINPRESSURE, MAXPRESSURE );
    // GEEK: REMOVE THIS later
  if( bTouch ) {
//GEEK: TOUCH    Serial.print("Touch Detected: x=[");Serial.print(p.x); Serial.print("], y=[");Serial.print(p.y);Serial.print("], Pressure=[");Serial.print(p.z);Serial.println("].");
  }
  return p;
}

// Draw Page Title - display helper function
void drawPageTitle( struct Detail *details ) {
  const int blueArray1[] = { BLUE1, BLUE1, BLUE2, BLUE3, BLUE4, BLUE5, BLUE6, BLUE7, BLUE8, BLUE9, BLUE10, BLUE11, BLUE11 };
  const int blueArray2[] = { BLUE1, BLUE2, BLUE3, BLUE4, BLUE5, BLUE6, BLUE7, BLUE8, BLUE9, BLUE10, BLUE11 };
    // ARRAY_SIZE
  const int arrSize1 = ARRAY_SIZE(blueArray1);
  const int arrSize2 = ARRAY_SIZE(blueArray2);

/* title from details.title;  Ok this is going to be wierd to listen up.
 * The plan here is to place string liternals into flash memory to save space. and is IMMUTABLE
 * To do that all string liternal must take the following form:
 *        details.title = (char*)PSTR("Your string here");
 * The reason for this is <pgmspace.h> there is the definition of PSTR which will place the literal
 * in flash memory. BUT. we need to now cast that to a const char* for the function call.
 * With that, the `title`, which we normaly just use strcpy, we need to use strcpy_P.
 * strcpy_P states the string will come from Flashmemory and copy it into SRAM space.
 * Once it is in SRAM sapce we can maniputate it all we want.
 * More understanding can be found at this forum:
 * https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
 * http://arduinoetcetera.blogspot.com/2013/07/using-program-memory-progmem.html
 * https://forum.arduino.cc/index.php?topic=91314.0
 * https://www.baldengineer.com/arduino-f-macro.html
 */
    // Declare an MAX_LEN for a char array, the size of the largest color array + 10.  just in case something snaeky comes across.
  int MAX_LEN = arrSize1 + 10;
    //const char* title = (*details).title;
  char title[MAX_LEN];
  memset(title,MAX_LEN, 0 );
  strcpy_P(title,((*details).title));

    // const char* title = (*details).title; // NERD: B4 Flash mem copy of string.  Old way of doing it.
  int length = strlen(title);
  int blueArray[length];
  int startX = (*details).startX;
  int startY = (*details).startY;
  int charRectWidth = (*details).charRectWidth;
  int charRectHeight = (*details).charRectHeight;
  int theGap = (*details).theGap;

    /* // Example of Blue Array print in HEX
       for ( int x = 0; x < length; x++ ) {
       Serial.print(blueArray1[x], HEX);Serial.print(" = ");Serial.println(blueArray[x],HEX);
       }
    */
  if ( length == arrSize1 ) {
    memcpy(blueArray, blueArray1, sizeof(blueArray1));
  } else if (length == arrSize2 ) {
    memcpy(blueArray, blueArray2, sizeof(blueArray2));
  } else {
    Serial.print(F("ooops,  Wrong size to deal with: title = ["));Serial.print(title);Serial.print(F("] length = "));Serial.println(length);
    return;
  }

    // Page Border Here
  tft.fillScreen(BLACK);
  tft.drawRoundRect(0, 0, 479, 319, 8, BLUE);     //Page border


    // Work the boxes first
  int fontCompWidth = ((*details).fontSize < 3) ? 5 : 0;
  int rectWidth = (charRectWidth * length) + (theGap * length) + fontCompWidth;

  tft.drawRect(startX-5, startY-5, rectWidth, charRectHeight + 10, WHITE);
  for( int x = 0; x < length; x++ ) {
    tft.fillRect(startX, startY, charRectWidth, charRectHeight, blueArray[x]);
    startX += (charRectWidth + theGap);
    delay((*details).delayTime);
  }

    // Reset working variables for the Text.
  startX = (*details).fontStartX;
  startY = (*details).fontStartY;
  theGap = (*details).fontGap;

  tft.setTextSize((*details).fontSize);
  tft.setTextColor((*details).fontColor);

    // Draw Title on top of Blue Boxes
  for( int x = 0; x < length + 1; x++) {
      /*  Horizontal, Vertical  */
    tft.setCursor(startX, startY);
    tft.print(title[x]);
    delay((*details).delayTime);
    startX += theGap;
  }


}



/********* 1. Startup Screen **************/
void StartupScreen() {
  Serial.println(F("DP: Startup Screen"));

#define wait1 70 //  50
#define wait2 200

  tft.drawRoundRect(0, 0, 480, 320, 8, WHITE);    // NERD: Page Border special for startup screen only

  struct Detail details;
  details.fontSize = 3;            // Font Size
  details.fontColor = WHITE;       // Font color
  details.fontStartX = 32;         // X pos from left where characters start
  details.fontStartY = 120;        // Y Pos down from top where characters start
  details.fontGap = 40;            // Gap between characters
  details.startX = 25;             // Start Position from the left to right.
  details.startY = 90;             // Start Position from the top down.
  details.charRectWidth = 30;      // Width of the background box behind characters
  details.charRectHeight = 80;     // Height of the background box behind characters
  details.theGap = 10;             // The spacing between background boxes and added to text spacing
  details.delayTime = wait1;       // The delay of draw between items
  details.title = (char*)PSTR("KEWL-STUSPH"); //"KEWL-STUSPH";   // Text
    //                         12345678901

  drawPageTitle(&details);

  delay(700);

  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(200, 240);
  tft.print(F("LOADING..."));

  for ( int i = 1; i < 420; i+=2)
  {
    tft.fillRect(30, 200, i, 15, BLUE2);
    delay(1);
  }
  tft.fillRect(195, 240, 130, 15, BLACK);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(210, 240);
  tft.print(F("LOADED"));
}
/********* 2. Home Screen *****************/
void drawHomeScreen() {
  Serial.println(F("DP: Home Screen"));

  struct Detail details;
  details.fontSize = 2;            // Font Size
  details.fontColor = BLACK;       // Font color
  details.fontStartX = 110;        // X pos from left where characters start
  details.fontStartY = 45;         // Y Pos down from top where characters start
  details.fontGap = 25;            // Gap between characters
  details.startX = 105;            // Start Position from the left to right.
  details.startY = 30;             // Start Position from the top down.
  details.charRectWidth = 20;      // Width of the background box behind characters
  details.charRectHeight = 45;     // Height of the background box behind characters
  details.theGap = 5;              // The spacing between background boxes and added to text spacing
  details.delayTime = 5;           // The delay of draw between items
  details.title = (char*)PSTR("KEWL-STUSPH");//"KEWL-STUSPH";
    //                         12345678901

    //BANNER

  drawPageTitle(&details);

  tft.fillRoundRect(60, 110, 360, 40, 8, BLUE8);
  tft.drawRoundRect(60, 110, 360, 40, 8, WHITE);

  tft.fillRoundRect(60, 170, 360, 40, 8, BLUE8);
  tft.drawRoundRect(60, 170, 360, 40, 8, WHITE);

  tft.fillRoundRect(60, 230, 360, 40, 8, BLUE8);
  tft.drawRoundRect(60, 230, 360, 40, 8, WHITE);
  tft.setTextColor(WHITE);

  tft.setCursor(185, 125);
  tft.print(F("VU METERS"));

  tft.setCursor(150, 185);
  tft.print(F("STANDBY PATTERNS"));

  tft.setCursor(195, 243);
  tft.print(F("SETTINGS"));

  tft.setTextSize(1);
  tft.setCursor(160, 305);
  tft.setTextColor(WHITE);
  tft.print(F("Select one of the options..."));

}
/********* 3. Brite & Vol Screen **********/
void drawPageBriteVol() {
  Serial.println(F("DP: BriteVol"));
  int lcv = 0;
  if(updateDisplay){
      //over,down ,length,height
    tft.drawRoundRect(30, 110, 420, 30, 8, BLUE);
    tft.fillRoundRect(oldBrightnessPos, 113, 20, 25, 8, BLACK);
    tft.fillRoundRect(brightnessPos, 113, 20, 25, 8, WHITE);

    tft.drawRoundRect(30, 195, 420, 30, 8, BLUE);
    tft.fillRoundRect(oldVolPos, 198, 20, 25, 8, BLACK);
    tft.fillRoundRect(volPos, 198, 20, 25, 8, YELLOW);

    tft.fillRect(105, 235, 40, 20, BLACK);    // L2 vol
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(110, 237);
    tft.print(dampnervol2);

    tft.fillRect(205, 235, 40, 20, BLACK);    // L3 Vol
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(210, 237);
    tft.print(dampnervol3);

    tft.fillRect(305, 235, 40, 20, BLACK);    // L5 Vol
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(310, 237);
    tft.print(dampnervol5);

    tft.fillRect(405, 235, 40, 20, BLACK);    // L7 Vol
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(410, 237);
    tft.print(dampnervol7);

    updateDisplay = false;
    return;
  }
    //if (updateDisplay == false) {

  struct Detail details;
  details.fontSize = 2;
  details.fontColor = WHITE;
  details.fontStartX = 85;
  details.fontStartY = 30;
  details.fontGap = 25;
  details.startX = 80;
  details.startY = 20;
  details.charRectWidth = 20;
  details.charRectHeight = 35;
  details.theGap = 5;
  details.delayTime = 5;
  details.title = (char*)PSTR("DISPLAY  MENU");//"DISPLAY  MENU";
    //                         1234567890123

  drawPageTitle(&details);


  tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
  tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(395, 278);
  tft.print(F("<<"));

  tft.drawRect(75, 15, 330, 45, WHITE);

    //Testing Stuff
  tft.setTextColor(WHITE);
  tft.setCursor(180, 84);
  tft.setTextSize(2);
  tft.print(F("BRIGHTNESS: "));
  tft.setTextColor(CYAN);
  tft.setCursor(320, 84);
  tft.setTextSize(2);
  tft.print(BRIGHTNESS);


  tft.setTextColor(WHITE);
  tft.setCursor(135, 170);
  tft.setTextSize(2);
  tft.print(F("VOL DAMPNERS:"));
  tft.setTextColor(CYAN);
  tft.setCursor(300, 170);
  tft.setTextSize(2);
  tft.print(F("VU Meters"));

  tft.drawRoundRect(30, 110, 420, 30, 8, BLUE);
  tft.fillRoundRect(brightnessPos, 113, 20, 25, 8, WHITE);

  tft.drawRoundRect(30, 195, 420, 30, 8, BLUE);
  tft.fillRoundRect(volPos, 198, 20, 25, 8, YELLOW);

  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(60, 237);
  tft.print(F("L2:"));
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  tft.setCursor(110, 237);
  tft.print(dampnervol2);

  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(160, 237);
  tft.print(F("L3:"));
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  tft.setCursor(210, 237);
  tft.print(dampnervol3);

  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(260, 237);
  tft.print(F("L5:"));
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  tft.setCursor(310, 237);
  tft.print(dampnervol5);

  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(360, 237);
  tft.print(F("L7:"));
  tft.setTextColor(CYAN);
  tft.setTextSize(2);
  tft.setCursor(410, 237);
  tft.print(dampnervol7);

    // testAndPrintMapRange(25,425,-1,6);

  delay(150);
}
/********* 4. VU Meter Screen 1 ***********/
void drawPageVU() {
  Serial.println(F("DP: VU"));

  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  const int btnWidth = 80;
  const int btnHeight = 40;


  struct Detail details;
  details.fontSize = 2;            // Font Size
  details.fontColor = BLACK;       // Font color
  details.fontStartX = 85;         // X pos from left where characters start
  details.fontStartY = 30;         // Y Pos down from top where characters start
  details.fontGap = 25;            // Gap between characters
  details.startX = 80;             // Start Position from the left to right.
  details.startY = 20;             // Start Position from the top down.
  details.charRectWidth = 20;      // Width of the background box behind characters
  details.charRectHeight = 35;     // Height of the background box behind characters
  details.theGap = 5;              // The spacing between background boxes and added to text spacing
  details.delayTime = 5;           // The delay of draw between items
  details.title = (char*)PSTR("VU METER MENU");//"VU METER MENU"; // Text
    //                         1234567890123

  drawPageTitle(&details);


// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1

  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(74, 93);
  tft.print(F("A"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(179, 93);
  tft.print(F("1"));


  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(289, 93);
  tft.print(F("2"));

  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(394, 93);
  tft.print(F("3"));

// 2nd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(74, 153);
  tft.print(F("4"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(179, 153);
  tft.print(F("5"));

  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(289, 153);
  tft.print(F("6"));


  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(394, 153);
  tft.print(F("7"));

// 3rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 200; // Row 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(74, 213);
  tft.print(F("8"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(179, 213);
  tft.print(F("9"));

  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(284, 213);
  tft.print(F("10"));

  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(390, 213);
  tft.print(F("11"));

// 4rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 260; // Row 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(68, 273);
  tft.print(F("12"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(173, 273);
  tft.print(F("P2"));

  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(280, 273);
  tft.print(F("RGB"));

  tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
  tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(395, 278);
  tft.print(F("<<"));

  tft.drawRect(75, 15, 330, 45, WHITE);

}
/********* 5. VU Meter Screen 2 ***********/
void drawPageVU2() {
  Serial.println(F("DP: VU PG 2"));

  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  const int btnWidth = 80;
  const int btnHeight = 40;


  struct Detail details;
  details.fontSize = 2;            // Font Size
  details.fontColor = BLACK;       // Font color
  details.fontStartX = 85;         // X pos from left where characters start
  details.fontStartY = 30;         // Y Pos down from top where characters start
  details.fontGap = 25;            // Gap between characters
  details.startX = 80;             // Start Position from the left to right.
  details.startY = 20;             // Start Position from the top down.
  details.charRectWidth = 20;      // Width of the background box behind characters
  details.charRectHeight = 35;     // Height of the background box behind characters
  details.theGap = 5;              // The spacing between background boxes and added to text spacing
  details.delayTime = 5;           // The delay of draw between items
  details.title = (char*)PSTR("VU METER PG 2");//"VU METER PG 2"; // Text
    //                         1234567890123

  drawPageTitle(&details);


// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(68, 93);
  tft.print(F("13"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(173, 93);
  tft.print(F("14"));

  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(283, 93);
  tft.print(F("15"));

  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(388, 93);
  tft.print(F("16"));

// 2nd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(68, 153);
  tft.print(F("17"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(173, 153);
  tft.print(F("18"));

  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(283, 153);
  tft.print(F("19"));

  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(388, 153);
  tft.print(F("20"));

// 3rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 200; // Row 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(68, 213);
  tft.print(F("21"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(173, 213);
  tft.print(F("22"));


// Back Button
  tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
  tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(395, 278);
  tft.print(F("<<"));

  tft.drawRect(75, 15, 330, 45, WHITE);

}
/********* 6. Standby Screen 1 ************/
void drawPageStandby() {
  Serial.println(F("DP: Standby 1"));

  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  const int btnWidth = 80;
  const int btnHeight = 40;

  struct Detail details;
  details.fontSize = 2;            // Font Size
  details.fontColor = BLACK;       // Font color
  details.fontStartX = 85;         // X pos from left where characters start
  details.fontStartY = 30;         // Y Pos down from top where characters start
  details.fontGap = 25;            // Gap between characters
  details.startX = 80;             // Start Position from the left to right.
  details.startY = 20;             // Start Position from the top down.
  details.charRectWidth = 20;      // Width of the background box behind characters
  details.charRectHeight = 35;     // Height of the background box behind characters
  details.theGap = 5;              // The spacing between background boxes and added to text spacing
  details.delayTime = 5;           // The delay of draw between items
  details.title = (char*)PSTR("STANDBY MODES");//"STANDBY MODES"; // Text
    //                         1234567890123

  drawPageTitle(&details);


    // top row buttons

// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1

  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(74, 93);
  tft.print(F("A"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(179, 93);
  tft.print(F("1"));


  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(289, 93);
  tft.print(F("2"));

  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(394, 93);
  tft.print(F("3"));

// 2nd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(74, 153);
  tft.print(F("4"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(179, 153);
  tft.print(F("5"));

  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(289, 153);
  tft.print(F("6"));


  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(394, 153);
  tft.print(F("7"));


// 3rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 200; // Row 3
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(74, 213);
  tft.print(F("8"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(179, 213);
  tft.print(F("9"));

  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(284, 213);
  tft.print(F("10"));

  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, GRAY);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(390, 213);
  tft.print(F("N/A"));

// 4rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 260; // Row 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(68, 273);
  tft.print(F("P2"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(173, 273);
  tft.print(F("PA"));


  tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
  tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(395, 278);
  tft.print(F("<<"));

}
/********* 7. Standby Screen 2 ************/
void drawPageStandby2() {
  Serial.println(F("DP: Standby 2"));

  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  const int btnWidth = 80;
  const int btnHeight = 40;


  struct Detail details;
  details.fontSize = 2;            // Font Size
  details.fontColor = BLACK;       // Font color
  details.fontStartX = 85;         // X pos from left where characters start
  details.fontStartY = 30;         // Y Pos down from top where characters start
  details.fontGap = 25;            // Gap between characters
  details.startX = 80;             // Start Position from the left to right.
  details.startY = 20;             // Start Position from the top down.
  details.charRectWidth = 20;      // Width of the background box behind characters
  details.charRectHeight = 35;     // Height of the background box behind characters
  details.theGap = 5;              // The spacing between background boxes and added to text spacing
  details.delayTime = 5;           // The delay of draw between items
  details.title = (char*)PSTR("SB MODES PG 2");//"SB MODES PG 2"; // Text
    //                         1234567890123

  drawPageTitle(&details);


// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(68, 93);
  tft.print(F("11"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(173, 93);
  tft.print(F("12"));

  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(283, 93);
  tft.print(F("13"));

  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(388, 93);
  tft.print(F("14"));

// 2nd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(68, 153);
  tft.print(F("15"));

  btnXPos = 145; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(173, 153);
  tft.print(F("16"));

  btnXPos = 255; // Column 3
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(283, 153);
  tft.print(F("17"));

  btnXPos = 360; // Column 4
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(388, 153);
  tft.print(F("18"));


// Back Button
  tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
  tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(395, 278);
  tft.print(F("<<"));

  tft.drawRect(75, 15, 330, 45, WHITE);

}
/********* 8. Settings Screen *************/
void drawPageSettings() {
  Serial.println(F("DP: Setings"));
  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  int btnWidth = 80;
  const int btnHeight = 40;

  struct Detail details;
  details.fontSize = 2;
  details.fontColor = WHITE;
  details.fontStartX = 85;
  details.fontStartY = 30;
  details.fontGap = 25;
  details.startX = 80;
  details.startY = 20;
  details.charRectWidth = 20;
  details.charRectHeight = 35;
  details.theGap = 5;
  details.delayTime = 5;
  details.title = (char*)PSTR("SETTINGS MENU");//"SETTINGS MENU";
    //                         1234567890123

  drawPageTitle(&details);

// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1
  btnWidth = 190;
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(64, 93);
  tft.print(F("Bright & Vol"));

  btnXPos = 255; // Column 2
  btnYPos = 80; // Row 1
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(278, 93);
  tft.print(F("Sequence Time"));

  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(64, 153);
  tft.print(F("PG Saver Time"));

  btnXPos = 255; // Column 2
  tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
  tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(278, 153);
  tft.print(F("Reset / Start"));

// 3rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 200; // Row 3
// When ready for new options


  tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
  tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(395, 278);
  tft.print(F("<<"));

    // testAndPrintMapRange(25, 425, 1, 255);

}
/********* 9. Page Saver Screen **********/
void drawPageSaver() {
  long now = millis();
  if (( now - lastRun ) < TIMER ) { return; /* Return if less than 2 seconds */ }
  lastRun = now;

  pageSaverXPos = random(5,265);
  pageSaverYPos = random(5,285);
  if ( ( pageSaverXPos >= 265 ) || (pageSaverXPos <= 8 )) { pageSaverXPos = random(5,265); }
  if ( ( pageSaverYPos >= 285 ) || (pageSaverYPos <= 8 )) { pageSaverYPos = random(5,285); }

    /*
      pageSaverXPos += ( random(3) * xdir);
      pageSaverYPos += ( random(5) * ydir);
      if ( ( pageSaverXPos >= 265 ) || (pageSaverXPos <= 8 )) { xdir *= -1; }
      if ( ( pageSaverYPos >= 285 ) || (pageSaverYPos <= 8 )) { ydir *= -1; }
    */

  hueval++;
  hueval %= 360;

  if( firstScreenSaverUpdate ) {
    tft.fillScreen(BLACK);
    firstScreenSaverUpdate = false;
  }

  if(updateDisplay){
      // Ok We are in Screen Saver and there is no pattern being played (buttonPressID is in side line items), lets call the patternName something else
    if ( (-10 < buttonPressID && buttonPressID < 100) ||
         (170 < buttonPressID && buttonPressID < 200) ||
         (270 < buttonPressID && buttonPressID < 300) ) {
      setPatternName((char*)PSTR("KEWL-STUSPH"));
    }

    tft.setTextSize(3);
    tft.setTextColor(BLACK);
    tft.setCursor(lastPageSaverXPos, lastPageSaverYPos);
    tft.print(wipePatternName);

    currPageSaverRGB = HSV2RGB(hueval, random(100), random(100)); //( hueval, random(100), random(100) );
    tft.setTextColor(currPageSaverRGB);
    tft.setCursor(pageSaverXPos, pageSaverYPos);
    tft.print(patternName);

      // Save off last data for next loop around
    lastPageSaverXPos = pageSaverXPos;
    lastPageSaverYPos = pageSaverYPos;
    memset(wipePatternName,MAX_PATTERN_NAME_LEN + 1,0);
    strncpy(wipePatternName,patternName,MAX_PATTERN_NAME_LEN);
    Serial.print("name: ");Serial.print(patternName);Serial.print(", button #: ");Serial.println(buttonPressID);

    updateDisplay = true;
    return;
  }
  tft.fillScreen(BLACK);
  updateDisplay = true;

}
/********* 10. Saver Time  Set Screen *****/
void drawPageSaverStartTime() {
  int lcv = 0;
  if(updateDisplay){
      //over,down ,length,height
    tft.drawRoundRect(30, 110, 420, 30, 8, BLUE);
    tft.fillRoundRect(oldSavePointX, 113, 20, 25, 8, BLACK);
    tft.fillRoundRect(savePointX1, 113, 20, 25, 8, WHITE);

    updateDisplay = false;
    return;
  }
  Serial.println(F("DP: PageSaverTime"));
    //if (updateDisplay == false) {

  tft.fillScreen(BLACK);

  struct Detail details;
  details.fontSize = 2;
  details.fontColor = WHITE;
  details.fontStartX = 85;
  details.fontStartY = 30;
  details.fontGap = 25;
  details.startX = 80;
  details.startY = 20;
  details.charRectWidth = 20;
  details.charRectHeight = 35;
  details.theGap = 5;
  details.delayTime = 5;
  details.title = (char*)PSTR("PG SAVER TIME");//"PG SAVER TIME";
    //                         1234567890123

  drawPageTitle(&details);

  pageSaverSeconds = map(savePointX1, 20, 420, 9, 90);
  tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
  tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(395, 278);
  tft.print(F("<<"));

  tft.drawRect(75, 15, 330, 45, WHITE);

    //Testing Stuff
  tft.setTextColor(WHITE);
  tft.setCursor(180, 84);
  tft.setTextSize(2);
  tft.print(F("DELAY TIME: "));

  tft.drawRoundRect(30, 110, 420, 30, 8, BLUE);
  tft.fillRoundRect(savePointX1, 113, 20, 25, 8, WHITE);

  tft.setTextColor(CYAN);
  tft.setCursor(215, 184);
  tft.setTextSize(4);
  tft.print(pageSaverSeconds);
  tft.setTextColor(YELLOW);
  tft.setCursor(275, 200);
  tft.setTextSize(2);
  tft.print(F("Seconds"));

  delay(150);

}
/********* 11. Cycle Time  Set Screen *****/
void drawPageCycleTime() {
  int lcv = 0;
  if(updateDisplay){
      //over,down ,length,height
    tft.drawRoundRect(30, 110, 420, 30, 8, BLUE);
    tft.fillRoundRect(oldSavePointX, 113, 20, 25, 8, BLACK);
    tft.fillRoundRect(savePointX2, 113, 20, 25, 8, WHITE);

    updateDisplay = false;
    return;
  }
  Serial.println(F("DP: PageCycleTime"));
    //if (updateDisplay == false) {

  struct Detail details;
  details.fontSize = 2;
  details.fontColor = WHITE;
  details.fontStartX = 85;
  details.fontStartY = 30;
  details.fontGap = 25;
  details.startX = 80;
  details.startY = 20;
  details.charRectWidth = 20;
  details.charRectHeight = 35;
  details.theGap = 5;
  details.delayTime = 5;
  details.title = (char*)PSTR("PG CYCLE TIME");//"PG CYCLE TIME";
    //                         1234567890123

  drawPageTitle(&details);

  meterCycleSeconds = map(savePointX2, 20, 420, 9, 90);
  tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
  tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
  tft.setTextSize(2);
  tft.setTextColor(WHITE);
  tft.setCursor(395, 278);
  tft.print(F("<<"));

  tft.drawRect(75, 15, 330, 45, WHITE);

    //Testing Stuff
  tft.setTextColor(WHITE);
  tft.setCursor(180, 84);
  tft.setTextSize(2);
  tft.print(F("DELAY TIME: "));

  tft.drawRoundRect(30, 110, 420, 30, 8, BLUE);
  tft.fillRoundRect(savePointX2, 113, 20, 25, 8, WHITE);

  tft.setTextColor(CYAN);
  tft.setCursor(215, 184);
  tft.setTextSize(4);
  tft.print(meterCycleSeconds);
  tft.setTextColor(YELLOW);
  tft.setCursor(275, 200);
  tft.setTextSize(2);
  tft.print(F("Seconds"));

  delay(150);

}
/********* 12. RGB Control Screen *********/
void drawPageRGBControl() {
  Serial.println(F("Draw Page: RGB Control"));

  if(updateDisplay){
//over,down ,length,height
    tft.drawRoundRect(30, 115, 420, 30, 8, BLUE);
    tft.fillRoundRect(oldred1, 118, 20, 25, 8,BLACK);
    tft.fillRoundRect(red1, 118, 20, 25, 8, RED);

    tft.drawRoundRect(30, 165, 420, 30, 8, BLUE);
    tft.fillRoundRect(oldgreen1, 168, 20, 25, 8, BLACK);
    tft.fillRoundRect(green1, 168, 20, 25, 8, LTGREEN);

    tft.drawRoundRect(30, 215, 420, 30, 8, BLUE);
    tft.fillRoundRect(oldblue1, 218, 20, 25, 8, BLACK);
    tft.fillRoundRect(blue1, 218, 20, 25, 8, BLUE);

    updateDisplay = false;
  }
  else { // (updateDisplay == false){

    struct Detail details;
    details.fontSize = 2;            // Font Size
    details.fontColor = WHITE;       // Font color
    details.fontStartX = 85;         // X pos from left where characters start
    details.fontStartY = 30;         // Y Pos down from top where characters start
    details.fontGap = 25;            // Gap between characters
    details.startX = 80;             // Start Position from the left to right.
    details.startY = 20;             // Start Position from the top down.
    details.charRectWidth = 20;      // Width of the background box behind characters
    details.charRectHeight = 35;     // Height of the background box behind characters
    details.theGap = 5;              // The spacing between background boxes and added to text spacing
    details.delayTime = 5;           // The delay of draw between items
    details.title = (char*)PSTR(" RGB CONTROL ");//" RGB CONTROL "; // Text
      //                         1234567890123

    drawPageTitle(&details);


    tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));

    tft.drawRect(75, 15, 330, 45, WHITE);

    tft.setTextColor(WHITE);
    tft.setCursor(60, 80);
    tft.setTextSize(2);
    tft.print(F("RGB MIXER: "));

    tft.setTextColor(YELLOW);
    tft.setCursor(190, 80);
    tft.setTextSize(2);
    tft.print(F("R:"));
    tft.setTextColor(CYAN);
    tft.setCursor(220, 80);
    tft.setTextSize(2);
    tft.print(redmin);

    tft.setTextColor(YELLOW);
    tft.setCursor(270, 80);
    tft.setTextSize(2);
    tft.print(F("G:"));
    tft.setTextColor(CYAN);
    tft.setCursor(300, 80);
    tft.setTextSize(2);
    tft.print(greenmin);

    tft.setTextColor(YELLOW);
    tft.setCursor(350, 80);
    tft.setTextSize(2);
    tft.print(F("B:"));
    tft.setTextColor(CYAN);
    tft.setCursor(380, 80);
    tft.setTextSize(2);
    tft.print(bluemin);

    tft.drawRoundRect(30, 115, 420, 30, 8, BLUE);
    tft.drawRoundRect(30, 165, 420, 30, 8, BLUE);
    tft.drawRoundRect(30, 215, 420, 30, 8, BLUE);

    tft.fillRoundRect(red1, 118, 20, 25, 8,RED);
    tft.fillRoundRect(green1, 168, 20, 25, 8, LTGREEN);
    tft.fillRoundRect(blue1, 218, 20, 25, 8, BLUE);
  }

  delay(50);
}
/********* 13.Pallete +/- Screen **********/
void drawPagePalettePlusMinus() {
  Serial.println(F("DP: Palete +/-"));

  if(updateDisplay){
//over,down ,length,height
      // HANS
    tft.fillRect(280, 245, 50, 40, BLACK);    // L2 vol

    tft.setCursor(290, 250);
    tft.setTextColor(CYAN);
    tft.print(paletteIndex);
    updateDisplay = false;
  }
  else {
    struct Detail details;
    details.fontSize = 2;            // Font Size
    details.fontColor = BLACK;       // Font color
    details.fontStartX = 110;        // X pos from left where characters start
    details.fontStartY = 45;         // Y Pos down from top where characters start
    details.fontGap = 25;            // Gap between characters
    details.startX = 105;            // Start Position from the left to right.
    details.startY = 30;             // Start Position from the top down.
    details.charRectWidth = 20;      // Width of the background box behind characters
    details.charRectHeight = 45;     // Height of the background box behind characters
    details.theGap = 5;              // The spacing between background boxes and added to text spacing
    details.delayTime = 5;           // The delay of draw between items
    details.title = (char*)PSTR("KEWL-STUSPH");//"KEWL-STUSPH";   // text
      //                         12345678901

      //BANNER
    drawPageTitle(&details);

    tft.fillRoundRect(60, 110, 360, 40, 8, BLUE8);
    tft.drawRoundRect(60, 110, 360, 40, 8, WHITE);

    tft.fillRoundRect(60, 170, 360, 40, 8, BLUE8);
    tft.drawRoundRect(60, 170, 360, 40, 8, WHITE);

    tft.setTextColor(WHITE);

    tft.setCursor(190, 123);
    tft.print(F("PALETTE +"));

    tft.setCursor(190, 183);
    tft.print(F("PALETTE -"));

    tft.setTextColor(YELLOW);
    tft.setCursor(190, 250);
    tft.print(F("PALETTE:"));
    tft.setCursor(290, 250);
    tft.setTextColor(CYAN);
    tft.print(paletteIndex);

    tft.setTextSize(1);
    tft.setCursor(160, 305);
    tft.setTextColor(WHITE);
    tft.print(F("Select one of the options..."));

    tft.fillRoundRect(380, 270, 60, 30, 8, BLUE11);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));
  }

}


//////////////////// KEY EVENT Handling //////////////////////////
void handleHomeScreenEvents(TSPoint point) {
    // IS_POINT_IN_TARGET(point,startX,xPxWidth,startY,yPxHeight)
  if (IS_POINT_IN_TARGET(point,60,360,110,40)) {
    tft.fillRoundRect(60, 110, 360, 40, 8, BLACK); // Black the button to indicate what was pressed
    tft.drawRoundRect(60, 110, 360, 40, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);                       // White Text on Black
    tft.setCursor(185, 125);
    tft.print(F("VU METERS"));

    currentPage = PG_VU;
    bNeedRedraw = true;
    return;
  }
  if (IS_POINT_IN_TARGET(point,60,360,170,40)) {
    tft.fillRoundRect(60, 170, 360, 40, 8, BLACK);
    tft.drawRoundRect(60, 170, 360, 40, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(150, 185);
    tft.print(F("STANDBY PATTERNS"));

    currentPage = PG_STANDBY;
    bNeedRedraw = true;
    return;
  }
  if (IS_POINT_IN_TARGET(point,60,360,230,40)) {
    tft.fillRoundRect(60, 230, 360, 40, 8, BLACK);
    tft.drawRoundRect(60, 230, 360, 40, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(195, 243);
    tft.print(F("SETTINGS"));

    currentPage = PG_SETTING;
    bNeedRedraw = true;
    return;
  }

    // NERD: This is where we would draw credits if we have the space redifined.
}

void handleCredentialEvents(TSPoint point) {
    // IS_POINT_IN_TARGET(p,startX,xPxWidth,startY,yPxHeight)
  if(IS_POINT_IN_TARGET(point,380,60,270,30)) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));
    currentPage = PG_HOME;

    bNeedRedraw = true;
  }
}

void handleVUMeterEvents(TSPoint point) {
  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  const int btnWidth = 80;
  const int btnHeight = 40;
  int prevButtonID = buttonPressID;

  if ( IS_POINT_IN_TARGET(point,380,60,270,30) ) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));
    currentPage = PG_HOME;

    bNeedRedraw = true;

    return;
  }

// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=100;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(74, 93);
    tft.print(F("A"));
    vuCurrentPatternNumber = 0;  // Reset the VU Pattern number to start from beginning
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=101;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(179, 93);
    tft.print(F("1"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 255; // Column 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=102;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(289, 93);
    tft.print(F("2"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 360; // Column 4
  if( IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=103;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(394, 93);
    tft.print(F("3"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
// 2nd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  if( IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=104;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(74, 153);
    tft.print(F("4"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=105;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(179, 153);
    tft.print(F("5"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 255; // Column 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=106;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(289, 153);
    tft.print(F("6"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 360; // Column 4
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=107;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(394, 153);
    tft.print(F("7"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
// 3rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 200; // Row 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=108;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(!50);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(74, 213);
    tft.print(F("8"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=109;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(179, 213);
    tft.print(F("9"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 255; // Column 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=110;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(284, 213);
    tft.print(F("10"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 360; // Column 4
  if (IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=111;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(390, 213);
    tft.print(F("11"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
// 4rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 260; // Row 4
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=112;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(68, 273);
    tft.print(F("12"));
    currentPage = PG_VU; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=198;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(173, 273);
    tft.print(F("P2"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = true;
  }
  btnXPos = 255; // Column 3
  if (IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=199;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(280, 273);
    tft.print(F("RGB"));
    currentPage = PG_RGBCONTROL; // Good to go

    bNeedRedraw = true;
  }

  if ( prevButtonID != buttonPressID ) {
    blackOut();
    delay(25);
  }

}

void handleVUMeterPage2Events(TSPoint point) {
  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  const int btnWidth = 80;
  const int btnHeight = 40;
  int prevButtonID = buttonPressID;

  if ( IS_POINT_IN_TARGET(point,380,60,270,30) ) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));
    currentPage = PG_HOME;

    bNeedRedraw = true;

    return;
  }

// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=113;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(68, 93);
    tft.print(F("13"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=114;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(173, 93);
    tft.print(F("14"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 255; // Column 3
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=115;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(283, 93);
    tft.print(F("15"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 360; // Column 4
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=116;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(388, 93);
    tft.print(F("16"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=117;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(68, 153);
    tft.print(F("17"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=118;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(173, 153);
    tft.print(F("18"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 255; // Column 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=119;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(283, 153);
    tft.print(F("19"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 360; // Column 4
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=120;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(388, 153);
    tft.print(F("20"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 40; // Column 1
  btnYPos = 200; // Row 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=121;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(68, 213);
    tft.print(F("21"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=122;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(173, 213);
    tft.print(F("22"));
    currentPage = PG_VU_2; // Good to go

    bNeedRedraw = false;
  }

  if ( prevButtonID != buttonPressID ) {
    blackOut();
    delay(25);
  }

}

void handleStandbyEvents(TSPoint point) {
  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  const int btnWidth = 80;
  const int btnHeight = 40;
  int prevButtonID = buttonPressID;
  
  if(IS_POINT_IN_TARGET(point,380, 60, 270, 30) ) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));

    currentPage = PG_HOME;

    bNeedRedraw = true;
  }

// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID = 200;
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(74, 93);
    tft.print(F("A"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=201;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(179, 93);
    tft.print(F("1"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 255; // Column 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=202;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(289, 93);
    tft.print(F("2"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 360; // Column 4
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=203;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(394, 93);
    tft.print(F("3"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }
// 2nd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=204;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(74, 153);
    tft.print(F("4"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=205;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(179, 153);
    tft.print(F("5"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 255; // Column 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=206;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(289, 153);
    tft.print(F("6"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 360; // Column 4
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=207;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(394, 153);
    tft.print(F("7"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }

// 3rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 200; // Row 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=208;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(74, 213);
    tft.print(F("8"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=209;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(179, 213);
    tft.print(F("9"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 255; // Column 3
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=210;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(284, 213);
    tft.print(F("10"));
    currentPage = PG_STANDBY; // Good to go

    bNeedRedraw = false;
  }
  
  btnXPos = 360; // Column 4
  /*  GEEK:  This is N/A, wwe are reserving for another item  we have disabled it as I really do not have a good use for it.
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    currentPage = PG_STANDBY; 

    bNeedRedraw = true;
  }
  */
// 4rd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 260; // Row 4
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=299;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(68, 273);
    tft.print(F("P2"));
    currentPage = PG_STANDBY_2; // Good to go

    bNeedRedraw = true;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
      // NERD: DO NOT CHange the render selection buttonPressID=298;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(173, 273);
    tft.print(F("PA"));
    currentPage = PG_PALLETTE; // Good to go

    bNeedRedraw = true;
  }
  
  if ( prevButtonID != buttonPressID ) {
    blackOut();
    delay(25);
  }
}

void handleStandbyPage2Events(TSPoint point) {
  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  const int btnWidth = 80;
  const int btnHeight = 40;
  int prevButtonID = buttonPressID;

  if ( IS_POINT_IN_TARGET(point,380,60,270,30) ) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));
    currentPage = PG_HOME;

    bNeedRedraw = true;

    return;
  }

// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=211;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(68, 93);
    tft.print(F("11"));
    currentPage = PG_STANDBY_2; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=212;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(173, 93);
    tft.print(F("12"));
    currentPage = PG_STANDBY_2; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 255; // Column 3
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=213;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(283, 93);
    tft.print(F("13"));
    currentPage = PG_STANDBY_2; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 360; // Column 4
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=214;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(388, 93);
    tft.print(F("14"));
    currentPage = PG_STANDBY_2; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=215;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(68, 153);
    tft.print(F("15"));
    currentPage = PG_STANDBY_2; // Good to go

    bNeedRedraw = false;
  }
  btnXPos = 145; // Column 2
  if(IS_POINT_IN_TARGET(point,btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=216;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(173, 153);
    tft.print(F("16"));
    currentPage = PG_STANDBY_2; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 255; // Column 3
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=217;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(283, 153);
    tft.print(F("17"));
    currentPage = PG_STANDBY_2; // Good to go

    bNeedRedraw = false;
  }

  btnXPos = 360; // Column 4
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=214;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(388, 153);
    tft.print(F("18"));
    currentPage = PG_STANDBY_2; // Good to go

    bNeedRedraw = false;
  }
  
  if ( prevButtonID != buttonPressID ) {
    blackOut();
    delay(25);
  }

}

void handlePaletteEvents(TSPoint point) {
    // IS_POINT_IN_TARGET(point,startX,xPxWidth,startY,yPxHeight)
  if(IS_POINT_IN_TARGET(point,60,360,110,40)) {
    tft.fillRoundRect(60, 110, 360, 40, 8, BLUE8);///////////////////////////////////////////////////////
    tft.drawRoundRect(60, 110, 360, 40, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(190, 123);
    tft.print(F("PALETTE +"));
    paletteIndex++;
    if (paletteIndex > MAX_PALETTE_INDEX){
      paletteIndex = 1;
    }

    currentPage = PG_PALLETTE;

    updateDisplay = true;
    bNeedRedraw = true;
  }
  if(IS_POINT_IN_TARGET(point,60,360,170,40)) {
    tft.fillRoundRect(60, 170, 360, 40, 8, BLUE8);
    tft.drawRoundRect(60, 170, 360, 40, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(190, 183);
    tft.print(F("PALETTE -"));
    paletteIndex--;
    if (paletteIndex < 1){
      paletteIndex = MAX_PALETTE_INDEX;
    }

    currentPage = PG_PALLETTE;

    updateDisplay = true;
    bNeedRedraw = true;
  }
  if(IS_POINT_IN_TARGET(point,380,60,270,30)) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));
    currentPage = PG_STANDBY;
    bNeedRedraw = true;
  }
}

void handleBriteVolPageEvents(TSPoint point) {
    // IS_POINT_IN_TARGET(point,startX,xPxWidth,startY,yPxHeight)
  if(IS_POINT_IN_TARGET(point,380,60,280,30)) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));

    // all pixels to 'off'
//    strip.begin();
//    strip.show(); 
//    FastLED.clear();
    
    currentPage = PG_SETTING;

    bNeedRedraw = true;
    return;
  }

////////////////////Brightness Slider////////////////////////
  if(IS_POINT_IN_TARGET(point,30,430,100,50)) {
    tft.fillRoundRect(320, 84, 64, 30, 8, BLACK);

    currentPage = PG_BRIGHTVOL;// Good to go
    oldBrightnessPos = brightnessPos;
    brightnessPos = point.x;
    brightnessPos = (brightnessPos > 420 ) ? 420 : brightnessPos;
    BRIGHTNESS = map(brightnessPos, 25, 425, 0, 255);

//    FastLED.setBrightness(BRIGHTNESS);
//    strip.setBrightness(BRIGHTNESS);

    tft.setTextColor(CYAN);
    tft.setCursor(320, 84);
    tft.setTextSize(2);
    tft.print(BRIGHTNESS);

    updateDisplay = true;
    bNeedRedraw = true;
    return;
  }
  if(IS_POINT_IN_TARGET(point,30,430,185,50)) {
    tft.fillRect(105, 235, 40, 20, BLACK);    // L2 vol
    tft.fillRect(205, 235, 40, 20, BLACK);    // L3 Vol
    tft.fillRect(305, 235, 40, 20, BLACK);    // L5 Vol
    tft.fillRect(405, 235, 40, 20, BLACK);    // L7 Vol

    currentPage = PG_BRIGHTVOL;// Good to go
    oldVolPos = volPos;
    volPos = point.x;
    volPos = (volPos > 420 ) ? 420 : volPos;
    dampnervol2 = map(volPos, 25, 425, 1, 6); // map(volPos, 25, 425, 1, 8);
    dampnervol3 = map(volPos, 25, 425, 1, 50); // map(volPos, 25, 425, 60, 1); // 265
    dampnervol5 = map(volPos, 25, 425, -1, 10); // map(volPos, 25, 425, .2, 8); // 270
    dampnervol7 = map(volPos, 25, 425, -1, 11); // map(volPos, 25, 425, .2, 8); // 278

    updateDisplay = true;
    bNeedRedraw = true;
    return;
  }
}

void handleSettingsPageEvents(TSPoint point) {
  int btnXPos = 40; // Column 1
  int btnYPos = 80; // Row 1
  const int btnWidth = 190;
  const int btnHeight = 40;
  updateDisplay = false;
    // IS_POINT_IN_TARGET(point,startX,xPxWidth,startY,yPxHeight)

  if ( IS_POINT_IN_TARGET(point,380,60,270,30) ) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));
    currentPage = PG_HOME;

    bNeedRedraw = true;

    return;
  }

// 1st Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 80; // Row 1
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=300;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(64, 93);
    tft.print(F("Bright & Vol"));
    currentPage = PG_BRIGHTVOL; // Good to go

    bNeedRedraw = true;
  }
  btnXPos = 255; // Column 2
  btnYPos = 80; // Row 1
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=300;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(278, 93);
    tft.print(F("Sequence Time"));
    currentPage = PG_CYCLETIME; // Good to go

    bNeedRedraw = true;
  }
// 2nd Row Buttons
  btnXPos = 40; // Column 1
  btnYPos = 140; // Row 2
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=300;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(64, 153);
    tft.print(F("PG Saver Time"));
    currentPage = PG_SCREENSAVERTIME; // Good to go

    bNeedRedraw = true;
  }
  btnXPos = 255; // Column 2
  if (IS_POINT_IN_TARGET(point, btnXPos, btnWidth, btnYPos, btnHeight) ) {
    buttonPressID=300;
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLACK);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    delay(150);
    tft.fillRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, BLUE8);
    tft.drawRoundRect(btnXPos, btnYPos, btnWidth, btnHeight, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(278, 153);
    tft.print(F("Reset / Start"));
    currentPage = PG_RESTART; // Good to go

    bNeedRedraw = true;
  }

}

void handleCycleTimeEvents(TSPoint point) {
    // IS_POINT_IN_TARGET(point,startX,xPxWidth,startY,yPxHeight)
  if(IS_POINT_IN_TARGET(point,380,60,280,30)) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));

    currentPage = PG_SETTING;

    bNeedRedraw = true;
    return;
  }
    // IS_POINT_IN_TARGET(point,30,430,110,30) // NEED ROOM FOr FAT Fingers
  if(IS_POINT_IN_TARGET(point,30,430,100,50)) {
    tft.fillRect(214, 184, 64, 30, BLACK);
    currentPage = PG_CYCLETIME; // Good to go
      // Save off data for next possible change
    oldMeterCycleSeconds = meterCycleSeconds;
    oldSavePointX = savePointX2;
      // Map the point and value
    savePointX2 = (point.x > 420 ) ? 420 : point.x;
    meterCycleSeconds = map(savePointX2, 20, 420, 9, 90);

      // Draw the new value
    tft.setTextColor(CYAN);
    tft.setCursor(215, 184);
    tft.setTextSize(4);
    tft.print(meterCycleSeconds);

    updateDisplay = true;
    bNeedRedraw = true;
    return;
  }
}

void handlePageSaverSartTimeEvents(TSPoint point) {
    // IS_POINT_IN_TARGET(point,startX,xPxWidth,startY,yPxHeight)
  if(IS_POINT_IN_TARGET(point,380,60,280,30)) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));

    currentPage = PG_SETTING;

//    resetPageSaverThread();

    bNeedRedraw = true;
    return;
  }
    // IS_POINT_IN_TARGET(point,30,430,110,30) // NEED ROOM FOr FAT Fingers
  if(IS_POINT_IN_TARGET(point,30,430,100,50)) {
    tft.fillRoundRect(210, 184, 64, 30, 8, BLACK);

    currentPage = PG_SCREENSAVERTIME;// Good to go
      // Save off data for next possible change
    oldSaverSeconds = pageSaverSeconds;
    oldSavePointX = savePointX1;
      // Map the point and value
    savePointX1 = (point.x > 420 ) ? 420 : point.x;
    pageSaverSeconds = map(savePointX1, 20, 420, 9, 90);

      // Draw the new value
    tft.setTextColor(CYAN);
    tft.setCursor(215, 184);
    tft.setTextSize(4);
    tft.print(pageSaverSeconds);

    updateDisplay = true;
    bNeedRedraw = true;
    return;
  }

}

void handlePageRGBVControl(TSPoint point) {
    // IS_POINT_IN_TARGET(point,startX,xPxWidth,startY,yPxHeight)
  if(IS_POINT_IN_TARGET(point,380,60,280,30)) {
      // Back button
    tft.fillRoundRect(380, 270, 60, 30, 8, BLACK);
    tft.drawRoundRect(380, 270, 60 , 30, 8, WHITE);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.setCursor(395, 278);
    tft.print(F("<<"));

    currentPage = PG_VU;

    bNeedRedraw = true;
    return;
  }
    // IS_POINT_IN_TARGET(point,30,430,115,30) // NEED ROOM FOr FAT Fingers
  if(IS_POINT_IN_TARGET(point,30,430,105,50)) {
    tft.drawRoundRect(30, 115, 420, 30, 8, BLUE);
    tft.fillRoundRect(red1, 118, 20, 25, 8, RED);
    currentPage = PG_RGBCONTROL;// Not sure what you want to do here
    oldred1 = red1;
    red1 = (point.x > 420 ) ? 420 : point.x;
    redmin = map(red1, 25, 440, 0, 255);

    tft.fillRect(219, 79, 40, 25, BLACK);    // REDMIN erase
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(220, 80);
    tft.print(redmin);

    updateDisplay = true;
    bNeedRedraw = true;
    return;
  }
    // if(IS_POINT_IN_TARGET(point,30,430,165,30)) {  // NEED ROOM FOr FAT Fingers
  if(IS_POINT_IN_TARGET(point,30,430,155,50)) {
    tft.drawRoundRect(30, 165, 420, 30, 8, BLUE);
    tft.fillRoundRect(green1, 168, 20, 25, 8, LTGREEN);
    currentPage = PG_RGBCONTROL;// Not sure what you want to do here
    oldgreen1 = green1;
    green1 = (point.x > 420 ) ? 420 : point.x;
    greenmin = map(green1, 25, 440, 0, 255);

    tft.fillRect(299, 79, 40, 25, BLACK);    // GREENMIN erase
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(300, 80);
    tft.print(greenmin);


    updateDisplay = true;
    bNeedRedraw = true;
    return;
  }
    // if(IS_POINT_IN_TARGET(point,30,430,215,30)) {  // NEED ROOM FOr FAT Fingers
  if(IS_POINT_IN_TARGET(point,30,430,205,50)) {
    tft.drawRoundRect(30, 215, 420, 30, 8, BLUE);
    tft.fillRoundRect(blue1, 218, 20, 25, 8,BLUE);
    currentPage = PG_RGBCONTROL;// Not sure what you want to do here
    oldblue1 = blue1;
    blue1 = (point.x > 420 ) ? 420 : point.x;
    bluemin = map(blue1, 25, 440, 0, 255);//420

    tft.fillRect(379, 79, 40, 25, BLACK);    // BLUEMIN erase
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.setCursor(380, 80);
    tft.print(bluemin);

    updateDisplay = true;
    bNeedRedraw = true;
    return;
  }
}

void handlePageSaverEvents( TSPoint point) {

  if(IS_POINT_IN_TARGET(point,5,475,5,315)) {
      // Back button
    int xPos = point.x - 30;
    int yPos = point.y - 20;
    tft.fillRoundRect(xPos, yPos, 80, 40, 8, BLUE8);
    tft.drawRoundRect(xPos, yPos, 80, 40, 8, WHITE);
    delay(150);
    tft.fillRoundRect(xPos, yPos, 80, 40, 8, BLACK);
      
    currentPage = lastPage;
    updateDisplay = false; // true;
    bNeedRedraw = true;
//    resetPageSaverThread(); 
    firstScreenSaverUpdate = true; // Reset for next time
//    tft.fillScreen(BLACK);
    return;
  }

}



//////////////////////// Other Functions /////////////////////////


#define MIN(left,right) (( left <= right ) ? left : right )
#define MAX(left,right) (( left >= right ) ? left : right )

long HSV2RGB(int hue, int sat, int val) {

  int i;
  float f,p,q,t;
  float h,s,v;
  long r,g,b;
  long longRGB;

    //expand the u8 hue in range 0->255 to 0->359* (there are problems at exactly 360)
  h = 359.0 * ((float)hue / 255.0 );

  h = MAX(0.0, MIN(360.0,h));
  s = MAX(0.0, MIN(100.0,sat));
  v = MAX(0.0, MIN(100.0,val));

  s /= 100;
  v /= 100;

  if( s == 0 ) {
      // Achromatic Grey
    r = g = b = round(v*255);
// long RGB = ((long)R << 16L) | ((long)G << 8L) | (long)B;
    longRGB = ((long)r << 16L) | ((long)g << 8L) | (long)b;
    return longRGB;
  }

  h /= 60;  //sector 0 to 5
  i = floor(h);
  f = h - i; // Factorial part of h
  p = v * (1 - s);
  q = v * (1 - s * f);
  t = v * (1 - s * (1 - f));
  switch(i) {
    case 0:
      r = round(255*v);
      g = round(255*t);
      b = round(255*p);
      break;
    case 1:
      r = round(255*q);
      g = round(255*v);
      b = round(255*p);
      break;
    case 2:
      r = round(255*p);
      g = round(255*v);
      b = round(255*t);
      break;
    case 3:
      r = round(255*p);
      g = round(255*q);
      b = round(255*v);
      break;
    case 4:
      r = round(255*t);
      g = round(255*p);
      b = round(255*v);
      break;
    default: // case 5:
      r = round(255*v);
      g = round(255*p);
      b = round(255*q);
  }
  
  longRGB = ((long)r << 16L) | ((long)g << 8L) | (long)b;
  return longRGB;

}

void blackOut() {
  

  setPatternName((char*)PSTR(" "));
}
