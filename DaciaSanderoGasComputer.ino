
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>
#include "BluetoothSerial.h"
#include "ELMduino.h"
#include "layout.h" // Twój plik layout.h
#include "logo.h"
#include "sandero.h"
#include <FFat.h>

// Piny ekranu
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32
#define BTN_KEY21 21
#define BTN_BOOT 0
#define BTN_GPIO35 35

// Szerokość i wysokość ekranu
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 170

// Wysokość górnej sekcji (30% wysokości ekranu)
#define TOP_HEIGHT (SCREEN_HEIGHT * 30 / 100)

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
float current_fuel_lh = 0.0;          // W L/h
uint8_t current_fuel_type_code = 0xFF; // Globalna zmienna, zainicjalizowana wartością błędu
unsigned long last_heartbeat = 0;       // Do sprawdzania czy system zyje

// Statystyki Trasy (Trip) - RAM
double trip_distance = 0.0;      
double trip_distance_gas = 0.0;  // Dystans na benzynie (trasa)
double trip_distance_lpg = 0.0;  // Dystans na LPG (trasa)
double trip_fuel_liters = 0.0;   

// Statystyki Tankowania (Tank) - FFat
double tank_distance = 0.0;      
double tank_distance_gas = 0.0;  
double tank_distance_lpg = 0.0;  
double tank_fuel_gas = 0.0;     // Zuzyte paliwo - Benzyna (tankowanie)
double tank_fuel_lpg = 0.0;     // Zuzyte paliwo - LPG (tankowanie)
unsigned long last_stats_save_time = 0;
#define TANK_STATS_FILE "/tank_stats_v4.dat" // Nowa wersja usuniety licznik klikniec

// Czas ostatniej aktualizacji statystyk
unsigned long last_update_ms = 0;

// Zarzadzanie wyswietlaniem
unsigned long last_display_update_time = 0;

// --- Obsługa Statystyk Tankowania (FFat) ---
void saveTankStats() {
  File f = FFat.open(TANK_STATS_FILE, FILE_WRITE);
  if (f) {
    f.write((uint8_t*)&tank_distance, sizeof(tank_distance));
    f.write((uint8_t*)&tank_distance_gas, sizeof(tank_distance_gas));
    f.write((uint8_t*)&tank_distance_lpg, sizeof(tank_distance_lpg));
    f.write((uint8_t*)&tank_fuel_gas, sizeof(tank_fuel_gas));
    f.write((uint8_t*)&tank_fuel_lpg, sizeof(tank_fuel_lpg));
    f.close();
    DEBUG_PORT.println("[STATS] Statystyki tankowania zapisane.");
  }
}

void loadTankStats() {
  if (FFat.exists(TANK_STATS_FILE)) {
    File f = FFat.open(TANK_STATS_FILE, FILE_READ);
    if (f) {
      f.read((uint8_t*)&tank_distance, sizeof(tank_distance));
      f.read((uint8_t*)&tank_distance_gas, sizeof(tank_distance_gas));
      f.read((uint8_t*)&tank_distance_lpg, sizeof(tank_distance_lpg));
      f.read((uint8_t*)&tank_fuel_gas, sizeof(tank_fuel_gas));
      f.read((uint8_t*)&tank_fuel_lpg, sizeof(tank_fuel_lpg));
      f.close();
      DEBUG_PORT.print("[STATS] Wczytano dystans tankowania: ");
      DEBUG_PORT.println(tank_distance);
    }
  }
}

// Obliczanie dystansu i paliwa
void updateTripStats(float speed_kmh, float fuel_lh) {
  unsigned long now = millis();
  if (last_update_ms == 0) {
    last_update_ms = now;
    return;
  }
  
  double dt_hours = (now - last_update_ms) / 3600000.0;
  
  if (speed_kmh > 0.5) {
    double d_dist = speed_kmh * dt_hours;
    trip_distance += d_dist;
    tank_distance += d_dist;
    
    // Rozdział na paliwa
    if (current_fuel_type_code == 0x05 || current_fuel_type_code == 0x0C) {
      trip_distance_lpg += d_dist;
      tank_distance_lpg += d_dist;
    } else {
      trip_distance_gas += d_dist;
      tank_distance_gas += d_dist;
    }
  }
  
  double d_fuel = fuel_lh * dt_hours;
  trip_fuel_liters += d_fuel;
  
  if (current_fuel_type_code == 0x05 || current_fuel_type_code == 0x0C) {
    tank_fuel_lpg += d_fuel;
  } else {
    tank_fuel_gas += d_fuel;
  }
  
  last_update_ms = now;

  // Autozapis statystyk tankowania co minutę (lub przy dużej zmianie)
  if (now - last_stats_save_time > 60000) {
    saveTankStats();
    last_stats_save_time = now;
  }
}

