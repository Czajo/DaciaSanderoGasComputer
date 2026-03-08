#ifndef LAYOUT_H
#define LAYOUT_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// Deklaracja zewnętrznego obiektu TFT
extern Adafruit_ST7789 *tftPtr; // Używamy wskaźnika do obiektu Adafruit_ST7789

// Funkcja do ustawienia wskaźnika do obiektu TFT
void setDisplay(Adafruit_ST7789 *display) {
  tftPtr = display;
}

// Stałe dla rozmiarów ikony strzałki
#define ARROW_W 8
#define ARROW_H 9

// Bitmapa dla strzałki (zielona)
const uint16_t arrow_rgb565[64] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x07E0, 0x07E0, 0x07E0, 0x0000,
  0x0000, 0x0000, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x0000, 0x0000,
  0x0000, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x0000,
  0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0,
  0x0000, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x0000,
  0x0000, 0x0000, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x07E0, 0x07E0, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x07E0, 0x0000, 0x0000, 0x0000,
};

/**
 * @brief Rysuje bitmapę strzałki na wyświetlaczu.
 * @param x Współrzędna X lewego górnego rogu bitmapy.
 * @param y Współrzędna Y lewego górnego rogu bitmapy.
 */
void drawExampleBitmap(int x, int y) {
  if (tftPtr) {
    tftPtr->drawRGBBitmap(x, y, arrow_rgb565, ARROW_W, ARROW_H);
  }
}

/**
 * @brief Rysuje wyśrodkowany tekst w określonym obszarze.
 * @param text Tekst do wyświetlenia.
 * @param x Współrzędna X lewego górnego rogu obszaru.
 * @param y Współrzędna Y lewego górnego rogu obszaru.
 * @param w Szerokość obszaru.
 * @param h Wysokość obszaru.
 */
void drawCenteredText(const char* text, int x, int y, int w, int h) {
  if (tftPtr) {
    tftPtr->setTextColor(ST77XX_WHITE);
    tftPtr->setTextSize(1);
    int16_t x1, y1;
    uint16_t w1, h1;
    tftPtr->getTextBounds(text, 0, 0, &x1, &y1, &w1, &h1);
    int tx = x + (w - w1) / 2 - x1;
    int ty = y + (h - h1) / 2 - y1;
    tftPtr->setCursor(tx, ty);
    tftPtr->print(text);
  }
}

/**
 * @brief Rysuje wyśrodkowany tekst na całym ekranie (do komunikatów statusowych).
 */
void showCenteredStatusText(const String& text, uint16_t color) {
  if (tftPtr) {
    tftPtr->fillScreen(ST77XX_BLACK);
    tftPtr->setTextWrap(false);
    tftPtr->setTextSize(1); // Powrót do mniejszej czcionki (v3.9.1)
    tftPtr->setTextColor(color);

    int16_t x1, y1;
    uint16_t w, h;
    int screen_width = tftPtr->width();
    int screen_height = tftPtr->height();

    tftPtr->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    int16_t x = (screen_width - w) / 2 - x1;
    int16_t y = (screen_height - h) / 2 - y1;

    tftPtr->setCursor(x, y);
    tftPtr->print(text);
  }
}

// --- Ikony Proceduralne ---

void drawIconGasoline(int x, int y) {
  tftPtr->fillRect(x+2, y+4, 8, 10, ST77XX_ORANGE); 
  tftPtr->drawRect(x+10, y+6, 2, 6, ST77XX_ORANGE); 
}

void drawIconGasolineBig(int x, int y) {
  uint16_t color = ST77XX_ORANGE;
  tftPtr->fillRect(x+4, y+8, 14, 18, color);    // Korpus
  tftPtr->fillRect(x+6, y+10, 10, 6, ST77XX_BLACK); // Ekranik
  tftPtr->drawRect(x+18, y+10, 4, 12, color);   // Wąż
  tftPtr->fillRect(x+3, y+26, 16, 2, color);    // Podstawa
}

void drawIconLPG(int x, int y) {
  tftPtr->fillRoundRect(x+2, y+5, 8, 9, 3, ST77XX_GREEN); 
}

void drawIconLPGBig(int x, int y) {
  uint16_t color = ST77XX_GREEN;
  tftPtr->fillRoundRect(x+4, y+10, 16, 16, 6, color); // Korpus butli
  tftPtr->fillRect(x+8, y+4, 8, 6, color);           // Górna obejma
  tftPtr->fillCircle(x+12, y+7, 2, ST77XX_BLACK);    // Zawór
}

void drawIconGasolineHuge(int x, int y) {
  uint16_t color = ST77XX_ORANGE;
  tftPtr->fillRect(x+6, y+10, 32, 42, color);    // Korpus
  tftPtr->fillRect(x+10, y+14, 24, 15, ST77XX_BLACK); // Ekranik
  tftPtr->drawRect(x+38, y+14, 8, 28, color);   // Wąż
  tftPtr->fillRect(x+4, y+52, 36, 4, color);    // Podstawa
}

