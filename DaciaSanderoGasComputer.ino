#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>
#include "layout.h"
#include "logo.h"
#include "sandero.h"
#include "elm327_connection.h"
#include "display_init.h"

// Prawidłowe piny dla Twojej płytki
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4
#define TFT_BLK  32
#define BOOT_BUTTON_PIN 0

// Kolory
#define COLOR_HEADER ST77XX_CYAN
#define COLOR_VALUE ST77XX_WHITE
#define COLOR_ERROR ST77XX_RED

// Zmienne globalne
bool debugMode = false;
String debugLog[10]; // Bufor na ostatnie 10 linii logów
int debugLogIndex = 0;

// Makro do logowania na Serial i do bufora ekranu
#define DEBUG_LOG(msg) do { DEBUG_PORT.println(msg); addDebugLog(String(msg)); } while(0)

// Funkcja do dodawania logów do bufora
void addDebugLog(String message) {
  debugLog[debugLogIndex] = message;
  debugLogIndex = (debugLogIndex + 1) % 10;
}

// Funkcja do wyświetlania logów na ekranie
void displayDebugLog() {
  if (!tftPtr) return;
  tftPtr->fillScreen(ST77XX_BLACK);
  tftPtr->setTextColor(ST77XX_WHITE);
  tftPtr->setTextSize(1);
  int y = 10;
  for (int i = 0; i < 10; i++) {
    int index = (debugLogIndex - 1 - i + 10) % 10;
    if (debugLog[index].length() > 0) {
      tftPtr->setCursor(5, y);
      tftPtr->print(debugLog[index]);
      y += 15;
    }
  }
  tftPtr->setCursor(5, 160);
  tftPtr->setTextColor(ST77XX_YELLOW);
  tftPtr->print("DEBUG MODE - BOOT to exit");
}

// Funkcja do sprawdzenia guzika BOOT
void checkBootButton() {
  static bool lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 50;
  bool buttonState = digitalRead(BOOT_BUTTON_PIN);
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState == LOW && lastButtonState == HIGH) {
      debugMode = !debugMode;
      if (debugMode) {
        addDebugLog("=== DEBUG MODE ON ===");
      } else {
        addDebugLog("=== DEBUG MODE OFF ===");
        drawLayout(SCREEN_WIDTH, SCREEN_HEIGHT, TOP_HEIGHT, topLeftW, topLeftH, topRightW, topRightH, current_fuel_type_code);
      }
    }
  }
  lastButtonState = buttonState;
}

// Inicjalizacja wyświetlacza z prawidłowymi pinami
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  DEBUG_PORT.begin(115200);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);
  tft.init(170, 320);
  tft.setRotation(3);
  setDisplay(&tft);
  initializeDisplay();
  showStartupScreens();
  addDebugLog("=== DEBUG MODE ON ===");
  showConnectionStatus("Podlaczanie do auta", ST77XX_BLUE);
  DEBUG_LOG("Podlaczanie do auta");
  if (!initializeBluetoothConnection()) {
    DEBUG_LOG("Blad polaczenia z Bluetooth");
    while (1);
  }
  DEBUG_LOG("Bluetooth OK");
  if (!initializeELM327Connection()) {
    DEBUG_LOG("Blad polaczenia z ELM327");
    while (1);
  }
  DEBUG_LOG("ELM327 OK");
  drawLayout(SCREEN_WIDTH, SCREEN_HEIGHT, TOP_HEIGHT, topLeftW, topLeftH, topRightW, topRightH, 0xFF);
}

void loop() {
  checkBootButton();
  if (debugMode) {
    displayDebugLog();
    delay(100);
  } else {
    readAndDisplayOBD();
    delay(10);
  }
}