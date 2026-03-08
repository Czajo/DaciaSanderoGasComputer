# Dacia Sandero Gas Computer 🚗💨 v4.5.1 Premium

Zaawansowany komputer pokładowy oparty na ESP32, zaprojektowany specjalnie dla samochodów **Dacia Sandero / Logan (silniki 1.0 TCe H4D)** z fabryczną instalacją LPG. Projekt rozwiązuje problem braku fabrycznego wskaźnika spalania gazu oraz braku czujnika MAF w tych jednostkach.

---

## 🚀 Główne fukcje (v4.5.1)

1.  **Premium Dashboard UI v4.5:** Nowoczesny, "karciany" interfejs z **gigantycznymi ikonami paliwa** (pełna wysokość kafelka 65px) oraz dyskretnymi etykietami `TR`, `TK`, `OL`, `PL`.
2.  **Inteligentne Spalanie (Speed-Density):** Estymacja masy powietrza na podstawie RPM, MAP i IAT. Automatyczne przełączanie jednostek: `L/100km` (podczas jazdy) oraz `L/h` (na postoju).
3.  **Detekcja Paliwa & Adaptacja:** Automatyczne rozpoznawanie paliwa (PID 51). System dynamicznie zmienia parametry fizyczne (AFR, gęstość) oraz kolorystykę interfejsu (Zielony dla LPG, Pomarańczowy dla Benzyny).
4.  **Niezależne Statystyki (Trip & Tank):**
    *   **TRASA (Trip):** Dystans i spalanie od uruchomienia, z podziałem na Benzynę i LPG.
    *   **TANKOWANIE (Tank):** Statystyki długookresowe dla obu paliw zapisywane niezależnie w pamięci **FFat**.
5.  **Brak "krzaczków":** Wszystkie napisy zostały oczyszczone z polskich znaków dla maksymalnej czytelności na wyświetlaczu TFT.

---

## 🎨 Interfejs Użytkownika

### Ekran Główny
*   **Góra:** Wielki wskaźnik spalania chwilowego z **gigantyczną ikoną** aktualnego paliwa (v4.5 Huge Icons).
*   **Dół (Karty v4.3):**
    *   `TR`: Średnie spalanie z obecnej trasy (**T**rasa).
    *   `TK`: Średnie spalanie od ostatniego resetu (**T**ankowanie).
    *   `OL`: Temperatura oleju (**O**lej).
    *   `PL`: Temperatura płynu chłodniczego (**P**łyn).
    *   *Uwaga: Etykiety są czarne i dyskretnie umieszczone w narożniku kafelka.*

### Obsługa Przycisku BOOT
*   **Krótkie kliknięcie:** Wyświetla ekran **STATYSTYKI** (side-by-side) z dokładnym podziałem na Benzynę (**B**) i LPG (**L**).
*   **Trzymanie (Live Timer):** Podczas trzymania przycisku na ekranie wyświetla się licznik sekund na żywo oraz informacja co zostanie zresetowane (LPG po 1.5s, Benzyna po 5s).

---

## 🛠️ Hardware & Setup

*   **ESP32 ze zintegrowanym LCD TFT (ST7789):** [Link do Temu](https://www.temu.com/pl/płytka-rozwojowa--esp32-z-16mb-flash-4-83cm-tft-lcd-st7789-modułem-wifi-bl-usb-c--idealna-dla--i-micropython-g-601099664065054.html)  
    *(Kompilacja wymagana przy ustawieniu Partycji np. na `Huge APP` lub `16MB Fatfs`)*
*   **Interfejs OBD-II:** Polecany **Vgate iCar Pro** (BT Classic lub BLE 4.0) - [Link do Vgate](https://vgate.pl/product/vgate-icar-pro-bt4-0/)
*   **Biblioteki:** `Adafruit_GFX`, `Adafruit_ST7789`, `ELMduino`, `FFat`.

### Konfiguracja połączenia
1.  **Podać mac-adres interfejsu** w kodzie: `uint8_t elm327_address[6] = {0x00, 0x00, ...};`.
2.  **"Podszyć się" pod sparowany telefon!** W `SerialBT.begin("Nazwa_Twojego_Telefonu", true);` wpisz nazwę telefonu, z którym iCar parował się wcześniej pomyślnie.

---

## 📈 Parametry techniczne
*   **AFR Benzyna:** 14.7 | **Gęstość:** 710 g/L
*   **AFR LPG:** 15.5 | **Gęstość:** 550 g/L
*   **VE (Volumetric Efficiency):** ~85% (Silnik 1.0 TCe)

---
*Projekt rozwijany pasjonacko dla społeczności Dacia Sandero / Logan. Szerokiej drogi!* 🏁
