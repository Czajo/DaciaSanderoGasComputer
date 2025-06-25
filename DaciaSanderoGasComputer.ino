#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>
#include "layout.h"
#include "logo.h"
#include "sandero.h"
#include "elm327_connection.h"
#include "display_init.h"

// Kolory
#define COLOR_HEADER ST77XX_CYAN
#define COLOR_VALUE ST77XX_WHITE
#define COLOR_ERROR ST77XX_RED

// Definicje pinów
#define BOOT_BUTTON_PIN 0

// Zmienne globalne
bool debugMode = false;
String debugLog[10]; // Bufor na ostatnie 10 linii logów
int debugLogIndex = 0;

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
  
  // Wyświetl informację o trybie
  tftPtr->setCursor(5, SCREEN_HEIGHT - 20);
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
      // Guzik został wciśnięty
      debugMode = !debugMode;
      
      if (debugMode) {
        addDebugLog("=== DEBUG MODE ON ===");
        displayDebugLog();
      } else {
        addDebugLog("=== DEBUG MODE OFF ===");
        // Przywróć normalny layout
        drawLayout(SCREEN_WIDTH, SCREEN_HEIGHT, TOP_HEIGHT, topLeftW, topLeftH, topRightW, topRightH, current_fuel_type_code);
      }
    }
  }
  
  lastButtonState = buttonState;
}

void setup() {
  DEBUG_PORT.begin(115200);

  // Inicjalizacja guzika BOOT
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Inicjalizacja wyświetlacza
  initializeDisplay();
  
  // Wyświetl ekrany startowe
  showStartupScreens();

  // --- Proces połączenia z OBD-II ---
  showConnectionStatus("Podlaczanie do auta", ST77XX_BLUE);
  
  // Inicjalizacja połączenia Bluetooth
  if (!initializeBluetoothConnection()) {
    while (1); // Zatrzymaj program jeśli nie można połączyć z Bluetooth
  }
  
  // Inicjalizacja połączenia ELM327
  if (!initializeELM327Connection()) {
    while (1); // Zatrzymaj program jeśli nie można połączyć z ELM327
  }

  // Rysowanie początkowego układu na ekranie, przekazując odczytany typ paliwa
  drawLayout(SCREEN_WIDTH, SCREEN_HEIGHT, TOP_HEIGHT, topLeftW, topLeftH, topRightW, topRightH, 0xFF); // Na starcie pokaż "BENZYNA" lub "LPG" po pierwszym odczycie w pętli
}

void loop() {
  // Sprawdź guzik BOOT
  checkBootButton();
  
  if (debugMode) {
    // W trybie debug tylko aktualizuj logi, nie rysuj layoutu
    // Logi będą dodawane przez funkcje w elm327_connection.cpp
    displayDebugLog();
  } else {
    // Normalny tryb - odczytuj i wyświetlaj dane OBD
    readAndDisplayOBD();
  }
  
  delay(10);
}