// --- SPIFFS Logging ---
#define LOG_FILE "/obd_log.csv"
#define LOG_MAX_BYTES 5000000  // ~5 MB limit (mamy 9.9 MB FATFS na 16M Flash)
unsigned long log_entry_count = 0;
int8_t current_gear = 0;       // 0-luz, 1-6 biegi, -1 wsteczny (estymacja)

// Stałe do obliczeń spalania
const float AIR_FUEL_RATIO_GASOLINE = 14.7;     // Stosunek stechiometryczny powietrze/paliwo dla benzyny
const float GASOLINE_DENSITY_G_PER_L = 710.0;   // Gęstość benzyny w g/L (około)

// Kolory
#define COLOR_HEADER ST77XX_CYAN
#define COLOR_VALUE ST77XX_WHITE
#define COLOR_ERROR ST77XX_RED

// MAP dla Dacia Sandero (PID 0x0B) zwraca bezpośrednio kPa (1 bajt, A = kPa).
// Używamy standardowej metody biblioteki manifoldPressure() która ma scale=1 wbudowane.
// Jeśli Dacia zwraca faktycznie różnię ciśnienia (gauge), a nie absolutne, to trzeba
// dodać ~100 kPa, ale na razie zostawiamy bezpośredni odczyt do weryfikacji.

// Wartości silnika 1.0 TCe (Dacia H4D)
const float ENGINE_DISPLACEMENT_L = 0.999;     // Pojemność silnika w litrach
const float ENGINE_VE = 85.0;                  // Volumetric Efficiency (VE) w % (do kalibracji, np. 80-90%)

// Progi dla wyświetlania spalania
#define SPEED_THRESHOLD_LH 20      // Poniżej tej prękości pokazujemy L/h (ruszanie/dohamowywanie)
#define MAX_L_100KM_DISPLAY 99.9  // Limit na ekranie, żeby nie pokazywać "800L" при ruszaniu

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

// --- Estymacja biegu na podstawie RPM i Speed ---
// Zwraca: 0 (luz), 1-6 (biegi)
void updateGear(uint32_t rpm, uint8_t speed) {
  if (speed < 2 || rpm < 400) {
    current_gear = 0; // Luz lub postój
    return;
  }

  float ratio = (float)rpm / (float)speed;
  
  // Jeśli obroty są bliskie jałowym (<1300) a prędkość spora, to pewnie jedziesz na LUZIE
  // ZMNIEJSZONO PROG RATIO DO 30, ABY NIE WYKRYWAŁ LUZU NA 4/5 BIEGU PRZY NISKICH OBROTACH
  if (rpm < 1300 && ratio < 30) {
    current_gear = 0; 
    return;
  }

  // Doprecyzowane progi na podstawie Twoich logów (RPM/Speed):
  if (ratio > 115) current_gear = 1;      // 1 bieg (ok. 150)
  else if (ratio > 62) current_gear = 2; // 2 bieg (ok. 70)
  else if (ratio > 41) current_gear = 3; // 3 bieg (u Ciebie 44.4 @ 50km/h)
  else if (ratio > 31) current_gear = 4; // 4 bieg 
  else if (ratio > 24) current_gear = 5; // 5 bieg
  else current_gear = 6;                // 6 bieg
}

