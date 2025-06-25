#ifndef DISPLAY_INIT_H
#define DISPLAY_INIT_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>

// Piny ekranu
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32

// Szerokość i wysokość ekranu
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 170

// Wysokość górnej sekcji (30% wysokości ekranu)
#define TOP_HEIGHT (SCREEN_HEIGHT * 30 / 100)

// Deklaracje zmiennych ekranu
extern int topLeftW, topLeftH, topRightW, topRightH;
extern Adafruit_ST7789 tft;

// Funkcje inicjalizacji ekranu
void initializeDisplay();
void showStartupScreens();
void calculateScreenDimensions();

#endif // DISPLAY_INIT_H 