void drawIconLPGHuge(int x, int y) {
  uint16_t color = ST77XX_GREEN;
  tftPtr->fillRoundRect(x+6, y+18, 40, 38, 14, color); // Korpus butli
  tftPtr->fillRect(x+16, y+6, 20, 14, color);           // Górna obejma
  tftPtr->fillCircle(x+26, y+13, 4, ST77XX_BLACK);    // Zawór
}

void drawIconOil(int x, int y) {
  tftPtr->fillCircle(x+6, y+10, 3, ST77XX_YELLOW); 
  tftPtr->fillTriangle(x+3, y+10, x+9, y+10, x+6, y+4, ST77XX_YELLOW); 
}

void drawIconCoolant(int x, int y) {
  tftPtr->drawRect(x+5, y+2, 2, 10, ST77XX_CYAN); 
  tftPtr->fillCircle(x+6, y+12, 3, ST77XX_CYAN);  
}

void drawIconRoad(int x, int y) {
  tftPtr->drawLine(x+2, y+14, x+5, y+2, ST77XX_WHITE);  
  tftPtr->drawLine(x+10, y+14, x+7, y+2, ST77XX_WHITE); 
  tftPtr->drawFastVLine(x+6, y+4, 2, 0xAD55);           
  tftPtr->drawFastVLine(x+6, y+10, 2, 0xAD55);
}

/**
 * @brief Rysuje zaawansowany ekran INFO (SIDE-BY-SIDE) (v4.4).
 */
void drawInfoPage(float tripDist, float tripGas, float tripLpg, float tankDist, float tankGas, float tankLpg, uint32_t logs) {
  if (!tftPtr) return;
  tftPtr->fillScreen(ST77XX_BLACK);
  
  // Naglowek
  tftPtr->setTextSize(1);
  tftPtr->setTextColor(ST77XX_CYAN);
  tftPtr->setCursor(120, 15);
  tftPtr->print("STATYSTYKI");
  tftPtr->drawFastHLine(0, 32, 320, 0x3186);
  
  tftPtr->setTextSize(1);
  // SEKCJA TRASA
  tftPtr->setTextColor(0xAD55);
  tftPtr->setCursor(20, 45);
  tftPtr->print("TRASA: ");
  tftPtr->print(tripDist, 1); tftPtr->print(" km total");
  
  tftPtr->setTextColor(ST77XX_ORANGE);
  tftPtr->setCursor(40, 65);
  tftPtr->print("[B] "); tftPtr->print(tripGas, 1); tftPtr->print(" km");
  
  tftPtr->setTextColor(ST77XX_GREEN);
  tftPtr->setCursor(180, 65);
  tftPtr->print("[L] "); tftPtr->print(tripLpg, 1); tftPtr->print(" km");

  // SEKCJA TANKOWANIE
  tftPtr->setTextColor(0xAD55);
  tftPtr->setCursor(20, 95);
  tftPtr->print("OD RESETU TANKOWANIA: ");
  tftPtr->print(tankDist, 1); tftPtr->print(" km");
  
  tftPtr->setTextColor(ST77XX_ORANGE);
  tftPtr->setCursor(40, 115);
  tftPtr->print("[B] "); tftPtr->print(tankGas, 1); tftPtr->print(" km");
  
  tftPtr->setTextColor(ST77XX_GREEN);
  tftPtr->setCursor(180, 115);
  tftPtr->print("[L] "); tftPtr->print(tankLpg, 1); tftPtr->print(" km");
  
  // Stopka INFO (Logi)
  tftPtr->drawFastHLine(0, 145, 320, 0x3186);
  tftPtr->setTextColor(0x7BEF);
  
  String logsText = "ZAPISANYCH LOGOW: " + String(logs);
  int16_t x1, y1;
  uint16_t w1, h1;
  tftPtr->getTextBounds(logsText, 0, 0, &x1, &y1, &w1, &h1);
  
  tftPtr->setCursor((320 - w1) / 2, 155);
  tftPtr->print(logsText);
}

/**
 * @brief Rysuje nowoczesną kartę statystyk z ikoną.
 */
