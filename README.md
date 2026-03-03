# Dacia Sandero Gas Computer 🚗💨

Kod programu na ESP32, który próbuje liczyć i wyświetlać rzeczywiste spalanie, w oparciu o połączony czytnik OBD-II. Urządzenie zostało zaprojektowane specjalnie dla samochodów Dacia Sandero (silniki 1.0 TCe H4D z fabryczną instalacją LPG i brakiem czujnika MAF).

## 🛠️ Użyty Sprzęt (Hardware)
#### Odpowiednio mały ESP32 ze zintegrowanym LCD TFT (ST7789)
https://www.temu.com/pl/płytka-rozwojowa--esp32-z-16mb-flash-4-83cm-tft-lcd-st7789-modułem-wifi-bl-usb-c--idealna-dla--i-micropython-g-601099664065054.html
*(Kompilacja możliwa przy ustawieniu Partycji np. na `Huge APP` lub `16MB Fatfs`)*

#### Vgate iCar Pro BLE 4.0 / Bluetooth Classic
https://vgate.pl/product/vgate-icar-pro-bt4-0/

---

## ⚠️ Ważne: Połączenie Bluetooth (Vgate iCar Pro)
Aby płyta główna ESP32 poprawnie zautoryzowała się i sparowała z interfejsem Vgate bez żądania kodu z poziomu samego modułu samochodowego, należy w kodzie:
1. **Podać mac-adres interfejsu OBD-II** (odczytany np. za pomocą darmowych aplikacji do skanowania Bluetooth na telefonie i wpisany w linijce `uint8_t address[6] = {0x00, 0x00, ...};`).
2. **"Podszyć się" pod sparowany wcześniej telefon!** Na ESP32 uruchamiana jest komenda `SerialBT.begin("Nazwa_Twojego_Telefonu", true);` - wpisz tam dokładną nazwę telefonu (z uwzględnieniem spacji), z którym Vgate parował się wcześniej pomyślnie. Inaczej iCar może odrzucać połączenia przez wbudowane zabezpieczenia (Master-Slave problem).

---

## 🚀 Główne fukcje i cechy projektu
1. **Dynamiczne Wyliczanie Spalania (Speed-Density):** Silnik 1.0 TCe **nie posiada przepływomierza (sensora MAF)**, więc system samodzielnie estymuje zassaną masę powietrza bazując na Obrotach Silnika (RPM), Ciśnieniu w Kolektorze SSącym (MAP) oraz Temperaturze w Dolocie (IAT).
2. **Automatyczna Detekcja Paliwa:** Samochód raportuje używane paliwo w standardowym PID 51 (Type of Fuel). Komputer na bieżąco sprawdza odpowiedź ECU i płynnie podmienia w pamięci ułamki i gęstość (Benzyna = AFR 14.7 i ~710g/L, LPG = AFR 15.5 i ~550g/L). Powoduje to natychmiastową, fizyczną zmianę wskazań `L/100km` na wyższe w przypadku załączenia gazu (o około ~20% pod tym samym obciążeniem).
3. **Dynamiczne Warianty Kolorystyczne:** Interfejs posiada wbudowany "płynny rzut na oko". Kiedy komputer wykryje jazdę na LPG, duże napisy spalania stają się soczyście zielone, synchronizując się z kontrolką "LPG". Po wymuszeniu benzyny – w ułamku sekundy kolor wielkiego napisu ewoluuje na pomarańczowy ("BENZYNA").
4. **Niestandardowy odczyt MAP:** W środowisku Dacia standardowe biblioteki (jak `ELMduino` i funkcja `manifoldPressure()`) bywają błędnie skalowane przez zachowanie sterownika. Dla Dacii wdrożono bezpośrednie odpytywanie PID hex `0x0B` w Service 01 i dodano w kodzie `MAP_MULTIPLIER = 10.0`, aby prawidłowo przesunąć przecinek we wzorze i odczytać dokładną wartość w **kPa**.

## 🎨 Logika Ekrany i Debugging
Interfejs podzielony jest na dwa zautomatyzowane rzuty klatek (co 10 sekund z racji na funkcję Debug).
1. **Ekran Główny [ 0 - 10 s ]:** Spalanie, Temperatura Cieczy/Oleju, RPM, i Prędkość.
2. **Ekran Diagnostyczny [ 10 - 20 s ]:** Wyświetla obróbkę żywą: `IAT Temp`, `MAP Press`, wyliczoną przed momentem masę `Est. MAF` (w g/s) używaną do wzoru L/100km, oraz surowy odbiór Hexadecymalny znacznika aktualnego paliwa ze sterownika (pomagający rozpoznać błędy komunikacyjne czy odcięcia zapłonu). Pętla ELM327 zaprojektowana jest w izolowanych STANACH (State Machine), aby utrata komunikatu np. z czujnikiem oleju, ignorowała błąd i wczytywała resztę danych (brak czkawki wyświetlacza).
