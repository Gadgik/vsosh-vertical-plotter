// ========== БИБЛИОТЕКИ ==========
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// ========== НАСТРОЙКИ ==========
Servo myServo;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define STEP_PIN 18
#define DIR_PINL 26
#define DIR_PINR 27
#define LINE_SENS 34
#define SERVO_PIN 13

#define THRESHOLD 4095

int const_d = 39;     // расстояние от оси крепления маркера до датчика
float mark_d = 12.0;  // диаметр маркера
float miss_size = 1;  // смещение
int sp_1 = 300;
int sp_0 = 600;

float s = 17;

#define BTN_UP 32
#define BTN_OK 33
#define BTN_DOWN 25

#define LED_PIN 23

#define MENU_ITEMS 6
int menuOffset = 0;

// ========== ФУНКЦИИ ДВИЖЕНИЯ ==========
void set_up() {
  digitalWrite(DIR_PINL, 1);
  digitalWrite(DIR_PINR, 0);
}

void set_down() {
  digitalWrite(DIR_PINL, 0);
  digitalWrite(DIR_PINR, 1);
}

void move(int sp = 1) {
  if (sp == 1) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(sp_1);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(sp_1);
  } else {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(sp_0);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(sp_0);
  }
}

void go_up(long mm) {
  set_up();
  long d = mm * s;
  for (long i = 0; i < d; i++) move();
}

void go_down(long mm) {
  set_down();
  long d = mm * s;
  for (long i = 0; i < d; i++) move();
}

bool is_line() {
  return digitalRead(LINE_SENS)==0;
}

// ========== ПОЗИЦИОНИРОВАНИЕ ==========
void find_line_start() {
  set_up();
  while (!is_line()) move();
  while (is_line()) move(0);
  go_up(const_d + mark_d / 2 + miss_size);
}

void homing() {
  set_down();
  while (!is_line()) move();
  while (is_line()) move(0);
  set_up();
  while (!is_line()) move();
  while (is_line()) move(0);
  go_up(const_d + mark_d / 2 + miss_size);
}

// ========== РИСОВАНИЕ ==========
void up() {
  delay(300);
  myServo.write(0);
  delay(300);
}

void down() {
  delay(300);
  for (int angle = 0; angle <= 65; angle++) {
    myServo.write(angle);
    delay(2);
  }
  delay(300);
}

void set_mark() {
  myServo.write(50);
  delay(100);
  myServo.write(45);
}

void point() {
  down();
  delay(200);
  up();
}

// ========== ДИСПЛЕЙ ==========
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* menuItems[MENU_ITEMS] = {
  "FUNC_1",
  "FUNC_2",
  "FUNC_3",
  "FUNC_4",
  "FUNC_5",
  "FUNC_6"
};

enum State {
  STATE_MENU,
  STATE_CONFIRM,
  STATE_RUNNING
};

State currentState = STATE_MENU;
int selectedItem = 0;
int confirmSelection = 0;

unsigned long lastBtnUp = 0;
unsigned long lastBtnOk = 0;
unsigned long lastBtnDown = 0;
const int debounceDelay = 200;

// ========== SETUP ==========
void setup() {
  Wire.begin(21, 22);
  Serial.begin(115200);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PINL, OUTPUT);
  pinMode(DIR_PINR, OUTPUT);
  pinMode(LINE_SENS, INPUT);

  myServo.attach(SERVO_PIN);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(25, 20);
  display.println("LOADING...");
  display.display();
  delay(1500);

  drawMenu();
}

// ========== LOOP ==========
void loop() {
  handleButtons();
  if (currentState == STATE_RUNNING) {
    runFunction(selectedItem);
  }
}

// ========== КНОПКИ ==========
void handleButtons() {
  unsigned long now = millis();

  if (!digitalRead(BTN_UP) && now - lastBtnUp > debounceDelay) {
    lastBtnUp = now;
    onButtonUp();
  }
  if (!digitalRead(BTN_OK) && now - lastBtnOk > debounceDelay) {
    lastBtnOk = now;
    onButtonOk();
  }
  if (!digitalRead(BTN_DOWN) && now - lastBtnDown > debounceDelay) {
    lastBtnDown = now;
    onButtonDown();
  }
}

