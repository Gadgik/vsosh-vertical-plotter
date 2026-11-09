#include "Arduino.h"
void acp() {
  ADCSRA = (ADCSRA & 0xF8) | 0x04;
}

bool but(int p) {
  if (digitalRead(p) == 0) {
    delay(200);
    return 1;
  }
  return 0;
}

void load_cal() {
  EEPROM.get(EEPROM_WHITE, white);
  EEPROM.get(EEPROM_BLACK, black);
  gray = (black + white) / 2;
}

void save_cal() {
  EEPROM.put(EEPROM_WHITE, white);
  EEPROM.put(EEPROM_BLACK, black);
}

int read_calib(int n = 30) {
  long sum = 0;
  for (int i = 0; i < n; i++) {
    sum += analogRead(LINE_SENS0);
    delay(15);
  }
  return sum / n;
}

void calib() {
  digitalWrite(ENX, 1);
  digitalWrite(ENY, 1);
  while (!but(BTN_SEL)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("== CALIBRATION ==");
    display.println();
    display.println("WHITE");
    display.println();
    display.print("VAL: ");
    display.println(analogRead(LINE_SENS0));
    display.display();
    delay(80);
  }
  delay(300);
  white = read_calib();

  while (!but(BTN_SEL)) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("== CALIBRATION ==");
    display.println();
    display.println("BLACK");
    display.println();
    display.print("VAL: ");
    display.println(analogRead(LINE_SENS0));
    display.display();
    delay(80);
  }
  delay(300);
  black = read_calib();

  gray = (black + white) / 2;

  save_cal();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WHITE: ");
  display.println(white);
  display.print("BLACK:    ");
  display.println(black);
  display.print("GRAY:  ");
  display.println(gray);
  display.display();
  delay(10000);
  digitalWrite(ENX, 0);
  digitalWrite(ENY, 0);
}

void motors_block() {
  m_state = !m_state;
  digitalWrite(ENX, m_state);
  digitalWrite(ENY, m_state);
}

float cord(float val) {
  return round(val / 5.0) * 5.0;
}

double rad(float deg) {
  return deg * (PI / 180.0);
}

void led(bool state) {
  digitalWrite(LED_PIN, state);
}

void set_r() {
  digitalWrite(DIRY, 1);
}

void set_l() {
  digitalWrite(DIRY, 0);
}

void set_up() {
  digitalWrite(DIRX1, 1);
  digitalWrite(DIRX2, 0);
  delayMicroseconds(100);
}

void set_down() {
  digitalWrite(DIRX1, 0);
  digitalWrite(DIRX2, 1);
  delayMicroseconds(100);
}

void moveY(int s = 1) {
  if (s == 1) {
    digitalWrite(STEPY, HIGH);
    delayMicroseconds(sp_y1);
    digitalWrite(STEPY, LOW);
    delayMicroseconds(sp_y1);
  } else if (s == 0) {
    digitalWrite(STEPY, HIGH);
    delayMicroseconds(sp_y0);
    digitalWrite(STEPY, LOW);
    delayMicroseconds(sp_y0);
  } else {
    digitalWrite(STEPY, HIGH);
    delayMicroseconds(s);
    digitalWrite(STEPY, LOW);
    delayMicroseconds(s);
  }
}

void moveX(int sp = 1) {
  if (sp == 1) {
    digitalWrite(STEPX1, HIGH);
    digitalWrite(STEPX2, HIGH);
    delayMicroseconds(sp_x1);
    digitalWrite(STEPX1, LOW);
    digitalWrite(STEPX2, LOW);
    delayMicroseconds(sp_x1);
  } else if (sp == 0) {
    digitalWrite(STEPX1, HIGH);
    digitalWrite(STEPX2, HIGH);
    delayMicroseconds(sp_x0);
    digitalWrite(STEPX1, LOW);
    digitalWrite(STEPX2, LOW);
    delayMicroseconds(sp_x0);
  } else {
    digitalWrite(STEPX1, HIGH);
    digitalWrite(STEPX2, HIGH);
    delayMicroseconds(sp);
    digitalWrite(STEPX1, LOW);
    digitalWrite(STEPX2, LOW);
    delayMicroseconds(sp);
  }
}

void go_right(long mm) {
  set_r();
  long d = mm * ky;
  for (long i = 0; i < d; i++) moveY();
}

void go_left(long mm) {
  set_l();
  long d = mm * ky;
  for (long i = 0; i < d; i++) moveY();
}

void go_up(long mm) {
  set_up();
  long d = mm * kx;
  for (long i = 0; i < d; i++) moveX();
}

void go_down(long mm) {
  set_down();
  long d = mm * kx;
  for (long i = 0; i < d; i++) moveX();
}

bool is_line() {
  int count = 0;
  for (int i = 0; i < 3; i++) {
    if (analogRead(LINE_SENS0) > gray) count++;
  }
  return count >= 2;
}


