#ifndef ELM327_CONNECTION_H
#define ELM327_CONNECTION_H

#include "BluetoothSerial.h"
#include "ELMduino.h"

// Obiekty Bluetooth i ELM327
extern BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

// Pamiętaj, ABY ZMIENIĆ TEN ADRES MAC NA ADRES SWOJEGO URZĄDZENIA OBD-II!
extern uint8_t elm327_address[6];

extern ELM327 myELM327;

// Zmienne do przechowywania odczytów z OBD
extern uint32_t current_rpm;
extern uint8_t current_speed;
extern float current_oil_temp;
extern float current_coolant_temp;
extern float current_maf;
extern float current_fuel_consumption;
extern uint8_t current_fuel_type_code;

// Definicja stanów odczytu OBD
enum OBD_STATE {
  ENG_RPM,
  SPEED,
  OIL_TEMP,
  COOLANT_TEMP,
  FUEL_CONS,
  FUEL_TYPE_UPDATE
};

// Zmienna stanu
extern OBD_STATE obd_state;

// Funkcje połączenia
bool initializeBluetoothConnection();
bool initializeELM327Connection();
void showConnectionStatus(const String& message, uint16_t color);

// Funkcje odczytu OBD
void readAndDisplayOBD();

// Funkcja do obliczania spalania
void calculateFuelConsumptionValue(float maf, uint8_t speed);

// Funkcja do dodawania logów debug (zdefiniowana w głównym pliku)
extern void addDebugLog(String message);

#endif // ELM327_CONNECTION_H 