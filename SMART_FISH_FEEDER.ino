#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
Servo servo;
OneWire oneWire(12);
DallasTemperature ds18b20(&oneWire);

#define BTN_MENU 5
#define BTN_UP   4
#define BTN_DOWN 3
#define BTN_OK   2
#define SERVO_PIN 6
#define RELAY_PIN 7
#define RELAY_ON LOW
#define RELAY_OFF HIGH
const int PH_PIN = A0;
float nilai_ph = 0;

float PH4 = 3.1;   // Tegangan saat pH 4
float PH7 = 2.6;   // Tegangan saat pH 7

String menuItems[] = {
  "1.Set Jam",
  "2.Set Jadwal",
  "3.Batas Suhu",
  "4.Keluar"
};

int jamPagi = 7, menitPagi = 0;
int jamSore = 17, menitSore = 0;
int addrJamPagi = 10, addrMenitPagi = 20, addrJamSore = 30, addrMenitSore = 40;
int batasSuhu = 30, addrBatasSuhu = 50;

int currentMenu = 0;
bool inMenu = false;
bool menuChanged = true;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;
float lastTemp = -999;

unsigned long statusTimeout = 0;

void showLoadingAnimation() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  lcd.setCursor(0, 1);
  
  for (int i = 0; i < 16; i++) {
    lcd.print((char)255); // Karakter blok penuh
    delay(100);           // Delay antar blok (atur sesuai selera)
  }
}

void setup() {
  Serial.begin(9600);
  
  lcd.init();
  lcd.backlight();

  showLoadingAnimation();
  
  Wire.begin();
  ds18b20.begin();
  
  servo.attach(SERVO_PIN);
  servo.write(0);
  
  pinMode(BTN_MENU, INPUT);
  pinMode(BTN_UP, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_OK, INPUT);
  pinMode(PH_PIN, INPUT);

  EEPROM.get(addrJamPagi, jamPagi);
  EEPROM.get(addrMenitPagi, menitPagi);
  EEPROM.get(addrJamSore, jamSore);
  EEPROM.get(addrMenitSore, menitSore);
  EEPROM.get(addrBatasSuhu, batasSuhu);
  
  if (batasSuhu < 10 || batasSuhu > 50) {
    batasSuhu = 30;
  }
  
  if (!rtc.begin()) { 
    lcd.setCursor(0, 0); 
    lcd.print("RTC tdk terbaca"); 
    while (1); 
  }
  
  if (rtc.lostPower()) rtc.adjust(DateTime(__DATE__, __TIME__));

  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("SMART AQUARIUM");
  lcd.setCursor(1, 1);
  lcd.print("D3 T. KOMPUTER");
  delay(2000);
}

void loop() {
  DateTime now = rtc.now();
  static bool pagiSudah = false;
  static bool soreSudah = false;
  static int lastSecond = -1;

  if (!inMenu) {
    if (now.second() != lastSecond) {
      lastSecond = now.second();

      if (statusTimeout > 0 && millis() > statusTimeout) {
        statusTimeout = 0;
        lcd.clear();
      }

      if (statusTimeout == 0) {
        showMainDisplay(now);
      }

      if (now.hour() == jamPagi && now.minute() == menitPagi && !pagiSudah) {
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Pakan Pagi Tiba ");
        triggerServo(); pagiSudah = true;
        statusTimeout = millis() + 3000; // Tampilkan status 3 detik
      } else if (now.hour() == jamSore && now.minute() == menitSore && !soreSudah) {
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Pakan Sore Tiba ");
        triggerServo(); soreSudah = true;
        statusTimeout = millis() + 3000;
      }

      if (now.minute() != menitPagi) pagiSudah = false;
      if (now.minute() != menitSore) soreSudah = false;
    }
  }

  if (!inMenu && digitalRead(BTN_MENU) == HIGH && millis() - lastButtonPress > debounceDelay) {
    inMenu = true;
    menuChanged = true;
    lastButtonPress = millis();
  }

  if (inMenu) {
    if (menuChanged) { showMenu(); menuChanged = false; }
    menuNavigation();
  }
}

