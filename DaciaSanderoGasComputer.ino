#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>
#include "BluetoothSerial.h"
#include "ELMduino.h"
#include "layout.h" // Twój plik layout.h
#include "logo.h"
#include "sandero.h"

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
// Szerokość lewej górnej sekcji (75% szerokości ekranu)
int topLeftW = (SCREEN_WIDTH * 3) / 4;
// Szerokość prawej górnej sekcji
int topRightW = SCREEN_WIDTH - topLeftW;
// Wysokość lewej górnej sekcji
int topLeftH = TOP_HEIGHT;
// Wysokość prawej górnej sekcji
int topRightH = TOP_HEIGHT;

// Obiekt TFT
Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Obiekty Bluetooth i ELM327
BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

// Pamiętaj, ABY ZMIENIĆ TEN ADRES MAC NA ADRES SWOJEGO URZĄDZENIA OBD-II!
uint8_t elm327_address[6] = { 0xDC, 0x0D, 0x30, 0x9F, 0x96, 0x0E };

ELM327 myELM327;

// Zmienne do przechowywania odczytów z OBD
uint32_t current_rpm = 0;
uint8_t current_speed = 0;
float current_oil_temp = 0.0;
float current_coolant_temp = 0.0;
float current_maf = 0.0;
float current_fuel_consumption = 0.0; // W L/100km
uint8_t current_fuel_type_code = 0xFF; // Globalna zmienna, zainicjalizowana wartością błędu

// Stałe do obliczeń spalania
const float AIR_FUEL_RATIO_GASOLINE = 14.7;     // Stosunek stechiometryczny powietrze/paliwo dla benzyny
const float GASOLINE_DENSITY_G_PER_L = 710.0;   // Gęstość benzyny w g/L (około)

// Kolory
#define COLOR_HEADER ST77XX_CYAN
#define COLOR_VALUE ST77XX_WHITE
#define COLOR_ERROR ST77XX_RED

// Definicja stanów odczytu OBD
enum OBD_STATE {
  ENG_RPM,
  SPEED,
  OIL_TEMP,
  COOLANT_TEMP,
  FUEL_CONS,
  FUEL_TYPE_UPDATE // Zmieniona nazwa, aby uniknąć konfliktu, i by było jasne, że to aktualizacja wyświetlania
};

// Zmienna stanu
OBD_STATE obd_state = ENG_RPM;

// --- Funkcja do obliczania spalania ---
void calculateFuelConsumptionValue(float maf, uint8_t speed) {
  if (speed > 0 && maf > 0) {
    float fuel_g_per_s = maf / AIR_FUEL_RATIO_GASOLINE;
    float fuel_L_per_s = fuel_g_per_s / GASOLINE_DENSITY_G_PER_L;
    float speed_km_per_s = speed / 3600.0;
    float fuel_L_per_km = fuel_L_per_s / speed_km_per_s;
    current_fuel_consumption = fuel_L_per_km * 100.0;

    DEBUG_PORT.print("Spalanie (estymowane): ");
    DEBUG_PORT.print(current_fuel_consumption, 2);
    DEBUG_PORT.println(" L/100km");
  } else {
    current_fuel_consumption = 0.0;
    DEBUG_PORT.println("Samochód stoi lub brak danych MAF, nie można obliczyć spalania.");
  }
}

