#include "display_init.h"
#include "layout.h"
#include "logo.h"
#include "sandero.h"

// Zmienne ekranu
int topLeftW, topLeftH, topRightW, topRightH;

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
  // Nie ustawiaj tftPtr na nullptr!
  calculateScreenDimensions();
}

// Funkcja wyświetlania ekranów startowych
void showStartupScreens() {
  if (!tftPtr) return;
  tftPtr->drawRGBBitmap(0, 0, logo, 320, 170);
  delay(3000);
  tftPtr->drawRGBBitmap(0, 0, bootlogo, 320, 170);
  delay(3000);
} 