// --- Logowanie danych do FFat ---
void logDataToFFat() {
  // Nie loguj, jeśli wszystkie kluczowe parametry są zerami
  if (current_rpm == 0 && current_speed == 0 && current_map == 0) {
    return;
  }

  // Sprawdzamy czy plik istnieje. Jeśli nie, twórz nagłówek.
  if (!FFat.exists(LOG_FILE)) {
    File f = FFat.open(LOG_FILE, FILE_WRITE);
    if (f) {
      // 10 kolumn: millis, rpm, speed, gear, ratio, map, iat, fuel_type, l100km, lh
      f.println("millis,rpm,speed_kmh,gear,ratio,map_kpa,iat_c,fuel_type_hex,consumption_l100km,consumption_lh");
      f.close();
      log_entry_count = 0; // Nowy plik -> licznik od zera
    }
  }

  // Sprawdzenie limitu rozmiaru
  File check = FFat.open(LOG_FILE, FILE_READ);
  if (check) {
    size_t fileSize = check.size();
    check.close();
    if (fileSize > LOG_MAX_BYTES) {
      FFat.remove(LOG_FILE);
      DEBUG_PORT.println("[LOG] Plik przekroczył limit – wyzerowano.");
      log_entry_count = 0;
      return;
    }
  }

  File f = FFat.open(LOG_FILE, FILE_APPEND);
  if (!f) {
    DEBUG_PORT.println("[LOG] Blad otwarcia do dopisania!");
    return;
  }

  if (f.print(millis())) {
    float ratio = (current_speed > 0) ? (float)current_rpm / current_speed : 0;
    f.print(',');  f.print(current_rpm);
    f.print(',');  f.print(current_speed);
    f.print(',');  f.print(current_gear);
    f.print(',');  f.print(ratio, 1);
    f.print(',');  f.print(current_map);
    f.print(',');  f.print(current_iat, 1);
    f.print(',');  f.print(current_fuel_type_code, HEX);
    f.print(',');  f.print(current_fuel_consumption, 2);
    f.print(',');  f.println(current_fuel_lh, 2);
    f.close();
    log_entry_count++;
    
    if (log_entry_count % 10 == 0) {
      DEBUG_PORT.print("[LOG] Zapisano wierszy: ");
      DEBUG_PORT.println(log_entry_count);
    }
  } else {
    DEBUG_PORT.println("[LOG] Blad zapisu danych!");
    f.close();
  }
}

// --- Obsługa komend przez Serial Monitor ---
void handleButton() {
  if (digitalRead(BTN_BOOT) == LOW) {
    unsigned long press_start = millis();
    DEBUG_PORT.println("[BTN] Guzik BOOT wcisniety...");
    
    // Czekaj na zwolnienie lub przekroczenie czasu - LICZNIK NA ZYWO (v3.9)
    while (digitalRead(BTN_BOOT) == LOW) {
      unsigned long held = millis() - press_start;
      float secs = held / 1000.0;
      
      String msg = "";
      uint16_t col = ST77XX_YELLOW;
      
      if (held > 5000) {
          msg = "Pusc zeby resetowac\nBENZYNE\n\n";
          col = ST77XX_ORANGE;
      } else if (held > 1500) {
          msg = "Pusc zeby resetowac\nLPG\n\n";
          col = ST77XX_GREEN;
      } else {
          msg = "STATYSTYKI\n\n";
          col = ST77XX_CYAN;
      }
      
      msg += String(secs, 1) + "s";
      showCenteredStatusText(msg, col);
      delay(100); // Odświeżanie co 100ms dla płynnego licznika
    }
    
    unsigned long final_held = millis() - press_start;
    
    if (final_held > 5000) {
        // RESET BENZYNY
        tank_distance_gas = 0;
        tank_fuel_gas = 0;
        tank_distance = tank_distance_lpg; 
        saveTankStats();
        showCenteredStatusText("Zresetowano\nBENZYNE", ST77XX_ORANGE);
        delay(1500);
    } else if (final_held > 1500) {
        // RESET LPG
        tank_distance_lpg = 0;
        tank_fuel_lpg = 0;
        tank_distance = tank_distance_gas; 
        saveTankStats();
        showCenteredStatusText("Zresetowano\nLPG", ST77XX_GREEN);
        delay(1500);
    } else {
        // KRÓTKIE KLIKNIĘCIE - Nowy Ekran Info v3.8
        drawInfoPage(trip_distance, trip_distance_gas, trip_distance_lpg, 
                     tank_distance, tank_distance_gas, tank_distance_lpg, 
                     log_entry_count);
        delay(5000); 
    }
    
    drawMainLayout(SCREEN_WIDTH, SCREEN_HEIGHT, TOP_HEIGHT, 0, 0, 0, 0, current_fuel_type_code);
  }
}

void handleHeartbeat() {
  if (millis() - last_heartbeat > 1000) {
    DEBUG_PORT.print(".");
    last_heartbeat = millis();
  }
}

