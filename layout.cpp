#include "layout.h"

// Deklaracja wskaźnika do obiektu Adafruit_ST7789
Adafruit_ST7789 *tftPtr;

// Funkcja do ustawienia wskaźnika do obiektu TFT
void setDisplay(Adafruit_ST7789 *display) {
  tftPtr = display;
}

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
 * @param color Kolor tekstu.
 */
void drawCenteredText(const char* text, int x, int y, int w, int h, uint16_t color) {
  if (tftPtr) {
    tftPtr->setTextColor(color);
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
 * @param text Tekst do wyświetlenia.
 * @param color Kolor tekstu.
 */
void showCenteredStatusText(const String text, uint16_t color) {
  if (tftPtr) {
    tftPtr->fillScreen(ST77XX_BLACK);
    tftPtr->setTextWrap(false);
    tftPtr->setTextSize(1); // Większy rozmiar tekstu dla statusu
    tftPtr->setTextColor(color);

    int16_t x1, y1;
    uint16_t w, h;
    // TFT_WIDTH i TFT_HEIGHT nie są globalne w display.h, więc musimy je przekazać lub założyć
    // Tutaj zakładamy, że SCREEN_WIDTH i SCREEN_HEIGHT są zdefiniowane w main.ino
    // i używamy ich jako domyślnych rozmiarów ekranu
    int screen_width_local = 320; // Możesz dostosować, jeśli znasz dokładny rozmiar ekranu
    int screen_height_local = 170; // Możesz dostosować, jeśli znasz dokładny rozmiar ekranu

    tftPtr->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    int16_t x = (screen_width_local - w) / 2 - x1;
    int16_t y = (screen_height_local - h) / 2 - y1;

    tftPtr->setCursor(x, y);
    tftPtr->print(text);
  }
}

/**
 * @brief Rysuje sekcję zużycia paliwa.
 * @param value Wartość zużycia paliwa.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawFuelConsumption(float value, int x, int y, int w, int h) {
  if (tftPtr) {
    tftPtr->setTextColor(ST77XX_GREEN);
    tftPtr->fillRect(x, y, w, h, ST77XX_BLACK); // Wypełnienie tła czernią

    tftPtr->setTextSize(2);
    String valStr = String(value, 1); // Wartość z jednym miejscem po przecinku
    int startX = x + 10;
    int startY = y + (h / 2) + 12; // Pozycja Y dla tekstu

    tftPtr->setCursor(startX, startY);
    tftPtr->print(valStr);

    int16_t bx, by;
    uint16_t bw, bh;
    tftPtr->getTextBounds(valStr, startX, startY, &bx, &by, &bw, &bh); // Pobranie wymiarów tekstu

    tftPtr->setTextSize(1);
    int unitX = bx + bw + 5; // Pozycja X dla jednostki
    int unitY = startY - (bh / 2); // Pozycja Y dla jednostki

    tftPtr->setCursor(unitX, unitY);
    tftPtr->setTextColor(ST77XX_WHITE);
    tftPtr->print("L/100km");
  }
}

/**
 * @brief Rysuje sekcję temperatury oleju.
 * @param value Wartość temperatury oleju.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawOilTemp(float value, int x, int y, int w, int h) {
  if (tftPtr) {
    tftPtr->fillRect(x, y, w, h, ST77XX_BLACK);
    tftPtr->setTextColor(ST77XX_YELLOW);
    drawCenteredText("temp oleju", x, y, w, h / 2, ST77XX_YELLOW); // Rysowanie nagłówka
    char buf[10];
    snprintf(buf, sizeof(buf), "%.1f °C", value); // Formatowanie wartości
    tftPtr->setCursor(x + (w - strlen(buf) * 6) / 2, y + h * 2 / 3); // Wyśrodkowanie wartości
    tftPtr->print(buf);
  }
}

/**
 * @brief Rysuje sekcję temperatury płynu chłodniczego.
 * @param value Wartość temperatury płynu chłodniczego.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawCoolantTemp(float value, int x, int y, int w, int h) {
  if (tftPtr) {
    tftPtr->fillRect(x, y, w, h, ST77XX_BLACK);
    tftPtr->setTextColor(ST77XX_CYAN);
    drawCenteredText("temp coolant", x, y, w, h / 2, ST77XX_CYAN);
    char buf[10];
    snprintf(buf, sizeof(buf), "%.1f °C", value);
    tftPtr->setCursor(x + (w - strlen(buf) * 6) / 2, y + h * 2 / 3);
    tftPtr->print(buf);
  }
}

/**
 * @brief Rysuje sekcję obrotów na minutę (RPM).
 * @param value Wartość RPM.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawRPM(int value, int x, int y, int w, int h) {
  if (tftPtr) {
    tftPtr->fillRect(x, y, w, h, ST77XX_BLACK);
    tftPtr->setTextColor(ST77XX_MAGENTA);
    drawCenteredText("RPM", x, y, w, h / 2, ST77XX_MAGENTA);
    char buf[10];
    snprintf(buf, sizeof(buf), "%d", value);
    tftPtr->setCursor(x + (w - strlen(buf) * 6) / 2, y + h * 2 / 3);
    tftPtr->print(buf);
  }
}

/**
 * @brief Rysuje sekcję prędkości.
 * @param value Wartość prędkości.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawSpeed(int value, int x, int y, int w, int h) {
  if (tftPtr) {
    tftPtr->fillRect(x, y, w, h, ST77XX_BLACK);
    tftPtr->setTextColor(ST77XX_ORANGE);
    drawCenteredText("SPEED", x, y, w, h / 2, ST77XX_ORANGE);
    char buf[10];
    snprintf(buf, sizeof(buf), "%d km/h", value);
    tftPtr->setCursor(x + (w - strlen(buf) * 6) / 2, y + h * 2 / 3);
    tftPtr->print(buf);
  }
}

/**
 * @brief Rysuje sekcję typu paliwa w oparciu o odczytany kod.
 * @param fuelCode Kod typu paliwa z OBD. Użyj 0xFF dla błędu/nieznanego stanu.
 * @param x Współrzędna X lewego górnego rogu sekcji.
 * @param y Współrzędna Y lewego górnego rogu sekcji.
 * @param w Szerokość sekcji.
 * @param h Wysokość sekcji.
 */
void drawFuelTypeSection(uint8_t fuelCode, int x, int y, int w, int h) {
  if (!tftPtr) return;

  // Wyświetlamy bezpośrednio odczytaną wartość fuelType
  char fuelText[20];
  uint16_t bgColor;
  uint16_t textColor = ST77XX_BLACK;

  if (fuelCode == 0xFF) {
    // Błąd odczytu lub nieznany stan
    snprintf(fuelText, sizeof(fuelText), "FUEL: ERR");
    bgColor = ST77XX_RED;
  } else {
    // Wyświetlamy kod w formacie hex
    snprintf(fuelText, sizeof(fuelText), "FUEL: 0x%02X", fuelCode);
    
    // Kolor tła w zależności od typu paliwa
    if (fuelCode == 0x05 || fuelCode == 0x0C) {
      bgColor = ST77XX_GREEN; // LPG
    } else if (fuelCode == 0x01) {
      bgColor = ST77XX_ORANGE; // Benzyna
    } else {
      bgColor = ST77XX_BLUE; // Inne typy paliwa
    }
  }

  tftPtr->fillRect(x, y, w, h, bgColor);
  tftPtr->setTextColor(textColor);
  
  // Zwiększamy rozmiar czcionki dla lepszego wykorzystania przestrzeni
  tftPtr->setTextSize(2);
  
  // Ręczne wyśrodkowanie tekstu z większą czcionką
  int16_t x1, y1;
  uint16_t textW, textH;
  tftPtr->getTextBounds(fuelText, 0, 0, &x1, &y1, &textW, &textH);
  
  int textX = x + (w - textW) / 2 - x1;
  int textY = y + (h - textH) / 2 - y1;
  
  tftPtr->setCursor(textX, textY);
  tftPtr->print(fuelText);
  
  // Przywracamy domyślny rozmiar czcionki
  tftPtr->setTextSize(1);
}

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
void drawLayout(int screen_width, int screen_height, int top_height, int topLeftW, int topLeftH, int topRightW, int topRightH, uint8_t initialFuelTypeCode) {
  if (tftPtr) {
    tftPtr->fillScreen(ST77XX_BLACK); // Wyczyść ekran

    // Górna lewa sekcja "Spalanie"
    tftPtr->fillRect(0, 0, topLeftW, topLeftH, ST77XX_BLACK);

    // Górna prawa sekcja na typ paliwa
    drawFuelTypeSection(initialFuelTypeCode, topLeftW, 0, topRightW, topRightH);
  }
} 