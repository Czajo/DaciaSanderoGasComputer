
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
Adafruit_ST7789 *tftPtr = nullptr; // Deklaracja globalna wskaźnika z pliku layout.h

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
uint8_t current_map = 0;       // Manifold Absolute Pressure (kPa)
float current_iat = 0.0;       // Intake Air Temp (C)
float current_maf_estimate = 0.0; // Obliczony w g/s we wzorze Speed-Density
float current_fuel_consumption = 0.0; // W L/100km
uint8_t current_fuel_type_code = 0xFF; // Globalna zmienna, zainicjalizowana wartością błędu

// Zarządzanie przewijaniem ekranów (Ekran Główny / Ekran Debug)
unsigned long last_screen_switch_time = 0;
bool is_debug_screen = false; // false = Ekran Główny, true = Ekran Debug

// Stałe do obliczeń spalania
const float AIR_FUEL_RATIO_GASOLINE = 14.7;     // Stosunek stechiometryczny powietrze/paliwo dla benzyny
const float GASOLINE_DENSITY_G_PER_L = 710.0;   // Gęstość benzyny w g/L (około)

// Kolory
#define COLOR_HEADER ST77XX_CYAN
#define COLOR_VALUE ST77XX_WHITE
#define COLOR_ERROR ST77XX_RED

// KOREKTA DLA DACIA 1.0 TCe (Sensor MAP)
// Auta te zwracają przez OBD w trybie 1, bajt A bardzo małą wartość (np. 11 przy jeździe). 
// Może to być ciśnienie raportowane w decyBarach / kPa/10 (lub ew. PSI). Używamy mnożnika x10
// lub x6.89 docelowo, by we wzorze znalazło się np. 27 albo 110 kPa.
const float MAP_MULTIPLIER = 10.0;

// Wartości silnika 1.0 TCe (Dacia H4D)
const float ENGINE_DISPLACEMENT_L = 0.999;     // Pojemność silnika w litrach
const float ENGINE_VE = 85.0;                  // Volumetric Efficiency (VE) w % (do kalibracji, np. 80-90%)

// Definicja stanów odczytu OBD
enum OBD_STATE {
  ENG_RPM,
  SPEED,
  OIL_TEMP,
  COOLANT_TEMP,
  MAP_PRESS,
  IAT_TEMP,
  FUEL_CONS,
  FUEL_TYPE_UPDATE
};

// Stałe do obliczeń spalania zostały już zdefiniowane wyżej (linie 58-59)

const float AIR_FUEL_RATIO_LPG = 15.5;          // Stosunek stechiometryczny powietrze/paliwo (LPG)
const float LPG_DENSITY_G_PER_L = 550.0;        // Gęstość LPG w g/L (około)

// Zmienna stanu
OBD_STATE obd_state = ENG_RPM;

// --- Funkcja do obliczania spalania metodą Speed-Density ---
void calculateFuelConsumptionValue(uint32_t rpm, uint8_t map_press, float iat_temp, uint8_t speed) {
  if (speed > 0 && rpm > 0 && map_press > 0) {
    
    // Obliczenie MAF (g/s) ze wzoru Speed-Density (Ideal Gas Law)
    // MAF = (RPM * MAP / (IAT + 273.15)) * (Displacement * VE / 120) * (MolarMassAir / R)
    // MolarMassAir / R = 28.97 / 8.314 ~= 3.484
    float imap = (float)rpm * (float)map_press / (iat_temp + 273.15);
    current_maf_estimate = (imap / 120.0) * (ENGINE_VE / 100.0) * ENGINE_DISPLACEMENT_L * 3.484;

    float afr = AIR_FUEL_RATIO_GASOLINE;
    float density = GASOLINE_DENSITY_G_PER_L;

    // Sprawdzamy czy aktualnie jeździsz na LPG (0x05 lub 0x0C)
    bool isLPG = (current_fuel_type_code == 0x05 || current_fuel_type_code == 0x0C);
    
    if (isLPG) {
      afr = AIR_FUEL_RATIO_LPG;
      density = LPG_DENSITY_G_PER_L;
    }

    float fuel_g_per_s = current_maf_estimate / afr;
    float fuel_L_per_s = fuel_g_per_s / density;
    float speed_km_per_s = speed / 3600.0;
    float fuel_L_per_km = fuel_L_per_s / speed_km_per_s;
    current_fuel_consumption = fuel_L_per_km * 100.0;

    DEBUG_PORT.print("MAF Estymowany (g/s): ");
    DEBUG_PORT.println(current_maf_estimate, 2);
    DEBUG_PORT.print("Spalanie (estymowane, ");
    DEBUG_PORT.print(isLPG ? "LPG" : "BENZYNA");
    DEBUG_PORT.print("): ");
    DEBUG_PORT.print(current_fuel_consumption, 2);
    DEBUG_PORT.println(" L/100km");
  } else {
    current_fuel_consumption = 0.0;
    DEBUG_PORT.println("Samochód stoi lub brak danych z silnika, stan = 0.");
  }
}