void showMainDisplay(DateTime now) {

  // baca sensor suhu air  
  ds18b20.requestTemperatures();
  float suhu = ds18b20.getTempCByIndex(0);

  // baca sensor ph air  
  int nilai_analog_ph = analogRead(PH_PIN);
  double teganganPh = 5.0 / 1024.0 * nilai_analog_ph;
  float ph_step = (PH4 - PH7) / 3.0;
  nilai_ph = 7.00 + ((PH7 - teganganPh) / ph_step);

  Serial.print("Nilai Suhu Air: ");
  Serial.println(suhu);
  Serial.print("Nilai PH: ");
  Serial.println(nilai_ph, 2);
  
  // Kontrol heater relay
  if (suhu < batasSuhu) {
    Serial.println("Heater menyala");
    digitalWrite(RELAY_PIN, RELAY_ON);  // Nyalakan heater
  } else {
    Serial.println("Heater mati");
    digitalWrite(RELAY_PIN, RELAY_OFF);   // Matikan heater
  }

  if (abs(suhu - lastTemp) >= 0.1) {
    lastTemp = suhu;
  }
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(lastTemp, 1);   // misal: 26.5
  lcd.print((char)223);     // derajat
  lcd.print("C ");
  
  lcd.print("P:");
  lcd.print(nilai_ph, 1);   // misal: 7.2

  lcd.setCursor(0, 1);
  lcd.print("Jam: ");
  if (now.hour() < 10) {
    lcd.print("0"); 
    lcd.print(now.hour()); 
    lcd.print(":");
  }
  if (now.minute() < 10) {
    lcd.print("0"); 
    lcd.print(now.minute()); 
    lcd.print(":");
  }
  if (now.second() < 10) {
    lcd.print("0");
    lcd.print(now.second());
  }
}



void triggerServo() {
  for (int i = 0; i < 4; i++) {
    Serial.println("[Servo] Buka ke-" + String(i + 1));
    servo.write(90);
    delay(500);
    servo.write(0);
    delay(500);
  }
}

void showMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Menu:");
  lcd.setCursor(0, 1);
  lcd.print("> " + menuItems[currentMenu]);
}

void menuNavigation() {
  if (digitalRead(BTN_UP) == HIGH && millis() - lastButtonPress > debounceDelay) {
    currentMenu = (currentMenu - 1 + 4) % 4;
    menuChanged = true;
    lastButtonPress = millis();
  }
  if (digitalRead(BTN_DOWN) == HIGH && millis() - lastButtonPress > debounceDelay) {
    currentMenu = (currentMenu + 1) % 4;
    menuChanged = true;
    lastButtonPress = millis();
  }
  if (digitalRead(BTN_OK) == HIGH && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    lcd.clear();
    switch (currentMenu) {
      case 0: setJamRTC();
      break;
      case 1: menuSetJadwal();
      break;
      case 2: setBatasSuhu();
      break;
      case 3:
        lcd.setCursor(0, 0);
        lcd.print("Keluar Menu");
        delay(1000);
        inMenu = false;
        lcd.clear();
        break;
    }
    menuChanged = true;
  }
}
// Fungsi: setJamRTC(), menuSetJadwal(), setPakanPagi(), setPakanSore(), setBatasSuhu() tetap digunakan.
void setJamRTC() {
  DateTime now = rtc.now();
  int h = now.hour();
  int m = now.minute();
  bool editJam = true;
  bool setting = true;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Jam RTC");

  while (setting) {
    lcd.setCursor(0, 1);
    lcd.print("Wkt: ");
    lcd.print(editJam ? ">" : " ");
    if (h < 10) lcd.print("0");
    lcd.print(h); lcd.print(":");
    lcd.print(!editJam ? ">" : " ");
    if (m < 10) lcd.print("0");
    lcd.print(m);

    if (digitalRead(BTN_UP) == HIGH && millis() - lastButtonPress > debounceDelay) {
      if (editJam) h = (h + 1) % 24;
      else m = (m + 1) % 60;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_DOWN) == HIGH && millis() - lastButtonPress > debounceDelay) {
      if (editJam) h = (h - 1 + 24) % 24;
      else m = (m - 1 + 60) % 60;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_MENU) == HIGH && millis() - lastButtonPress > debounceDelay) {
      editJam = !editJam;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_OK) == HIGH && millis() - lastButtonPress > debounceDelay) {
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, 0));
      lcd.setCursor(0, 0);
      lcd.print("Jam Disimpan    ");
      delay(1000);
      setting = false;
      lastButtonPress = millis();
    }
  }
  lcd.clear();
}

// ...kode sebelumnya tetap...

void menuSetJadwal() {
  int subMenu = 0;
  int totalSub = 2;
  bool pilih = true;

  lcd.clear();
  while (pilih) {
    lcd.setCursor(0, 0);
    lcd.print("Set Jadwal Pakan");
    lcd.setCursor(0, 1);
    lcd.print(subMenu == 0 ? ">Pagi " : " Pagi ");
    lcd.print(subMenu == 1 ? ">Sore" : " Sore");

    if (digitalRead(BTN_UP) == HIGH && millis() - lastButtonPress > debounceDelay) {
      subMenu = (subMenu - 1 + totalSub) % totalSub;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_DOWN) == HIGH && millis() - lastButtonPress > debounceDelay) {
      subMenu = (subMenu + 1) % totalSub;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_OK) == HIGH && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      if (subMenu == 0) setPakanPagi();
      else setPakanSore();
      pilih = false;
    }
    if (digitalRead(BTN_MENU) == HIGH && millis() - lastButtonPress > debounceDelay) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Kembali...");
      delay(500);
      inMenu = false;
      pilih = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tekan Menu...");
      lastButtonPress = millis();
    }
  }
  lcd.clear();
}

