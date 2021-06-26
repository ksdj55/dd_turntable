
//Direct Drive Turntable using Arduino Uno and TMC2209
//Created by: Kasidej Khunvattanakarn @ 2020-2021
//Licensed under GNU General Public License v3.0
//https://github.com/ksdj55/dd_turntable

#define LED_PIN 13
#define M_EN 6
#define M_ST 5
#define M_DR 4
#define PW_SW 8

bool start = false;
bool clock_start = false;
bool sw_state = true;
bool sw_read = true;
long sw_downtime = 0;
int set_speed = 0;
bool action = false;
boolean toggle1 = 0;
long setspeed = 0;
long startspeed = 65000;
long currentspeed = startspeed;
float acc_rate = 0.005;
float acc_pow = 20;
long rpm33speed = 8960;
long rpm45speed = 6632;
long rpm78speed = 3626;

void setup() {
  //Serial.begin(9600);
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Started");
  pinMode(LED_PIN, OUTPUT);
  pinMode(M_ST, OUTPUT);
  pinMode(M_DR, OUTPUT);
  pinMode(M_EN, OUTPUT);
  pinMode(PW_SW, INPUT_PULLUP);
  digitalWrite(M_EN, HIGH);
  digitalWrite(M_DR, HIGH);
  start=false;
  
  cli();//stop interrupts
  
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  //OCR1A = 8955;//8955;//9000;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  setspeed = 8960;
  OCR1A = startspeed;
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  sei();//allow interrupts
}

ISR(TIMER1_COMPA_vect){//timer1 interrupt 1Hz toggles pin 13 (LED)
//generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)
  if(clock_start) {
    if (toggle1){
      digitalWrite(M_ST,HIGH);
      toggle1 = 0;
    }else{
      digitalWrite(M_ST,LOW);
      toggle1 = 1;
    }
  }
}


void loop() {
  sw_read = digitalRead(PW_SW);
  if(!sw_read && sw_state) { //SW down
    //start = !start;
    if(start) {
      stop_motor();
    } else {
      setspeed = rpm33speed;
      start_motor();
    }
    delay(500);
  }
  sw_state = sw_read;

  handle_acc();
  delay(50);

  if(Serial.available() > 0) {
    setspeed = Serial.parseInt();
    if(setspeed > 0) {
      if(!start) {
        currentspeed = startspeed;
      }
      if(!start) 
        start_motor();
    } else {
      if(start)
        stop_motor();
    }
  }
}

void start_motor() {
  start = true;
  digitalWrite(M_EN, LOW);
  currentspeed = startspeed;
  clock_start = true;
}

void stop_motor() {
  start = false;
  setspeed = startspeed;
}

void handle_acc() {
  float acc_mlp = 1.0 + (((float)currentspeed / startspeed) * acc_pow);
  if(setspeed > currentspeed) { //DECELERATE
    if((currentspeed + (currentspeed * acc_rate * acc_mlp)) >= setspeed) {
      currentspeed = setspeed;
    }else{
      currentspeed += currentspeed * acc_rate * acc_mlp;
    }
    if(currentspeed >= startspeed && !start) {
        digitalWrite(M_EN, HIGH);
        clock_start = false;
    }
    OCR1A = currentspeed;
    Serial.println((String) currentspeed);
  } else if (setspeed < currentspeed && start) { //ACCELERATE
    if((currentspeed - (currentspeed * acc_rate  * acc_mlp)) <= setspeed) {
      currentspeed = setspeed;
    }else {
      currentspeed -= currentspeed * acc_rate * acc_mlp;
    }
    OCR1A = currentspeed;
    Serial.println((String) currentspeed);
  }
}