void handleSerialCommands() {
  if (!DEBUG_PORT.available()) return;
  char cmd = (char)DEBUG_PORT.read();

  if (cmd == 'D' || cmd == 'd') {
    DEBUG_PORT.println("[LOG] === DUMP OBD LOG ===");
    File f = FFat.open(LOG_FILE, FILE_READ);
    if (!f) {
      DEBUG_PORT.println("[LOG] Brak pliku lub pusty.");
      return;
    }
    while (f.available()) {
      DEBUG_PORT.write(f.read());
    }
    f.close();
    DEBUG_PORT.println("[LOG] === KONIEC LOGU ===");

  } else if (cmd == 'C' || cmd == 'c') {
    FFat.remove(LOG_FILE);
    log_entry_count = 0;
    DEBUG_PORT.println("[LOG] Log wyczyszczony.");

  } else if (cmd == 'I' || cmd == 'i') {
    File f = FFat.open(LOG_FILE, FILE_READ);
    size_t sz = f ? f.size() : 0;
    if (f) f.close();
    DEBUG_PORT.print("[LOG] Rozmiar logu: ");
    DEBUG_PORT.print(sz);
    DEBUG_PORT.print(" B | Wpisów: ");
    DEBUG_PORT.println(log_entry_count);
  }
}

// --- Funkcja do obliczania spalania metodą Speed-Density ---
void calculateFuelConsumptionValue(uint32_t rpm, uint8_t map_press, float iat_temp, uint8_t speed) {
  if (rpm > 0 && map_press > 0) {
    
    // Zabezpieczenie IAT: jeśli sensor szaleje (< -35C lub > 100C), przyjmij 20C
    float safe_iat = iat_temp;
    if (iat_temp < -35.0 || iat_temp > 100.0) safe_iat = 20.0;

    // Obliczenie MAF (g/s) ze wzoru Speed-Density (Ideal Gas Law)
    float imap = (float)rpm * (float)map_press / (safe_iat + 273.15);
    current_maf_estimate = (imap / 120.0) * (ENGINE_VE / 100.0) * ENGINE_DISPLACEMENT_L * 3.484;

    float afr = AIR_FUEL_RATIO_GASOLINE;
    float density = GASOLINE_DENSITY_G_PER_L;

    // Sprawdzamy czy aktualnie jeździsz na LPG (0x05, 0x09 lub 0x0C)
    // UWAGA: Logi pokazały że 0x09 to start na benzynie (bifuel), więc traktujemy jako LPG tylko 0x05 i 0x0C
    bool isLPG = (current_fuel_type_code == 0x05 || current_fuel_type_code == 0x0C);
    
    if (isLPG) {
      afr = AIR_FUEL_RATIO_LPG;
      density = LPG_DENSITY_G_PER_L;
    }

    float fuel_g_per_s = current_maf_estimate / afr;
    float fuel_L_per_s = fuel_g_per_s / density;

    // Spalanie L/h (zawsze liczone)
    current_fuel_lh = fuel_L_per_s * 3600.0;

    // Spalanie L/100km (tylko gdy jedziemy)
    if (speed > 0) {
      float speed_km_per_s = speed / 3600.0;
      current_fuel_consumption = (fuel_L_per_s / speed_km_per_s) * 100.0;
    } else {
      current_fuel_consumption = 0.0;
    }

    DEBUG_PORT.print("MAF: ");
    DEBUG_PORT.print(current_maf_estimate, 2);
    DEBUG_PORT.print(" | L/h: ");
    DEBUG_PORT.print(current_fuel_lh, 1);
    DEBUG_PORT.print(" | L/100km: ");
    DEBUG_PORT.println(current_fuel_consumption, 1);
  } else {
    current_fuel_consumption = 0.0;
    current_fuel_lh = 0.0;
    current_maf_estimate = 0.0;
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
        // Używamy standardowej metody biblioteki - PID 0x0B
        float tempMap = myELM327.manifoldPressure();

        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          // KOREKTA DLA DACIA (logi potwierdziły mnożnik x10)
          current_map = (uint8_t)(tempMap * 10.0); 
          DEBUG_PORT.print("MAP (raw): ");
          DEBUG_PORT.print(tempMap);
          DEBUG_PORT.print(" -> kPa: ");
          DEBUG_PORT.println(current_map);
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
        // Wykryj bieg przed logowaniem
        updateGear(current_rpm, current_speed);

        // OBLICZ SPALANIE (Kluczowy krok)
        calculateFuelConsumptionValue(current_rpm, current_map, current_iat, current_speed);

        if (current_speed >= 0.0 && current_fuel_lh >= 0.0) {
          updateTripStats(current_speed, current_fuel_lh);
          
          // Przygotuj dane do wyswietlenia
          float avgTrip = (trip_distance > 0.1) ? (trip_fuel_liters * 100.0 / trip_distance) : 0.0;
          
          // Średnia z tankowania dla AKTUALNEGO paliwa
          float avgTank = 0.0;
          if (current_fuel_type_code == 0x05 || current_fuel_type_code == 0x0C) {
              avgTank = (tank_distance_lpg > 0.1) ? (tank_fuel_lpg * 100.0 / tank_distance_lpg) : 0.0;
          } else {
              avgTank = (tank_distance_gas > 0.1) ? (tank_fuel_gas * 100.0 / tank_distance_gas) : 0.0;
          }
          
          // Rysuj nowy układ 2-kolumnowy (Implementacja w layout.h)
          updateTripDisplay(current_fuel_lh, current_fuel_consumption, avgTrip, avgTank, 
                           current_oil_temp, current_coolant_temp, current_fuel_type_code);
        }
        
        // Zapisz dane do FFat co cykl OBD
        logDataToFFat();

        obd_state = ENG_RPM;
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
  delay(3000); // Czekaj 3s, żeby Serial Monitor w PC zdążył się "podpiąć" i pokazać start

  // --- Inicjalizacja FFat i Auto-eksport ---
  bool ffatMounted = FFat.begin(false); // Najpierw próbujemy bez formatowania
  if (!ffatMounted) {
    DEBUG_PORT.println("[LOG] FFat nie zamontowany, probuje z formatowaniem...");
    ffatMounted = FFat.begin(true);
  }

  if (ffatMounted) {
    DEBUG_PORT.println("[LOG] FFat OK.");
    if (FFat.exists(LOG_FILE)) {
      File f = FFat.open(LOG_FILE, FILE_READ);
      if (f) {
        size_t s = f.size();
        DEBUG_PORT.print("[LOG] Wielkosc logu: ");
        DEBUG_PORT.print(s);
        DEBUG_PORT.println(" bajtow.");

        // Synchronizacja licznika wpisów na podstawie pliku
        log_entry_count = 0;
        while (f.available()) {
          if (f.read() == '\n') log_entry_count++;
        }
        if (log_entry_count > 0) log_entry_count--; // Odejmij linię nagłówka
        
        f.seek(0); // Wróć na start dla eksportu
        DEBUG_PORT.print("[LOG] Znaleziono ");
        DEBUG_PORT.print(log_entry_count);
        DEBUG_PORT.println(" wpisow z poprzednich sesji.");

        DEBUG_PORT.println("[LOG] === AUTO EXPORT OBD LOG ===");
        while (f.available()) {
          DEBUG_PORT.write(f.read());
        }
        f.close();
        DEBUG_PORT.println("[LOG] === KONIEC LOGU ===");
      }
    } else {
      DEBUG_PORT.println("[LOG] Brak pliku logu.");
    }
    DEBUG_PORT.println("[LOG] Komendy: C=wyczys, I=info, D=dump");
  } else {
    DEBUG_PORT.println("[LOG] KRYTYCZNY BLAD FFat!");
  }

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
    // Usunięto while(1), żeby można było odczytać logi bez auta
  } else {
    DEBUG_PORT.println("Bluetooth connected.");
    showCenteredStatusText("Polaczono z BT", ST77XX_YELLOW);
    delay(1000);
  }

  DEBUG_PORT.println("Initializing ELM327...");
  pinMode(BTN_BOOT,  INPUT_PULLUP);
  // Tymczasowo wylaczamy pinMode dla 21 i 35 zeby sprawdzic czy to one nie psuja ekranu
  // pinMode(BTN_KEY21, INPUT_PULLUP); 
  // pinMode(BTN_GPIO35, INPUT_PULLUP);
  if (!myELM327.begin(ELM_PORT, false, 2000)) {
    DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 2 (ELM327)");
    showCenteredStatusText("Brak polaczenia z OBD", ST77XX_RED);
    // Usunięto while(1)
  } else {
    DEBUG_PORT.println("Connected to ELM327");
    showCenteredStatusText("Polaczono z OBD", ST77XX_GREEN);
    delay(1000);
  }

  loadTankStats(); // Wczytaj statystyki paliwa
  
  // Rysowanie początkowego układu na ekranie
  drawMainLayout(SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 0, 0, 0xFF); 
}

void loop() {
  handleHeartbeat();      // Sprawdzaj czy program zyje
  handleSerialCommands(); // Obsługa komend D/C/I przez Serial Monitor
  handleButton();         // Test guzika BOOT
  readAndDisplayOBD();
  delay(10);
}