void onButtonUp() {
  if (currentState == STATE_MENU) {
    selectedItem--;
    if (selectedItem < 0) selectedItem = MENU_ITEMS - 1;
    drawMenu();
  } else if (currentState == STATE_CONFIRM) {
    confirmSelection = 1 - confirmSelection;
    drawConfirm();
  }
}

void onButtonDown() {
  if (currentState == STATE_MENU) {
    selectedItem++;
    if (selectedItem >= MENU_ITEMS) selectedItem = 0;
    drawMenu();
  } else if (currentState == STATE_CONFIRM) {
    confirmSelection = 1 - confirmSelection;
    drawConfirm();
  }
}

void onButtonOk() {
  if (currentState == STATE_MENU) {
    currentState = STATE_CONFIRM;
    confirmSelection = 0;
    drawConfirm();
  } else if (currentState == STATE_CONFIRM) {
    if (confirmSelection == 0) {
      currentState = STATE_RUNNING;
      drawRunning();
    } else {
      currentState = STATE_MENU;
      drawMenu();
    }
  }
}

// ========== ОТРИСОВКА ==========
void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(34, 0);
  display.println("== MENU ==");

  int visible = 4;

  if (selectedItem < menuOffset) menuOffset = selectedItem;
  if (selectedItem >= menuOffset + visible) menuOffset = selectedItem - visible + 1;

  for (int i = 0; i < visible; i++) {
    int idx = menuOffset + i;
    if (idx >= MENU_ITEMS) break;

    int y = 14 + i * 12;

    if (idx == selectedItem) {
      display.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }

    display.setCursor(2, y);
    display.print(idx + 1);
    display.print(". ");
    display.println(menuItems[idx]);
  }

  display.setTextColor(SSD1306_WHITE);
  display.display();
}

void drawConfirm() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("Start function?");
  display.setCursor(0, 10);
  display.println(menuItems[selectedItem]);

  if (confirmSelection == 0) {
    display.fillRect(10, 30, 40, 14, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRect(10, 30, 40, 14, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(20, 33);
  display.println("YES");

  if (confirmSelection == 1) {
    display.fillRect(70, 30, 40, 14, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRect(70, 30, 40, 14, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(82, 33);
  display.println("NO");

  display.setTextColor(SSD1306_WHITE);
  display.display();
}


void drawRunning() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WORKING...");
  display.setCursor(0, 12);
  display.println(menuItems[selectedItem]);
  display.display();
}

void led_on() {
  digitalWrite(LED_PIN, HIGH);
}

// ========== СЛУЖЕБНЫЕ ==========
void finishFunction() {
  digitalWrite(LED_PIN, LOW);
  currentState = STATE_MENU;
  drawMenu();
}

// ========== ФУНКЦИИ ==========
void runFunction(int funcNum) {

  switch (funcNum) {
    case 0:  // Функция 1
      led_on();
      finishFunction();
      break;
    case 1:  // Функция 2
      led_on();
      find_line_start();
      point();
      go_up(50);
      point();
      go_up(50);
      point();
      go_up(50);
      point();
      go_up(50);
      point();
      homing();
      go_down(150);
      finishFunction();
      break;

    case 2:  // Функция 3
      find_line_start();
      delay(3000);
      go_up(10);
      down();
      go_up(20);
      up();

      delay(3000);
      go_up(30);
      down();
      go_up(20);
      up();

      delay(3000);
      go_up(30);
      down();
      go_up(20);
      up();

      delay(3000);
      go_up(30);
      down();
      go_up(20);
      up();

      // homing();
      go_down(150 + 160);

      finishFunction();
      break;

    case 3:  // Функция 4
      led_on();
      find_line_start();
      go_up(100);
      homing();
      finishFunction();
      break;

    case 4:  // Функция 5
      led_on();
      find_line_start();
      finishFunction();
      break;

    case 5:  // Функция 6
      led_on();
      homing();
      finishFunction();
      break;
  }

  delay(100);
}