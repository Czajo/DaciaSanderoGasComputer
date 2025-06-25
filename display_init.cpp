#include "display_init.h"
#include "layout.h"
#include "logo.h"
#include "sandero.h"

// Zmienne ekranu
int topLeftW, topLeftH, topRightW, topRightH;
Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Funkcja do obliczania wymiarów sekcji ekranu
void calculateScreenDimensions() {
  // Szerokość lewej górnej sekcji (65% szerokości ekranu)
  topLeftW = (SCREEN_WIDTH * 13) / 20;
  // Szerokość prawej górnej sekcji
  topRightW = SCREEN_WIDTH - topLeftW;
  // Wysokość lewej górnej sekcji
  topLeftH = TOP_HEIGHT;
  // Wysokość prawej górnej sekcji
  topRightH = TOP_HEIGHT;
}

// Funkcja inicjalizacji wyświetlacza
void initializeDisplay() {
  tft.init(170, 320);
  tft.setRotation(3);
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH);
  tft.setFont(&FreeSans9pt7b);

  setDisplay(&tft); // Ustawienie wskaźnika do obiektu TFT w layout.h
  
  // Oblicz wymiary sekcji ekranu
  calculateScreenDimensions();
}

// Funkcja wyświetlania ekranów startowych
void showStartupScreens() {
  // Wyświetl logo
  tft.drawRGBBitmap(0, 0, logo, 320, 170);
  delay(3000);
  
  // Wyświetl bootlogo
  tft.drawRGBBitmap(0, 0, bootlogo, 320, 170);
  delay(3000);
} 