// --- Główna funkcja readAndDisplayOBD() ---
void readAndDisplayOBD() {
  switch (obd_state) {
    case ENG_RPM:
      {
        float tempRPM = myELM327.rpm();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_rpm = (uint32_t)tempRPM;
          DEBUG_PORT.print("RPM: ");
          DEBUG_PORT.println(current_rpm);
          drawRPM(current_rpm, 0, TOP_HEIGHT + (SCREEN_HEIGHT - TOP_HEIGHT) / 2, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          obd_state = SPEED;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          current_rpm = 0;
          obd_state = SPEED;
        }
        break;
      }

    case SPEED:
      {
        float tempSpeed = myELM327.kph();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_speed = (uint8_t)tempSpeed;
          DEBUG_PORT.print("Speed: ");
          DEBUG_PORT.print(current_speed);
          DEBUG_PORT.println(" km/h");
          drawSpeed(current_speed, SCREEN_WIDTH / 2, TOP_HEIGHT + (SCREEN_HEIGHT - TOP_HEIGHT) / 2, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          obd_state = OIL_TEMP;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          current_speed = 0;
          obd_state = OIL_TEMP;
        }
        break;
      }

    case OIL_TEMP:
      {
        float tempOil = myELM327.oilTemp();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_oil_temp = tempOil;
          DEBUG_PORT.print("Oil Temp: ");
          DEBUG_PORT.print(current_oil_temp);
          DEBUG_PORT.println(" C");
          drawOilTemp(current_oil_temp, 0, TOP_HEIGHT, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          obd_state = COOLANT_TEMP;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          current_oil_temp = -1;
          obd_state = COOLANT_TEMP;
        }
        break;
      }

    case COOLANT_TEMP:
      {
        float tempCoolant = myELM327.engineCoolantTemp();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_coolant_temp = tempCoolant;
          DEBUG_PORT.print("Coolant Temp: ");
          DEBUG_PORT.print(current_coolant_temp);
          DEBUG_PORT.println(" C");
          drawCoolantTemp(current_coolant_temp, SCREEN_WIDTH / 2, TOP_HEIGHT, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          obd_state = FUEL_CONS;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          current_coolant_temp = -1;
          obd_state = FUEL_CONS;
        }
        break;
      }

    case FUEL_CONS:
      {
        current_maf = myELM327.mafRate();

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          DEBUG_PORT.print("MAF: ");
          DEBUG_PORT.print(current_maf);
          DEBUG_PORT.println(" g/s");

          calculateFuelConsumptionValue(current_maf, current_speed);

          DEBUG_PORT.print("Fuel Consumption: ");
          DEBUG_PORT.print(current_fuel_consumption, 1);
          DEBUG_PORT.println(" L/100km");

          drawFuelConsumption(current_fuel_consumption, 0, 0, topLeftW, topLeftH);
          obd_state = FUEL_TYPE_UPDATE;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          DEBUG_PORT.println("Błąd odczytu MAF. Spalanie ustawione na 0.");
          current_maf = 0.0;
          current_fuel_consumption = 0.0;
          drawFuelConsumption(current_fuel_consumption, 0, 0, topLeftW, topLeftH);
          obd_state = FUEL_TYPE_UPDATE;
        }
        break;
      }

    case FUEL_TYPE_UPDATE: // Odpowiedzialność za odczyt i przekazanie kodu do rysowania
      {
        uint8_t tempFuelTypeCode = myELM327.fuelType(); // Odczytaj wartość z OBD

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_fuel_type_code = tempFuelTypeCode; // Zapisz odczytaną wartość
          DEBUG_PORT.print("Fuel Type Code: ");
          DEBUG_PORT.println(current_fuel_type_code, HEX); // Wyświetl kod w HEX dla łatwiejszego debugowania
        } else { // W przypadku błędu odczytu (różne od ELM_GETTING_MSG)
          myELM327.printError();
          DEBUG_PORT.println("Błąd odczytu typu paliwa.");
          // Ustaw current_fuel_type_code na specjalną wartość błędu (np. 0xFF),
          // aby drawFuelTypeSection mogła to obsłużyć.
          current_fuel_type_code = 0xFF;
        }

        // Zawsze wywołaj funkcję rysującą z aktualnym kodem lub kodem błędu
        drawFuelTypeSection(current_fuel_type_code, topLeftW, 0, topRightW, topRightH);
        obd_state = ENG_RPM; // Powrót na początek cyklu
        break;
      }
  }
}

// Funkcje pomocnicze do generowania losowych danych (możesz je usunąć, jeśli już ich nie używasz)
float randFloat0to25() {
  return ((float)rand() / (float)RAND_MAX) * 25.0;
}

int randInt(int min, int max) {
  return min + rand() % (max - min + 1);
}

void setup() {
  DEBUG_PORT.begin(115200);

  tft.init(170, 320);
  tft.setRotation(3);
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH);
  tft.setFont(&FreeSans9pt7b);

  setDisplay(&tft); // Ustawienie wskaźnika do obiektu TFT w display.h

  tft.drawRGBBitmap(0, 0, logo, 320, 170);
  delay(3000);
  tft.drawRGBBitmap(0, 0, bootlogo, 320, 170);
  delay(3000);

  // --- Proces połączenia z OBD-II ---
  showCenteredStatusText("Podlaczanie do auta", ST77XX_BLUE);
  SerialBT.setPin("1234", 4);
  ELM_PORT.begin("Czajo_x5pro", true);

  DEBUG_PORT.println("Connecting to OBD scanner via Bluetooth...");
  if (!ELM_PORT.connect(elm327_address)) {
    DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1 (Bluetooth)");
    showCenteredStatusText("Brak polaczenia z BT", ST77XX_RED);
    while (1);
  }
  DEBUG_PORT.println("Bluetooth connected.");
  showCenteredStatusText("Polaczono z BT", ST77XX_YELLOW);
  delay(1000);

  DEBUG_PORT.println("Initializing ELM327...");
  if (!myELM327.begin(ELM_PORT, false, 2000)) {
    DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 2 (ELM327)");
    showCenteredStatusText("Brak polaczenia z OBD", ST77XX_RED);
    while (1);
  }
  DEBUG_PORT.println("Connected to ELM327");
  showCenteredStatusText("Polaczono z OBD", ST77XX_GREEN);
  delay(1000);

  // Odczytaj typ paliwa raz po połączeniu
  uint8_t initialReadFuelTypeCode = myELM327.fuelType();
  if (myELM327.nb_rx_state == ELM_SUCCESS) {
    current_fuel_type_code = initialReadFuelTypeCode;
    DEBUG_PORT.print("Initial Fuel Type Code: ");
    DEBUG_PORT.println(current_fuel_type_code, HEX); // Wyświetl w HEX
  } else {
    myELM327.printError();
    DEBUG_PORT.println("Błąd początkowego odczytu typu paliwa.");
    current_fuel_type_code = 0xFF; // Ustaw 0xFF dla błędu
  }

  // Rysowanie początkowego układu na ekranie, przekazując odczytany typ paliwa
  drawLayout(SCREEN_WIDTH, SCREEN_HEIGHT, TOP_HEIGHT, topLeftW, topLeftH, topRightW, topRightH, current_fuel_type_code);
}

void loop() {
  readAndDisplayOBD();
  delay(50);
}