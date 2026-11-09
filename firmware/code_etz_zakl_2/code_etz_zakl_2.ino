#include <Servo.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Servo myServo;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

const int ms = 4;
const char* items[ms] = {
  "test1",
  "test2",
  "MOTOR_EN",
  "calibration"
};
int curs = 0;

int a[100][2];
int cnt = 0;

bool m_state = 1;

#define BTN_UP 31
#define BTN_DOWN 28
#define BTN_SEL 29

#define STEPX1 45
#define STEPX2 41
#define DIRX1 43
#define DIRX2 39
#define ENX 47
#define STEPY 35
#define DIRY 33
#define ENY 37

#define LED_PIN 6
#define LINE_SENS0 A0
#define SERVO_PIN 46


// размер поля
float field_X = 230;
float field_Y = 160;

float d_const_x = 29;   // расстояние при калибровке по  X
float d_const_y = 7.5;  // расстояние при калибровке по Y
float mark_d = 8.0;     // диаметр маркера
float d_miss_x = 0;     // смещение x
float d_miss_y = 0;     // смещение y

float d_l0_x = -26;  // нулевое положение датчика линии по X
float d_l0_y = 8;    // нулевое положение датчика линии по Y
float miss_x = 0.7;
float miss_y = 0.3;

float x_pos_l = 0.98;
float y_pos_l = 1;

int sp_x1 = 160;
int sp_x0 = 190;
int sp_y1 = 80;
int sp_y0 = 100;

int sp_xs = 150;  // скорости сканирования по x
int sp_ys = 50;   // скорости сканирования по y

float cx = 0, cy = 0;
float errX = 0, errY = 0;

float kx = 34.02;
float ky = 80;

#define EEPROM_WHITE 4
#define EEPROM_BLACK 6

int gray = 500;
int white = 0;
int black = 0;

#include "movement.h"

void test1() {
  led(1);
  start();
  homeY();
  delay(300);
  go(155, 5);
  down();
  go(5, 5);
  go(5, 155);
  up();
  homeX();
  go_down(180);
  led(0);
}

void test2() {
  led(1);
  go_up(200);
  delay(300);
  cx = 0, cy = 0;
  go(155, 5);
  down();
  go(5, 5);
  go(5, 155);
  up();
  go_down(200);
  led(0);
}

void exec(int id) {
  switch (id) {
    case 0:
      {
        test1();
        break;
      }
    case 1:
      {
        test2();
        break;
      }
    case 2:
      {
        motors_block();
        break;
      }
    case 3:
      {
        calib();
        break;
      }
  }
}

void setup() {
  acp();

  Serial.begin(9600);
  Serial2.begin(9600);

  pinMode(STEPX1, OUTPUT);
  pinMode(DIRX1, OUTPUT);
  pinMode(STEPX2, OUTPUT);
  pinMode(DIRX2, OUTPUT);
  pinMode(STEPY, OUTPUT);
  pinMode(DIRY, OUTPUT);
  pinMode(ENX, OUTPUT);
  pinMode(ENY, OUTPUT);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SEL, INPUT_PULLUP);

  digitalWrite(ENX, HIGH);
  digitalWrite(ENY, HIGH);

  pinMode(LINE_SENS0, INPUT);
  pinMode(LED_PIN, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();

  load_cal();

  myServo.attach(SERVO_PIN);
  up();

  start_scr();
  menu();
}

void loop() {
  if (but(BTN_UP)) {
    curs--;
    if (curs < 0) curs = ms - 1;
    menu();
  }

  if (but(BTN_DOWN)) {
    curs++;
    if (curs >= ms) curs = 0;
    menu();
  }

  if (but(BTN_SEL)) {
    bool st = 1;
    while (!but(BTN_SEL)) {
      display.clearDisplay();
      display.setCursor(35, 0);
      display.print("CONTINUE?");
      display.setCursor(20, 8 * 4);
      display.print(st == 1 ? "<YES>" : "YES");
      display.setCursor(85, 8 * 4);
      display.print(st == 0 ? "<NO>" : "NO");
      if (but(BTN_UP) | but(BTN_DOWN)) {
        st = !st;
      }
      display.display();
    }
    if (st) {
      running();
      led(1);
      exec(curs);
      led(0);
    }
    menu();
  }
}
