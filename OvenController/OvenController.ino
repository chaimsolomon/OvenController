#include <EEPROM.h>
#include <TimerOne.h>
#include <PID_v1.h>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <max6675.h>

// green
#define pin_top 13
// yellow
#define pin_bottom 12
// white
#define pin_fan 11
// gray
#define pin_light 8 

#define max_menu_state 3
#define max_oven_state 3

#define eepr_oven_state_addr 0
#define eepr_setpoint_addr 10
#define eepr_topbottom_addr 20
#define eepr_kp_addr 30
#define eepr_ki_addr 40
#define eepr_kd_addr 50

LiquidCrystal_I2C lcd(0x27,16,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

int thermoDO = A2;
int thermoCS = A1;
int thermoCLK = A0;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

int btn1, btn2, btn3, btn4, btn5, btn6, btn7;

int changed = 0;
int menu=0;
int volatile oven_state = 0;
double top_bottom = 0;
int volatile pwm_top = 0;
int volatile pwm_bottom = 0;
int volatile top_pwm_sp = 0;
int volatile bottom_pwm_sp = 0;
double kp=1.5;
double ki=0.25;
double kd=0;
long clock=0;
// PID
double Setpoint = 170.0, Input=20.0, Output=2.0, Difference=0;
//PID(&Input, &Output, &Setpoint, Kp, Ki, Kd, Direction)
//PID myPID(&Input, &Output, &Setpoint, kp, ki, kd, DIRECT);

int volatile timerstate = 0;

void timercallback(){

  pwm_top++;
  if (pwm_top > 99) pwm_top = 0;
  
  pwm_bottom--;
  if (pwm_bottom <= 0) pwm_bottom = 100;
  
  if ((oven_state >= 2) && (pwm_top < top_pwm_sp) && (clock != 0) && (Input <= 250)) {digitalWrite(pin_top, 1);} else {digitalWrite(pin_top, 0);}
  if ((oven_state >= 2) && (pwm_bottom < bottom_pwm_sp) && (clock != 0&& (Input <= 250))) {digitalWrite(pin_bottom, 1);} else {digitalWrite(pin_bottom, 0);}
  if ((oven_state == 3) && (Input <= 250) && (clock != 0)) {digitalWrite(pin_fan, 1);} else {digitalWrite(pin_fan, 0);}
  if ((oven_state > 0) && (clock != 0)) {digitalWrite(pin_light, 1);} else {digitalWrite(pin_light, 0);}
  
  
  switch (timerstate) {
    case 0:  
    if (clock > 0) clock--;
    break;
    case 1:
      Input = thermocouple.readCelsius();
    break;
    case 2:
      Difference = (Setpoint - Input);
      if (Difference > 10) Difference = 10.0;
      if (Difference < 0) Difference = 0;
      Output = 20 * Difference;
    break;
    case 3:
    break;
    case 4:
      top_pwm_sp = (1.0 + top_bottom) * (Output/2);
      bottom_pwm_sp = (1.0 - top_bottom) * (Output/2);
      if (top_pwm_sp > 100) {
        bottom_pwm_sp += (top_pwm_sp - 100);
        top_pwm_sp = 100;
        };
      if (bottom_pwm_sp > 100) {
        top_pwm_sp += (bottom_pwm_sp - 100);
        bottom_pwm_sp = 100;
        };
      if (top_pwm_sp > 100) {
        top_pwm_sp = 100;
        };
    break;
    case 5:
    break;
    case 6:
    break;
    case 7:
    break;
    case 8:
    break;
    case 9:
    default: 
    timerstate = -1;
    break;
    }
    timerstate++;
}

void setup() {
EEPROM.get(eepr_oven_state_addr, oven_state);
EEPROM.get(eepr_setpoint_addr, Setpoint);
EEPROM.get(eepr_topbottom_addr, top_bottom);
EEPROM.get(eepr_kp_addr, kp);
EEPROM.get(eepr_ki_addr, ki);
EEPROM.get(eepr_kd_addr, kd);

  pwm_top = 0;
  pwm_bottom = 0;
  top_pwm_sp = 0;
  bottom_pwm_sp = 0;

  changed = 0;

  menu = 0;
  lcd.init(); // initialize the lcd 
//  myPID.SetMode(AUTOMATIC);
//  myPID.SetOutputLimits(0, 200);
//  myPID.SetSampleTime(1000);

//  myPID.SetTunings(kp, ki, kd);
  timerstate = 0;
  Timer1.initialize(100000);
  Timer1.attachInterrupt(timercallback);

  lcd.init();
  // Print a splash screen message to the LCD.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Oven Thermostat");
  lcd.setCursor(0,1);
  lcd.print("Initializing");
  lcd.setCursor(0,2);
  Input = thermocouple.readCelsius();
  lcd.print(Input);
  
  lcd.setCursor(0,0);

  pinMode(A9, INPUT);
  pinMode(A10, INPUT);
  pinMode(A11, INPUT);
  pinMode(A12, INPUT);
  pinMode(A13, INPUT);
  pinMode(A14, INPUT);
  pinMode(A15, INPUT);
  pinMode(pin_top, OUTPUT);
  pinMode(pin_bottom, OUTPUT);
  pinMode(pin_fan, OUTPUT);
  pinMode(pin_light, OUTPUT);
}

void loop() {
//  myPID.Compute();

   lcd.clear();
  
  lcd.setCursor(0,0);
  switch (oven_state) {
    case 0:
      lcd.print("off      ");
      break;
    case 1:
      lcd.print("light on ");
      break;
    case 2:
      lcd.print("Oven on no fan");
      break;
    case 3:
      lcd.print("Oven on with fan ");
      break;
    default:
      oven_state = 0;
      break;
  }
  lcd.setCursor(0,1);
  lcd.print("        ");
  lcd.setCursor(0,1);
  //lcd.print(clock);
  if (clock >= 0) {
    lcd.print(clock/3600);
    lcd.print(":");
    lcd.print((clock/60)%60);
    lcd.print(":");
    lcd.print(clock%60);  
  }
  else {
    lcd.print("No timer");
  }
  lcd.print(" ");

  lcd.print(Setpoint);
  lcd.print(" deg C");
  lcd.setCursor(0,2);

  lcd.print(Input);
  lcd.print(" ");
  
  lcd.print(top_bottom);
  lcd.print("   ");
  lcd.print(top_pwm_sp);
  lcd.print(" ");
  lcd.print(bottom_pwm_sp);

  lcd.setCursor(0,3);
  switch (menu) {
  case 0:
  lcd.print("Mode");
  break;
  case 1:
  lcd.print("Upper/lower heat");
  break;
  case 2:
  lcd.print("Timer set");
  break;
  case 3:
  lcd.print("RESET");
  break;
  default:
  menu = 0;
  }


  btn1 = !digitalRead(A9);
  btn2 = !digitalRead(A10);
  btn3 = !digitalRead(A11);
  btn4 = !digitalRead(A12);
  btn5 = !digitalRead(A13);
  btn6 = !digitalRead(A14);
  btn7 = !digitalRead(A15);

  changed = 0;
  if (btn1) {
    Setpoint += 5;
    changed = 1;
    };  
  if (btn2) {
    Setpoint -= 5;
    changed = 1;
    };  
  if (Setpoint > 250) {Setpoint = 250; changed = 1;}
  if (Setpoint < 20) {Setpoint = 20; changed = 1;}
  if (changed == 1) EEPROM.put(eepr_setpoint_addr, Setpoint);

  if (btn5) {
    menu++;
    if (menu > max_menu_state) menu = 0;
  }
 
  if (btn3) {
    menu--;
    if (menu < 0) menu = max_menu_state;
  }

  switch (menu) {
    case 0: //Mode
      changed = 0;
      if (btn7) {oven_state++; changed = 1;}
      if (btn6) {oven_state--; changed = 1;}
      if (btn4) {
        if (oven_state > 1) {oven_state = 0; changed = 1;}
        }
      if (oven_state > max_oven_state) {oven_state = 0; changed = 1;}
      if (oven_state < 0) {oven_state = max_oven_state; changed = 1;}
      if (changed == 1) EEPROM.put(eepr_oven_state_addr, oven_state);
    break;
    case 1: //Upper/lower heat
      changed = 0;
      if (btn7) {top_bottom += 0.1; changed = 1;}
      if (btn6) {top_bottom -= 0.1; changed = 1;}
      if (top_bottom > 1) {top_bottom = 1; changed = 1;}
      if (top_bottom < -1) {top_bottom = -1; changed = 1;}
      if (changed == 1) EEPROM.put(eepr_topbottom_addr, top_bottom);      
    break;
    case 2: //Timer set
      if (btn7) {clock += 60;}
      if (btn6) {clock -= 60;}
      if (clock < -1) clock = -1;
      if (clock > 3600*5) clock = 3600*5;
    break;
    case 3: //RESET all
      if (btn4) {
          clock = 0;
          Setpoint = 180;
          EEPROM.put(eepr_setpoint_addr, Setpoint);
          top_bottom = 0;
          EEPROM.put(eepr_topbottom_addr, top_bottom);
//          myPID.SetTunings(kp, ki, kd);
          
          oven_state = 0;
          
        }
    break;
    default:
    menu = 0;
    break;
  }
  
    delay(50);
}
