#include "elm327_connection.h"
#include "layout.h"

// Definicje stałych ekranu (potrzebne dla funkcji rysujących)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 170
#define TOP_HEIGHT (SCREEN_HEIGHT * 30 / 100)
extern int topLeftW, topLeftH, topRightW, topRightH;

// Obiekty Bluetooth i ELM327
BluetoothSerial SerialBT;
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

// Zmienna stanu
OBD_STATE obd_state = ENG_RPM;

// Stałe do obliczeń spalania
const float AIR_FUEL_RATIO_GASOLINE = 14.7;     // Stosunek stechiometryczny powietrze/paliwo dla benzyny
const float AIR_FUEL_RATIO_LPG = 15.7;          // Stosunek stechiometryczny powietrze/paliwo dla LPG
const float GASOLINE_DENSITY_G_PER_L = 710.0;   // Gęstość benzyny w g/L (około)
const float LPG_DENSITY_G_PER_L = 550.0;        // Gęstość LPG w g/L (około)

// Funkcja do wyświetlania statusu połączenia
void showConnectionStatus(const String& message, uint16_t color) {
  showCenteredStatusText(message, color);
}

// Inicjalizacja połączenia Bluetooth
bool initializeBluetoothConnection() {
  DEBUG_PORT.println("Connecting to OBD scanner via Bluetooth...");
  
  SerialBT.setPin("1234", 4);
  ELM_PORT.begin("Czajo_x5pro", true);
  
  if (!ELM_PORT.connect(elm327_address)) {
    DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1 (Bluetooth)");
    showConnectionStatus("Brak polaczenia z BT", ST77XX_RED);
    return false;
  }
  
  DEBUG_PORT.println("Bluetooth connected.");
  showConnectionStatus("Polaczono z BT", ST77XX_YELLOW);
  delay(1000);
  return true;
}

// Inicjalizacja połączenia ELM327
bool initializeELM327Connection() {
  DEBUG_PORT.println("Initializing ELM327...");
  
  if (!myELM327.begin(ELM_PORT, false, 2000)) {
    DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 2 (ELM327)");
    showConnectionStatus("Brak polaczenia z OBD", ST77XX_RED);
    return false;
  }
  
  DEBUG_PORT.println("Connected to ELM327");
  showConnectionStatus("Polaczono z OBD", ST77XX_GREEN);
  delay(1000);
  return true;
}

// --- Funkcja do obliczania spalania ---
void calculateFuelConsumptionValue(float maf, uint8_t speed) {
  if (speed > 0 && maf > 0 && maf < 500) { // Dodana walidacja MAF
    // Wybierz stałe w zależności od typu paliwa
    float air_fuel_ratio;
    float fuel_density;
    
    if (current_fuel_type_code == 0x05 || current_fuel_type_code == 0x0C) {
      // LPG
      air_fuel_ratio = AIR_FUEL_RATIO_LPG;
      fuel_density = LPG_DENSITY_G_PER_L;
      DEBUG_PORT.println("Obliczanie spalania dla LPG");
    } else {
      // Benzyna (domyślnie)
      air_fuel_ratio = AIR_FUEL_RATIO_GASOLINE;
      fuel_density = GASOLINE_DENSITY_G_PER_L;
      DEBUG_PORT.println("Obliczanie spalania dla benzyny");
    }
    
    float fuel_g_per_s = maf / air_fuel_ratio;
    float fuel_L_per_s = fuel_g_per_s / fuel_density;
    float speed_km_per_s = speed / 3600.0;
    
    // Dodana ochrona przed dzieleniem przez bardzo małe wartości
    if (speed_km_per_s > 0.001) {
      float fuel_L_per_km = fuel_L_per_s / speed_km_per_s;
      current_fuel_consumption = fuel_L_per_km * 100.0;
      
      DEBUG_PORT.print("MAF: "); DEBUG_PORT.print(maf); DEBUG_PORT.println(" g/s");
      DEBUG_PORT.print("Speed: "); DEBUG_PORT.print(speed); DEBUG_PORT.println(" km/h");
      DEBUG_PORT.print("Air/Fuel ratio: "); DEBUG_PORT.println(air_fuel_ratio);
      DEBUG_PORT.print("Fuel density: "); DEBUG_PORT.print(fuel_density); DEBUG_PORT.println(" g/L");
      DEBUG_PORT.print("Spalanie (estymowane): ");
      DEBUG_PORT.print(current_fuel_consumption, 2);
      DEBUG_PORT.println(" L/100km");
    } else {
      current_fuel_consumption = 0.0;
      DEBUG_PORT.println("Prędkość zbyt niska, nie można obliczyć spalania.");
    }
  } else {
    current_fuel_consumption = 0.0;
    if (speed <= 0) {
      DEBUG_PORT.println("Samochód stoi, nie można obliczyć spalania.");
    } else if (maf <= 0) {
      DEBUG_PORT.println("Brak danych MAF, nie można obliczyć spalania.");
    } else {
      DEBUG_PORT.println("MAF poza zakresem, nie można obliczyć spalania.");
    }
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
            current_rpm = 0; // lub np. -1 jeśli chcesz wyświetlać "---"
            DEBUG_PORT.println("RPM zbyt niskie, ustawiam 0");
          } else {
            current_rpm = (uint32_t)tempRPM;
          }
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
      DEBUG_PORT.println("SPEED");
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
      DEBUG_PORT.println("OIL_TEMP");
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
      DEBUG_PORT.println("COOLANT_TEMP");
      {
        float tempCoolant = myELM327.engineCoolantTemp();
        if (myELM327.nb_rx_state == ELM_SUCCESS) {
          current_coolant_temp = tempCoolant;
          DEBUG_PORT.print("Coolant Temp: ");
          DEBUG_PORT.print(current_coolant_temp);
          DEBUG_PORT.println(" C");
          drawCoolantTemp(current_coolant_temp, SCREEN_WIDTH / 2, TOP_HEIGHT, SCREEN_WIDTH / 2, (SCREEN_HEIGHT - TOP_HEIGHT) / 2);
          obd_state = FUEL_TYPE_UPDATE;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          current_coolant_temp = -1;
          obd_state = FUEL_TYPE_UPDATE;
        }
        break;
      }

    case FUEL_CONS:
      DEBUG_PORT.println("FUEL_CONS");
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
          obd_state = ENG_RPM;
        } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
          myELM327.printError();
          DEBUG_PORT.println("Błąd odczytu MAF. Spalanie ustawione na 0.");
          current_maf = 0.0;
          current_fuel_consumption = 0.0;
          drawFuelConsumption(current_fuel_consumption, 0, 0, topLeftW, topLeftH);
          obd_state = ENG_RPM;
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
        } else { // W przypadku błędu odczytu (różne od ELM_GETTING_MSG)
          myELM327.printError();
          DEBUG_PORT.println("Błąd odczytu typu paliwa.");
          // Ustaw current_fuel_type_code na specjalną wartość błędu (np. 0xFF),
          // aby drawFuelTypeSection mogła to obsłużyć.
          current_fuel_type_code = 0xFF;
        }

        // Zawsze wywołaj funkcję rysującą z aktualnym kodem lub kodem błędu
        drawFuelTypeSection(current_fuel_type_code, topLeftW, 0, topRightW, topRightH);
        obd_state = FUEL_CONS; // Powrót na początek cyklu
        break;
      }
  }
} 