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


void setup() {
  DEBUG_PORT.begin(115200);

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
  readAndDisplayOBD();
  delay(10);
}