// --- Główna funkcja readAndDisplayOBD() ---
void readAndDisplayOBD() {
  // Dodaj logowanie stanu
  DEBUG_PORT.print("Aktualny stan: ");
  switch (obd_state) {
    case ENG_RPM:
      DEBUG_PORT.println("ENG_RPM");
      {
        float tempRPM = myELM327.rpm();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          if (tempRPM < 100) {
            current_rpm = 0; 
            DEBUG_PORT.println("RPM zbyt niskie, ustawiam 0");
          } else {
            current_rpm = (uint32_t)tempRPM;
          }
          DEBUG_PORT.print("RPM: ");
          DEBUG_PORT.println(current_rpm);
          if (!is_debug_screen) {
            drawRPM(current_rpm, 0, TOP_HEIGHT + (SCREEN_HEIGHT - TOP_HEIGHT) / 2, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          }
          obd_state = SPEED;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          DEBUG_PORT.println("Blad pobrania RPM. Przechodze dalej.");
          current_rpm = 0;
          obd_state = SPEED;
        }
        break;
      }

    case SPEED:
      DEBUG_PORT.println("SPEED");
      {
        float tempSpeed = myELM327.kph();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_speed = (uint8_t)tempSpeed;
          DEBUG_PORT.print("Speed: ");
          DEBUG_PORT.print(current_speed);
          DEBUG_PORT.println(" km/h");
          if (!is_debug_screen) {
            drawSpeed(current_speed, SCREEN_WIDTH / 2, TOP_HEIGHT + (SCREEN_HEIGHT - TOP_HEIGHT) / 2, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          }
          obd_state = OIL_TEMP;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          DEBUG_PORT.println("Blad pobrania Speed. Przechodze dalej.");
          current_speed = 0;
          obd_state = OIL_TEMP;
        }
        break;
      }

    case OIL_TEMP:
      DEBUG_PORT.println("OIL_TEMP");
      {
        float tempOil = myELM327.oilTemp();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_oil_temp = tempOil;
          DEBUG_PORT.print("Oil Temp: ");
          DEBUG_PORT.print(current_oil_temp);
          DEBUG_PORT.println(" C");
          if (!is_debug_screen) {
            drawOilTemp(current_oil_temp, 0, TOP_HEIGHT, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          }
          obd_state = COOLANT_TEMP;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          DEBUG_PORT.println("Blad pobrania Oil Temp (brak czujnika?). Przechodze dalej.");
          // W przypadku braku PID, nie blokujemy pętli
          obd_state = COOLANT_TEMP;
        }
        break;
      }

    case COOLANT_TEMP:
      DEBUG_PORT.println("COOLANT_TEMP");
      {
        float tempCoolant = myELM327.engineCoolantTemp();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_coolant_temp = tempCoolant;
          DEBUG_PORT.print("Coolant Temp: ");
          DEBUG_PORT.print(current_coolant_temp);
          DEBUG_PORT.println(" C");
          if (!is_debug_screen) {
            drawCoolantTemp(current_coolant_temp, SCREEN_WIDTH / 2, TOP_HEIGHT, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          }
          obd_state = MAP_PRESS;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          DEBUG_PORT.println("Blad pobrania Coolant. Przechodze dalej.");
          obd_state = MAP_PRESS;
        }
        break;
      }

    case MAP_PRESS:
      DEBUG_PORT.println("MAP_PRESS");
      {
        // Biblioteczne myELM327.manifoldPressure() dziwnie się zachowuje (pokazuje 11 kPa podczas jazdy)
        // Dacia Sandero (Service 01, PID 0B) zwraca czystą 1-bajtową wartość (A) gdzie A = ciśnienie w kPa.
        float tempMap = myELM327.processPID(1, 11, 1, 1, 0, 0); // Service 1, PID 11(hex 0B), 1 odp, mnoż 1, dodaj 0

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          // Aplikujemy KOREKTĘ dla Dacii (pomnożenie przez 6.89 PSI -> kPa lub ew. 10)
          current_map = (uint8_t)(tempMap * MAP_MULTIPLIER);
          DEBUG_PORT.print("MAP (po korekcie): ");
          DEBUG_PORT.print(current_map);
          DEBUG_PORT.println(" kPa");
          if (is_debug_screen) {
            drawMAP(current_map, 0, TOP_HEIGHT, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          }
          obd_state = IAT_TEMP;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          current_map = 0;
          obd_state = IAT_TEMP;
        }
        break;
      }

    case IAT_TEMP:
      DEBUG_PORT.println("IAT_TEMP");
      {
        float tempIat = myELM327.intakeAirTemp();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_iat = tempIat;
          DEBUG_PORT.print("IAT: ");
          DEBUG_PORT.print(current_iat);
          DEBUG_PORT.println(" C");
          if (is_debug_screen) {
            drawIAT(current_iat, SCREEN_WIDTH / 2, TOP_HEIGHT, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          }
          obd_state = FUEL_TYPE_UPDATE;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          current_iat = 20.0; // Domyślnie używamy 20 stopni jako 'safe value' w wypadku braku odczytu
          obd_state = FUEL_TYPE_UPDATE;
        }
        break;
      }

    case FUEL_CONS:
      DEBUG_PORT.println("FUEL_CONS");
      {
        // Ponieważ estymujemy MAF z MAP i IAT, po prostu liczymy z wcześniej pobranych wartości w switch'u.
        calculateFuelConsumptionValue(current_rpm, current_map, current_iat, current_speed);

        drawFuelConsumption(current_fuel_consumption, current_fuel_type_code, 0, 0, topLeftW, topLeftH);
        
        if (is_debug_screen) {
            // Rysujemy pod spodem wyliczony MAF na połowie kwadratu
            drawEstimatedMAF(current_maf_estimate, 0, TOP_HEIGHT + (SCREEN_HEIGHT - TOP_HEIGHT) / 2, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
            // Obok MAFa rysujemy surowy odczyt PID 51 (Paliwo), żeby zobaczyć co faktycznie zwraca auto
            drawFuelCode(current_fuel_type_code, SCREEN_WIDTH / 2, TOP_HEIGHT + (SCREEN_HEIGHT - TOP_HEIGHT) / 2, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
        }

        obd_state = ENG_RPM;
        
        // Zmiana ekranu co 10 sekund (Przełączanie co pelny cykl OBD by nie urwać rysowania)
        if (millis() - last_screen_switch_time > 10000) {
          is_debug_screen = !is_debug_screen;
          last_screen_switch_time = millis();
          
          if (is_debug_screen) {
            drawDebugLayout(SCREEN_WIDTH, SCREEN_HEIGHT, TOP_HEIGHT, topLeftW, topLeftH, topRightW, topRightH, current_fuel_type_code);
          } else {
            drawMainLayout(SCREEN_WIDTH, SCREEN_HEIGHT, TOP_HEIGHT, topLeftW, topLeftH, topRightW, topRightH, current_fuel_type_code);
          }
        }
        break;
      }

    case FUEL_TYPE_UPDATE:
      DEBUG_PORT.println("FUEL_TYPE_UPDATE");
      {
        uint8_t tempFuelTypeCode = myELM327.fuelType(); // Odczytaj wartość z OBD

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_fuel_type_code = tempFuelTypeCode; // Zapisz odczytaną wartość
          DEBUG_PORT.print("Fuel Type Code: ");
          DEBUG_PORT.println(current_fuel_type_code, HEX); // Wyświetl kod w HEX dla łatwiejszego debugowania
          obd_state = FUEL_CONS;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) { 
          myELM327.printError();
          DEBUG_PORT.println("Błąd odczytu typu paliwa. Wymuszam powrót procedury.");
          // Nie ustawiamy 0xFF na siłę, zeby ekran nie mrugał pomaranczowym błędem po jednym zagubionym pakiecie
          obd_state = FUEL_CONS;
        }

        // Zawsze wywołaj funkcję rysującą z aktualnym kodem lub kodem błędu
        drawFuelTypeSection(current_fuel_type_code, topLeftW, 0, topRightW, topRightH);
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
  //tft.drawRGBBitmap(0, 0, bootlogo, 320, 170);
  //delay(3000);

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

  // Rysowanie początkowego układu na ekranie, przekazując odczytany typ paliwa
  last_screen_switch_time = millis();
  is_debug_screen = false;
  drawMainLayout(SCREEN_WIDTH, SCREEN_HEIGHT, TOP_HEIGHT, topLeftW, topLeftH, topRightW, topRightH, 0xFF); // Na starcie pokaż "BENZYNA" lub "LPG" po pierwszym odczycie w pętli
}

void loop() {
  readAndDisplayOBD();
  delay(10);
}