void drawStatCard(const char* label, float value, const char* unit, uint16_t color, int x, int y, int w, int h, int iconType, uint8_t fuelCode) {
  if (!tftPtr) return;
  
  // Tło i ramka karty
  tftPtr->fillRoundRect(x+2, y+2, w-4, h-4, 4, 0x0841); 
  tftPtr->drawRoundRect(x+2, y+2, w-4, h-4, 4, 0x3186); 
  
  // Etykieta karty (v4.3.2) - Czarny skrot, przesuniety glebiej
  tftPtr->setTextSize(1);
  tftPtr->setTextColor(ST77XX_BLACK); 
  tftPtr->setCursor(x + w - 28, y + 16);
  if (iconType == 1) tftPtr->print("TK");      // Tankowanie (TK)
  else if (iconType == 2) tftPtr->print("TR"); // Trasa (TR)
  else if (iconType == 3) tftPtr->print("OL"); // Olej (OL)
  else if (iconType == 4) tftPtr->print("PL"); // Plyn (PL)

  // Ikona (Przesunięta na środek lewej krawędzi)
  int iconX = x + 10;
  int iconY = y + (h / 2) - 8;
  if (iconType == 1) {
    if (fuelCode == 0x05 || fuelCode == 0x0C) drawIconLPG(iconX, iconY);
    else drawIconGasoline(iconX, iconY);
  }
  else if (iconType == 2) drawIconRoad(iconX, iconY);
  else if (iconType == 3) drawIconOil(iconX, iconY);
  else if (iconType == 4) drawIconCoolant(iconX, iconY);

  // Wartość - POWIĘKSZONA (v4.0)
  tftPtr->setTextSize(2);
  tftPtr->setTextColor(color);
  String valStr = String(value, (value < 100 && value > -10) ? 1 : 0);
  
  int16_t x1, y1;
  uint16_t w1, h1;
  tftPtr->getTextBounds(valStr, 0, 0, &x1, &y1, &w1, &h1);
  
  // Wyśrodkowanie wartości z uwzględnieniem ikony
  int valX = x + 40; // Przesunięcie od lewej (ikona zajmuje ok 30px)
  int valY = y + (h - h1) / 2 - y1;
  tftPtr->setCursor(valX, valY);
  tftPtr->print(valStr);
  
  // Jednostka - Mała (Size 1) obok wartości
  tftPtr->setTextSize(1);
  tftPtr->setTextColor(0x7BEF);
  tftPtr->setCursor(valX + w1 + 4, valY + 6);
  tftPtr->print(unit);
}

/**
 * @brief Rysuje główną sekcję chwilową (duża karta na górze).
 */
void drawInstantCard(float instVal, const char* unit, uint8_t fuelCode, int x, int y, int w, int h) {
  uint16_t color = (fuelCode == 0x05 || fuelCode == 0x0C) ? ST77XX_GREEN : ST77XX_ORANGE;

  tftPtr->fillRoundRect(x+2, y+2, w-4, h-4, 6, 0x0000);
  tftPtr->drawRoundRect(x+2, y+2, w-4, h-4, 6, color);  
  
  // GIGANTYCZNE IKONY (v4.5) - Na wysokosc calego kafelka
  if (fuelCode == 0x05 || fuelCode == 0x0C) {
    drawIconLPGHuge(x + 15, y + 2);
  } else {
    drawIconGasolineHuge(x + 15, y + 2);
  }

  tftPtr->setTextSize(3);
  tftPtr->setTextColor(color);
  String valStr = String(instVal, 1);
  int16_t x1, y1;
  uint16_t w1, h1;
  tftPtr->getTextBounds(valStr, 0, 0, &x1, &y1, &w1, &h1);
  
  // Przesunięcie wartości w prawo, aby zrobić miejsce dla gigantycznej ikony
  int valX = x + 85 + (w - 85 - w1) / 2;
  tftPtr->setCursor(valX, y + 42); 
  tftPtr->print(valStr);

  tftPtr->setTextSize(1);
  tftPtr->setTextColor(ST77XX_WHITE);
  tftPtr->setCursor(valX + w1 + 6, y + 50); // Jednostka tuż za wartością
  tftPtr->print(unit);
}

/**
 * @brief Rysuje ogólny układ ekranu głównego (Siatka 2-kolumnowa).
 */
void drawMainLayout(int SCREEN_WIDTH, int SCREEN_HEIGHT, int TOP_HEIGHT, int topLeftW, int topLeftH, int topRightW, int topRightH, uint8_t initialFuelTypeCode) {
  if (tftPtr) {
    tftPtr->fillScreen(ST77XX_BLACK);
  }
}

/**
 * @brief Główna funkcja aktualizująca dane na nowym układzie.
 */
void updateTripDisplay(float instLH, float inst100, float avgTrip, float avgTank, 
                       float oilTemp, float coolTemp, uint8_t fuelCode) {
  if (!tftPtr) return;
  
  // 1. Sekcja górna (Live)
  if (inst100 > 0.1 && inst100 < 99) {
    drawInstantCard(inst100, "L/100", fuelCode, 0, 0, 320, 65);
  } else {
    drawInstantCard(instLH, "L/h", fuelCode, 0, 0, 320, 65);
  }
  
  int cardW = 160;
  int cardH = 48;
  int startY = 68;

  // 2. Sekcja dolna - Moduły
  drawStatCard("AVG TRASA", avgTrip, "L/100", ST77XX_WHITE, 0, startY, cardW, cardH, 2, fuelCode);
  drawStatCard("AVG TANK", avgTank, "L/100", ST77XX_CYAN, cardW, startY, cardW, cardH, 1, fuelCode);
  
  drawStatCard("TEMP OLEJU", oilTemp, "C", ST77XX_YELLOW, 0, startY + cardH + 2, cardW, cardH, 3, fuelCode);
  drawStatCard("TEMP PLYNU", coolTemp, "C", ST77XX_CYAN, cardW, startY + cardH + 2, cardW, cardH, 4, fuelCode);
}

#endif