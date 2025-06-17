#include "BluetoothSerial.h"
#include "ELMduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Piny ekranu
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32

#define TFT_WIDTH 320
#define TFT_HEIGHT 170

Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

uint8_t address[6] = { 0xDC, 0x0D, 0x30, 0x9F, 0x96, 0x0E };  // PRZYKŁADOWY ADRES! ZMIEŃ GO!

ELM327 myELM327;


uint32_t rpm = 0;
uint8_t speed = 0;
// Kolory
#define COLOR_HEADER ST77XX_CYAN
#define COLOR_VALUE ST77XX_WHITE
#define COLOR_ERROR ST77XX_RED

void showCenteredText(const String text, uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(2);
  tft.setTextColor(color);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int16_t x = (TFT_WIDTH - w) / 2 - x1;
  int16_t y = (TFT_HEIGHT - h) / 2 - y1;

  tft.setCursor(x, y);
  tft.print(text);
}


void setup() {
#if LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif
  // TFT init
  tft.init(170, 320);  // szer x wys
  tft.setRotation(3);
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH);

  DEBUG_PORT.begin(115200);
  showCenteredText("Podlaczanie do auta", ST77XX_BLUE);
  SerialBT.setPin("1234", 4);
  ELM_PORT.begin("Czajo_x5pro", true); //nazwa urządzenia poprzednio podpiętego pod iCara

  if (!ELM_PORT.connect(address)) {
    DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
    showCenteredText("Brak polaczenia z BT", ST77XX_RED);
    while (1)
      ;
  }

  if (!myELM327.begin(ELM_PORT, false, 2000)) {
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    showCenteredText("Brak polaczenia z OBD", ST77XX_RED);
    while (1)
      ;
  }

  Serial.println("Connected to ELM327");
  showCenteredText("Polaczono z OBD", ST77XX_GREEN);
}

void wyswietlrpm() {
float tempRPM = myELM327.rpm();


  if (myELM327.nb_rx_state == ELM_SUCCESS) {
    rpm = (uint32_t)tempRPM;
    Serial.print("RPM: ");
    Serial.println(rpm);
    showCenteredText("RPM: " + (String)rpm, ST77XX_GREEN);
  } else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    myELM327.printError();
  delay(100);
}

void loop() {
  wyswietlrpm();
}