void setPakanPagi() {
  bool setting = true;
  bool editJam = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Atur Pakan Pagi");

  while (setting) {
    lcd.setCursor(0, 1);
    lcd.print("Wkt: ");
    lcd.print(editJam ? ">" : " ");
    if (jamPagi < 10) lcd.print("0");
    lcd.print(jamPagi); lcd.print(":");
    lcd.print(!editJam ? ">" : " ");
    if (menitPagi < 10) lcd.print("0");
    lcd.print(menitPagi); lcd.print("   ");

    if (digitalRead(BTN_UP) == HIGH && millis() - lastButtonPress > debounceDelay) {
      if (editJam) jamPagi = (jamPagi + 1) % 24;
      else menitPagi = (menitPagi + 1) % 60;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_DOWN) == HIGH && millis() - lastButtonPress > debounceDelay) {
      if (editJam) jamPagi = (jamPagi - 1 + 24) % 24;
      else menitPagi = (menitPagi - 1 + 60) % 60;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_MENU) == HIGH && millis() - lastButtonPress > debounceDelay) {
      editJam = !editJam;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_OK) == HIGH && millis() - lastButtonPress > debounceDelay) {
      EEPROM.put(addrJamPagi, jamPagi);
      EEPROM.put(addrMenitPagi, menitPagi);
      lcd.setCursor(0, 0);
      lcd.print("Disimpan!       ");
      delay(1000);
      setting = false;
      lastButtonPress = millis();
    }
  }
  lcd.clear();
}

void setPakanSore() {
  bool setting = true;
  bool editJam = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Atur Pakan Sore");

  while (setting) {
    lcd.setCursor(0, 1);
    lcd.print("Wkt: ");
    lcd.print(editJam ? ">" : " ");
    if (jamSore < 10) lcd.print("0");
    lcd.print(jamSore); lcd.print(":");
    lcd.print(!editJam ? ">" : " ");
    if (menitSore < 10) lcd.print("0");
    lcd.print(menitSore); lcd.print("   ");

    if (digitalRead(BTN_UP) == HIGH && millis() - lastButtonPress > debounceDelay) {
      if (editJam) jamSore = (jamSore + 1) % 24;
      else menitSore = (menitSore + 1) % 60;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_DOWN) == HIGH && millis() - lastButtonPress > debounceDelay) {
      if (editJam) jamSore = (jamSore - 1 + 24) % 24;
      else menitSore = (menitSore - 1 + 60) % 60;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_MENU) == HIGH && millis() - lastButtonPress > debounceDelay) {
      editJam = !editJam;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_OK) == HIGH && millis() - lastButtonPress > debounceDelay) {
      EEPROM.put(addrJamSore, jamSore);
      EEPROM.put(addrMenitSore, menitSore);
      lcd.setCursor(0, 0);
      lcd.print("Disimpan!       ");
      delay(1000);
      setting = false;
      lastButtonPress = millis();
    }
  }
  lcd.clear();
}


void setBatasSuhu() {
  bool setting = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Batas Suhu");

  while (setting) {
    lcd.setCursor(0, 1);
    lcd.print("Suhu: ");
    lcd.print(batasSuhu);
    lcd.print((char)223);
    lcd.print("C  ");

    if (digitalRead(BTN_UP) == HIGH && millis() - lastButtonPress > debounceDelay) {
      batasSuhu++;
      if (batasSuhu > 50) batasSuhu = 50;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_DOWN) == HIGH && millis() - lastButtonPress > debounceDelay) {
      batasSuhu--;
      if (batasSuhu < 10) batasSuhu = 10;
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_MENU) == HIGH && millis() - lastButtonPress > debounceDelay) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Kembali...");
      delay(500);
      setting = false;
      inMenu = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tekan Menu...");
      lastButtonPress = millis();
    }
    if (digitalRead(BTN_OK) == HIGH && millis() - lastButtonPress > debounceDelay) {
      EEPROM.put(addrBatasSuhu, batasSuhu);
      lcd.setCursor(0, 0);
      lcd.print("Tersimpan!      ");
      delay(1000);
      setting = false;
      lastButtonPress = millis();
    }
  }
  lcd.clear();
}
