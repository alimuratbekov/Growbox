#include <iarduino_RTC.h>
#include <LiquidCrystal_I2C.h>

// LCD pins:
// SDA - A4
// SCL - A5

// RTC pins:
// RST - 5
// DAT - 6
// CLK - 7

#define LED_BTN_PIN 2
#define LED_MOSFET_PIN 11
#define PUMP_BTN_PIN 4
#define PUMP_MOSFET_PIN 12
#define LDR_PIN A0

iarduino_RTC time(RTC_DS1302, 5, 7, 6);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  delay(300);

  // устанавливаем режимы портов
  pinMode(LED_BTN_PIN, INPUT_PULLUP);
  pinMode(LED_MOSFET_PIN, OUTPUT);
  pinMode(PUMP_BTN_PIN, INPUT_PULLUP);
  pinMode(PUMP_MOSFET_PIN, OUTPUT);

  // инициализируем RTC-модуль
  time.begin();
  //time.settime(0, 0, 23, 5, 12, 20, 6);

  // инициализируем LCD
  lcd.init();
  lcd.backlight();

  Serial.begin(9600);
}

// переменные для обработки кнопки светодиодного модуля
bool led_btn, led_btn_flag = false;

// переменные для обработки кнопки насоса
bool pump_btn, pump_btn_flag = false;

// переменные для управления транзистором светодиодного модуля
bool led_mosfet_flag = false;
const int day_hour = 5;
const int day_minute = 0;
const int day_second = 0;
const int night_hour = 22;
const int night_minute = 0;
const int night_second = 0;
const int ldr_threshold = 300;
long long last_led_switch = 0;
int led_waiting_time = 3000;

// переменные для управления транзистором насоса
bool pump_mosfet_flag = false;
long long last_pump_activation = 0;
int watering_time = 3000;
const int pump_activation_hour = 16;
const int pump_activation_minute = 0;
const int pump_activation_second = 0;

void loop() {
  handleLedButton();
  if (pump_mosfet_flag == true) {
    if (millis() - last_pump_activation >= watering_time) {
      pump_mosfet_flag = false;
      digitalWrite(PUMP_MOSFET_PIN, pump_mosfet_flag);
    }
  }
  else {
    handlePumpButton();
  }
  if (millis()%100 == 0) {
    displayTime();
    timingLed(time.Hours, time.minutes, time.seconds);
    timingPump(time.Hours, time.minutes, time.seconds);
    displayLedState(led_mosfet_flag);
    displayPumpState(pump_mosfet_flag);
  }
  if (isNight() == true && millis()-last_led_switch >= led_waiting_time) {
    last_led_switch = millis();
    Serial.println(analogRead(A0));
    if (checkLightPresence() == true && led_mosfet_flag == true) {
      led_mosfet_flag = false;
      digitalWrite(LED_MOSFET_PIN, led_mosfet_flag);
    }
    else if (checkLightPresence() == false && led_mosfet_flag == false) {
      led_mosfet_flag = true;
      digitalWrite(LED_MOSFET_PIN, led_mosfet_flag);
    }
  }
}

// функция обработки нажатия кнопки светодиодного модуля
void handleLedButton() {
  led_btn = !digitalRead(LED_BTN_PIN);
  if (led_btn == true && led_btn_flag == false) {
    led_btn_flag = true;
  }
  else if (led_btn == false && led_btn_flag == true) {
    led_btn_flag = false;
    led_mosfet_flag = !led_mosfet_flag;
    digitalWrite(LED_MOSFET_PIN, led_mosfet_flag);
  }
}

// функция обработки нажатия кнопки насоса
void handlePumpButton() {
  pump_btn = !digitalRead(PUMP_BTN_PIN);
  if (pump_btn == true && pump_btn_flag == false) {
    pump_btn_flag = true;
  }
  else if (pump_btn == false && pump_btn_flag == true) {
    pump_btn_flag = false;
    activatePump();
  }
}

// функция активации насоса
void activatePump() {
  last_pump_activation = millis();
  pump_mosfet_flag = true;
  digitalWrite(PUMP_MOSFET_PIN, pump_mosfet_flag);
}

// функция отображения текущего состояния насоса на дисплее
void displayPumpState(bool pump_on) {
  String pump_state;
  if (pump_on)
    pump_state = "ON ";
  else
    pump_state = "OFF";
  pump_state = "PUMP:" + pump_state;
  displayString(pump_state, 8, 1);
}

// функция для управления светодиодом в зависимости от времени суток
void timingLed(int h, int m, int s) {
  if (h == night_hour && m == night_minute && s == night_second) {
    led_mosfet_flag = true;
    digitalWrite(LED_MOSFET_PIN, led_mosfet_flag);
  }
  else if (h == day_hour && m == day_minute && s == day_second) {
    led_mosfet_flag = false;
    digitalWrite(LED_MOSFET_PIN, led_mosfet_flag);
  }
}

void timingPump(int h, int m, int s) {
  if (h == pump_activation_hour && m == pump_activation_minute && s == pump_activation_second && pump_mosfet_flag == false) {
    activatePump();
  }
}

// функция отображения текущего времени на дисплее
void displayTime() {
  String current_time = time.gettime("H:i:s");
  if (isNight() == true)
    current_time += " Night";
  else
    current_time += " Day  ";
  displayString(current_time, 0, 0);
}

// функция отображения текущего состояния светодиодного модуля на дисплее
void displayLedState(bool led_on) {
  String led_state;
  if (led_on)
    led_state = "ON ";
  else
    led_state = "OFF";
  led_state = "LED:" + led_state;
  displayString(led_state, 0, 1);
}

// функция для посимвольного выведения строки на дисплей
void displayString(String s, int column, int row) {
  for (int i=0; i<s.length();i++) {
    lcd.setCursor(i+column, row);
    lcd.print(s[i]);
  }
}

// функция для проверки уровня освещенности
bool checkLightPresence() {
  int val = analogRead(LDR_PIN);
  if (val > ldr_threshold)
    return true;
  else
    return false;
}

// функция для проверки времени суток
bool isNight() {
  int h = time.Hours;
  int m = time.minutes;
  int s = time.seconds;
  if (h >= night_hour || h <= day_hour)
    return true;
  else
    return false;
}