// ПОЗИЦИОНИРОВАНИЕ
void start(float start_x = 0) {
  set_up();
  while (!is_line()) moveX(sp_xs);
  while (is_line()) moveX(sp_xs);
  go_up(d_const_x - mark_d / 2 + d_miss_x);
  cx = start_x;
}

void homeY(float home_y = 0) {
  set_l();
  while (!is_line()) moveY(sp_ys);
  go_left(d_const_y);
  cy = home_y;
}

void homeX(float home_x = 0) {
  set_down();
  while (!is_line()) moveX(sp_xs);
  while (is_line()) moveX(sp_xs);
  go_down(d_const_x - mark_d / 2 + d_miss_x);
  set_up();
  while (!is_line()) moveX(sp_xs);
  while (is_line()) moveX(sp_xs);
  go_up(d_const_x - mark_d / 2 + d_miss_x);
  cx = home_x;
}

void moveTo(float dx, float dy, bool sp = 1) {
  int sp_x, sp_y;
  if (sp) {
    sp_x = sp_x1;
    sp_y = sp_y1;
  } else {
    sp_x = sp_x0;
    sp_y = sp_y0;
  }

  float X = fabs(dx) * kx + errX;
  float Y = fabs(dy) * ky + errY;
  long stepsX = (long)X;
  long stepsY = (long)Y;
  errX = X - stepsX;
  errY = Y - stepsY;

  if (dx >= 0) set_up();
  else set_down();

  if (dy >= 0) set_r();
  else set_l();

  long Steps = max(stepsX, stepsY);
  if (Steps == 0) return;

  long Xcnt = 0;
  long Ycnt = 0;

  for (long i = 0; i < Steps; i++) {
    Xcnt += stepsX;
    Ycnt += stepsY;

    bool stepX = false;
    bool stepY = false;

    if (Xcnt >= Steps) {
      Xcnt -= Steps;
      stepX = true;
    }
    if (Ycnt >= Steps) {
      Ycnt -= Steps;
      stepY = true;
    }

    if (stepX) {
      digitalWrite(STEPX1, HIGH);
      digitalWrite(STEPX2, HIGH);
    }
    if (stepY) digitalWrite(STEPY, HIGH);

    if (stepX && stepY) {
      int minD = min(sp_x, sp_y);
      int maxD = max(sp_x, sp_y);
      delayMicroseconds(minD);
      if (sp_x <= sp_y) {
        digitalWrite(STEPX1, LOW);
        digitalWrite(STEPX2, LOW);
      } else {
        digitalWrite(STEPY, LOW);
      }
      delayMicroseconds(maxD - minD);
      if (sp_x <= sp_y) {
        digitalWrite(STEPY, LOW);
      } else {
        digitalWrite(STEPX1, LOW);
        digitalWrite(STEPX2, LOW);
      }
    } else if (stepX) {
      delayMicroseconds(sp_x);
      digitalWrite(STEPX1, LOW);
      digitalWrite(STEPX2, LOW);
    } else if (stepY) {
      delayMicroseconds(sp_y);
      digitalWrite(STEPY, LOW);
    }

    if (stepX && stepY) {
      delayMicroseconds(max(sp_x, sp_y));
    } else if (stepX) {
      delayMicroseconds(sp_x);
    } else if (stepY) {
      delayMicroseconds(sp_y);
    } else {
      delayMicroseconds(max(sp_x, sp_y));
    }
  }
}

void go(float x, float y, bool sp = 1) {
  float dx = x - cx;
  float dy = y - cy;
  moveTo(dx, dy, sp);
  cx = x;
  cy = y;
}

bool is_fill(float x, float y) {
  go(x + d_l0_x * x_pos_l, y + d_l0_y * y_pos_l);
  delay(100);
  return is_line();
}

void up() {
  delay(200);
  myServo.write(30);
  delay(200);
  myServo.write(35);
  delay(50);
}

void down() {
  delay(200);
  for (int i = 35; i < 90; i++) {
    myServo.write(i);
    delay(1);
  }
  delay(200);
}


void menu() {
  display.clearDisplay();
  display.setCursor(20, 0);
  display.print("==========");

  int sti = 0;
  int vis = 4;

  if (curs >= vis) {
    sti = curs - vis + 1;
  }

  for (int i = 0; i < vis && (sti + i) < ms; i++) {
    int idx = sti + i;
    display.setCursor(10, 16 + i * 12);
    display.print(idx == curs ? "> " : "  ");
    display.print(items[idx]);
  }

  display.display();
}

void running() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WORKING...");
  display.setCursor(0, 12);
  display.println(items[curs]);
  display.display();
}

void start_scr() {
  m_state = 1;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 0);
  display.println("=== MOTORS OFF ===");
  display.display();
  while (true) {
    if (but(BTN_SEL)) {
      digitalWrite(ENX, LOW);
      digitalWrite(ENY, LOW);
      m_state = 0;
      delay(100);
      return;
    }
  }
}