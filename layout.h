#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// Deklaracja zewnętrznego obiektu TFT
extern Adafruit_ST7789 *tftPtr; // Używamy wskaźnika do obiektu Adafruit_ST7789

// Funkcja do ustawienia wskaźnika do obiektu TFT
void setDisplay(Adafruit_ST7789 *display);

// Stałe dla rozmiarów ikony strzałki
#define ARROW_W 8
#define ARROW_H 9

// Bitmapa dla strzałki (zielona)
extern const uint16_t arrow_rgb565[64];

/**
 * @brief Rysuje bitmapę strzałki na wyświetlaczu.
 * @param x Współrzędna X lewego górnego rogu bitmapy.
 * @param y Współrzędna Y lewego górnego rogu bitmapy.
 */
void drawExampleBitmap(int x, int y);

/**
 * @brief Rysuje wyśrodkowany tekst w określonym obszarze.
 * @param text Tekst do wyświetlenia.
 * @param x Współrzędna X lewego górnego rogu obszaru.
 * @param y Współrzędna Y lewego górnego rogu obszaru.
 * @param w Szerokość obszaru.
 * @param h Wysokość obszaru.
 * @param color Kolor tekstu.
 */
void drawCenteredText(const char* text, int x, int y, int w, int h, uint16_t color);

/**
 * @brief Rysuje wyśrodkowany tekst na całym ekranie (do komunikatów statusowych).
 * @param text Tekst do wyświetlenia.
 * @param color Kolor tekstu.
 */
void showCenteredStatusText(const String text, uint16_t color);

/**
 * @brief Rysuje sekcję zużycia paliwa.
 * @param value Wartość zużycia paliwa.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawFuelConsumption(float value, int x, int y, int w, int h);

/**
 * @brief Rysuje sekcję temperatury oleju.
 * @param value Wartość temperatury oleju.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawOilTemp(float value, int x, int y, int w, int h);

/**
 * @brief Rysuje sekcję temperatury płynu chłodniczego.
 * @param value Wartość temperatury płynu chłodniczego.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawCoolantTemp(float value, int x, int y, int w, int h);

/**
 * @brief Rysuje sekcję obrotów na minutę (RPM).
 * @param value Wartość RPM.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawRPM(int value, int x, int y, int w, int h);

/**
 * @brief Rysuje sekcję prędkości.
 * @param value Wartość prędkości.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawSpeed(int value, int x, int y, int w, int h);

/**
 * @brief Rysuje sekcję typu paliwa w oparciu o odczytany kod.
 * @param fuelCode Kod typu paliwa z OBD. Użyj 0xFF dla błędu/nieznanego stanu.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawFuelTypeSection(uint8_t fuelCode, int x, int y, int w, int h);

/**
 * @brief Rysuje ogólny układ ekranu.
 * @param screen_width Całkowita szerokość ekranu.
 * @param screen_height Całkowita wysokość ekranu.
 * @param top_height Wysokość górnej sekcji.
 * @param topLeftW Szerokość lewej górnej sekcji.
 * @param topLeftH Wysokość lewej górnej sekcji.
 * @param topRightW Szerokość prawej górnej sekcji.
 * @param topRightH Wysokość prawej górnej sekcji.
 * @param initialFuelTypeCode Początkowy kod typu paliwa do wyświetlenia. Użyj 0xFF dla błędu/nieznanego.
 */
void drawLayout(int screen_width, int screen_height, int top_height, int topLeftW, int topLeftH, int topRightW, int topRightH, uint8_t initialFuelTypeCode);

#endif