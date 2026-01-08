#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

#define FIRE_CONFIRM_TIME 300
#define MQ2_MARGIN 80      

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo gate;

// ---------------- PINS ----------------
#define IR_ENTRY 22
#define GREEN_LED 30
#define RED_LED   31
#define WHITE_LED 32
#define MQ2    A0
#define BUZZER 34
#define RELAY  35
#define SERVO_PIN 9

// ---------------- KEYPAD ----------------
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {36,37,38,39};
byte colPins[COLS] = {40,41,42,43};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ---------------- VARIABLES ----------------
bool slot1=false, slot2=false, slot3=false;
String inputID="";

bool entryMode=false;
bool exitMode=false;

bool mq2Ready=false;
bool fireActive=false;

unsigned long mq2StartTime;
unsigned long fireDetectTime=0;

int mq2Baseline = 0; 

// Siren timing
unsigned long sirenTimer=0;
bool sirenState=false;

// ---------------- SETUP ----------------
void setup(){
  pinMode(IR_ENTRY, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);

  gate.attach(SERVO_PIN);
  gate.write(0);

  lcd.init();
  lcd.backlight();

  lcd.print("WARMING SENSOR");
  lcd.setCursor(0,1);
  lcd.print("PLEASE WAIT");

  mq2StartTime = millis();
}
void loop()
{

  digitalWrite(WHITE_LED, digitalRead(IR_ENTRY)==LOW);

  // ---------- MQ-2 WARM UP ----------
  if(!mq2Ready){
    if(millis() - mq2StartTime >= 30000){
      mq2Baseline = analogRead(MQ2);
      mq2Ready = true;
      showIdle();
    }
    return;
  }

  // ---------- FIRE DETECTION ----------
  int mq2Value = analogRead(MQ2);

  if(mq2Value > mq2Baseline + MQ2_MARGIN){
    if(fireDetectTime == 0) fireDetectTime = millis();
    if(millis() - fireDetectTime >= FIRE_CONFIRM_TIME){
      fireActive = true;
    }
  } else {
    fireDetectTime = 0;
    fireActive = false;
    noTone(BUZZER);
    digitalWrite(RED_LED, LOW);
    digitalWrite(RELAY, LOW);
  }

  // ---------- FIRE MODE ----------
  if(fireActive){
    digitalWrite(RED_LED, HIGH);
    digitalWrite(RELAY, HIGH);

    lcd.setCursor(0,0);
    lcd.print("FIRE  FIRE     ");
    lcd.setCursor(0,1);
    lcd.print("GO OUT         ");

    if(millis() - sirenTimer > (sirenState ? 500 : 300)){
      sirenTimer = millis();
      sirenState = !sirenState;
      tone(BUZZER, sirenState ? 1000 : 1400);
    }
    return;
  }

  // ---------------- KEYPAD ----------------
  char key = keypad.getKey();
  if(!key) return;

  if((key=='A'||key=='B'||key=='C'||key=='D') && !exitMode){
    if(freeSlots()==0){
      lcd.clear();
      lcd.print("SLOT FULL");
      lcd.setCursor(0,1);
      lcd.print("EXIT PRESS #");
      return;
    }
    entryMode = true;
    inputID = "";
    lcd.clear();
    lcd.print("SLOTS FREE:");
    lcd.print(freeSlots());
    lcd.setCursor(0,1);
    lcd.print("ENTER ID:");
  }

  else if(key=='#'){
    exitMode = true;
    entryMode = false;
    inputID = "";
    lcd.clear();
    lcd.print("EXIT MODE");
    lcd.setCursor(0,1);
    lcd.print("ENTER ID:");
  }

  else if(isDigit(key)){
    inputID += key;
    lcd.setCursor(10,1);
    lcd.print(inputID);
  }

  if(inputID.length()==4){
    if(entryMode) entryProcess(inputID);
    else if(exitMode) exitProcess(inputID);
    inputID="";
    entryMode=false;
    exitMode=false;
  }
}

// ---------------- FUNCTIONS ----------------
int freeSlots(){
  return (!slot1) + (!slot2) + (!slot3);
}

int idToSlot(String id){
  if(id=="2222") return 1;
  if(id=="1111") return 2;
  if(id=="3333") return 3;
  return 0;
}

void entryProcess(String id){
  int s = idToSlot(id);
  if(s==0 || (s==1&&slot1)||(s==2&&slot2)||(s==3&&slot3)){
    digitalWrite(RED_LED, HIGH);
    tone(BUZZER, 800);
    lcd.clear();
    lcd.print("WRONG ID");
    lcd.setCursor(0,1);
    lcd.print("TRY AGAIN");
    delay(1000);
    noTone(BUZZER);
    digitalWrite(RED_LED, LOW);
    showIdle();
    return;
  }

  if(s==1) slot1=true;
  if(s==2) slot2=true;
  if(s==3) slot3=true;

  digitalWrite(GREEN_LED, HIGH);
  gate.write(90);

  lcd.clear();
  lcd.print("ACCESS ALLOWED");
  lcd.setCursor(0,1);
  lcd.print("GO TO SLOT ");
  lcd.print(s);

  delay(2000);
  gate.write(0);
  digitalWrite(GREEN_LED, LOW);
  showIdle();
}

void exitProcess(String id){
  int s = idToSlot(id);
  if(s==1) slot1=false;
  if(s==2) slot2=false;
  if(s==3) slot3=false;

  lcd.clear();
  lcd.print("SLOT ");
  lcd.print(s);
  lcd.print(" FREE");
  delay(2000);
  showIdle();
}

void showIdle(){
  lcd.clear();
  lcd.print("SYSTEM READY");
  lcd.setCursor(0,1);
  lcd.print("ENTRY:A EXIT:#");
}

