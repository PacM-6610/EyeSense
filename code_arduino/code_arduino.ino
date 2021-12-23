#include <HX711.h>
#include <SoftwareSerial.h>
bool VERBOSE = false;
bool printGUI = true;
// PINS
// Vibrators
int latchPin = 5;  // Latch pin of 74HC595 is connected to Digital pin 5
int clockPin = 3; // Clock pin of 74HC595 is connected to Digital pin 6
int dataPin = 4;  // Data pin of 74HC595 is connected to Digital pin 4
byte vibrators = 0;    // Variable to hold the pattern of which vibrators are currently turned on or off

// US sensors
const int nb_US = 7;
const int trigPin = 10;//to 10
const int pingPin1 = 6;
const int pingPin2 = 7;
const int pingPin3 = 8;
const int pingPin4 = 9;
const int pingPin5 = 14; // A0
const int pingPin6 = 15; // A1
const int pingPin7 = 16; // A2

unsigned long timeout = 100000;//[microseconde]24ms pour 4 mètre. Garder 100ms
long duration[nb_US];
unsigned int vUS[nb_US];//[]
unsigned int vUS_actual; //minimum distance detected [cm]. =350 if too close or too far
unsigned int US_index; //index of the US sensor detecting the minimum distance. 100, if no US detects
unsigned int dir_obstacle = 0; // dir of the closest obstacle
#define NODETECTION  100 //When no obstacle detected
const float MaxUS = 2;//[m]
const float MaxUS_cm = MaxUS * 100; //[m]
const float MinUS = 0;//[m]

//Motor
const int controlPin1 = 13;// INA2
const int controlPin2 = 12;// INA1
const int enablePin = 11;
int motorEnabled = 1;
int motorSpeed = 0;
int motorDirection = 1;

//Button
int interruptPin = 2;
unsigned long startDebounceTime = 0;
unsigned long debounceDelay = 1000;

//Load Cell
#define DOUT  17 //A3
#define CLK  18 //A4
HX711 scale;
float calibration_factor = 3751; //-7050 worked for my 440lb max scale setup
float vLC = 0; //[N]
const float MaxLC = 20;//[N]
const float MinLC = 1;//[N]

// Plug pins Loadcell
// E+: red
// E-: black
// A-: white
// A+: green

//Calculation
float MAXSPEEDMOTOR = 255;
float Kp = 200; //PD-controller
float error  = 0;
const float Ratio_US_LC = (MaxUS - MinUS) / (MaxLC - MinLC);
float MinThightening = 0;//Ratio_US_LC*Kp;

//PC communication
String PCdata;
char d1;
unsigned long sendDataTimerStart = 0;
const unsigned long sendDataInterval = 500; //[milliseconds]
bool stopVibrators = false;
bool STOP = false;
bool motorBackOnSTOP = false;

void setup() {
  Serial.begin(9600);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), on_STOP_button_pushed, RISING);
  
  //Motor
  pinMode(controlPin1, OUTPUT);
  pinMode(controlPin2, OUTPUT);
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, LOW);
  motorSpeed = 0;

  //Load Cell
  scale.begin(DOUT, CLK);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  long zero_factor = scale.read_average(); //Get a baseline reading

  //Ultrasonic
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  pinMode(trigPin, OUTPUT);
  pinMode(pingPin1, INPUT);
  pinMode(pingPin2, INPUT);
  pinMode(pingPin3, INPUT);
  pinMode(pingPin4, INPUT);
  pinMode(pingPin5, INPUT);
  pinMode(pingPin6, INPUT);
  pinMode(pingPin7, INPUT);

  //Vibrators
  // Set all the pins of 74HC595 (Shift Register) as OUTPUT
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  //PC communication
  sendDataTimerStart = millis();
}


void loop() {
  //Read PC data
  if (printGUI == true) {
    read_PC_data();
  }

  if (STOP == false) {
    //US sensors
    read_distance_US(false);
    find_minimum_distance_direction();

    //Load Cell
    read_LoadCell_value();

    //Motor control loop
    control_loop();
  }

  // Vibrate
  vibrate_to_indicate_direction();

  //Motor
  send_motor_speed();

  //Print
  if (VERBOSE) {
    Serial.print(STOP);
    Serial.print("Min: ");
    Serial.print(vUS_actual, DEC);
    Serial.print("cm ");
    Serial.print("| Vib number : ");
    Serial.print(dir_obstacle);
    Serial.print(" | F:"); //US0
    Serial.print(vUS[0]);
    Serial.print("cm  F-R:");
    Serial.print(vUS[1]);
    Serial.print("cm  R:");
    Serial.print(vUS[2]);
    Serial.print("cm  B-R:");
    Serial.print(vUS[3]);
    Serial.print("cm  B-L:");
    Serial.print(vUS[4]);
    Serial.print("cm  L:");
    Serial.print(vUS[5]);
    Serial.print("cm  F-L:");
    Serial.print(vUS[6]);
    Serial.print("cm");
    Serial.print(" | LoadCell: ");
    Serial.print(vLC, 3);
    Serial.print(" | Error: ");
    Serial.print(error);
    Serial.print(" | MotorSpeed: ");
    Serial.println(motorSpeed);
  }

}

/*
   updateShiftRegister() - This function sets the latchPin to low, then calls the Arduino function 'shiftOut'
   to shift out contents of variable 'vibrators' in the shift register before putting the 'latchPin' high again.
*/
void updateShiftRegister() {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, vibrators);
  digitalWrite(latchPin, HIGH);
}

/*
   Transform LoadCell value from Newton to meters
*/
float LoadCell_to_USsensor() {
  return Ratio_US_LC * vLC;
}

/*
   Read LoadCell and stock current value
*/
void read_LoadCell_value() {
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  vLC = scale.get_units() / 10;
  if (vLC <= 0) {
    vLC = 0;
  }
}

/*
   Vibrate in the corresponding obstacle direction
*/
void vibrate_to_indicate_direction() {
  int vibrator_number = 0;              // choose which vibrator to connect.
  vibrators = 0; // Initially turns all the vibrators off, by giving the variable 'vibrators' the value 0
  updateShiftRegister();
  if (stopVibrators || STOP) {
    dir_obstacle = NODETECTION;
  }
  // Make the corresponding vibrator vibrate
  switch (dir_obstacle) { // choose which vibrator to connect
    case 0:
      // statements
      vibrator_number = 1;                  // choose which vibrator to connect.
      bitSet(vibrators, vibrator_number);   // Set the bit that controls that vibrator in the variable 'vibrators'
      vibrator_number = 6;
      bitSet(vibrators, vibrator_number);
      updateShiftRegister();
      break;

    case 1:
      // statements
      vibrator_number = dir_obstacle;       // choose which vibrator to connect.
      bitSet(vibrators, vibrator_number);   // Set the bit that controls that vibrator in the variable 'vibrators'
      updateShiftRegister();
      break;

    case 2:
      // statements
      vibrator_number = dir_obstacle;       // choose which vibrator to connect.
      bitSet(vibrators, vibrator_number);   // Set the bit that controls that vibrator in the variable 'vibrators'
      updateShiftRegister();
      break;

    case 3:
      // statements
      vibrator_number = dir_obstacle;       // choose which vibrator to connect.
      bitSet(vibrators, vibrator_number);   // Set the bit that controls that vibrator in the variable 'vibrators'
      updateShiftRegister();
      break;

    case 4:
      // statements
      vibrator_number = dir_obstacle;       // choose which vibrator to connect.
      bitSet(vibrators, vibrator_number);   // Set the bit that controls that vibrator in the variable 'vibrators'
      updateShiftRegister();
      break;

    case 5:
      // statements
      vibrator_number = dir_obstacle;       // choose which vibrator to connect.
      bitSet(vibrators, vibrator_number);   // Set the bit that controls that vibrator in the variable 'vibrators'
      updateShiftRegister();
      break;

    case 6:
      // statements
      vibrator_number = dir_obstacle;       // choose which vibrator to connect.
      bitSet(vibrators, vibrator_number);   // Set the bit that controls that vibrat or in the variable 'vibrators'
      updateShiftRegister();
      break;

    case NODETECTION:
      vibrators = 0; // Initially turns all the vibrators off, by giving the variable 'vibrators' the value 0
      updateShiftRegister();
      break;

      //default:
      //Serial.print("Error Index number : (In switch cases in function vibrate_to_indicate_direction)");
  }
}

/*
   Find the US sensor with the smallest value and indicate its index
*/
void find_minimum_distance_direction() {
  // Find minimum distance and extract direction
  vUS_actual = MaxUS_cm;
  US_index = NODETECTION;
  for ( int idx = 0; idx < nb_US; idx++ ) {
    if ( vUS[idx] < MaxUS_cm && vUS_actual > vUS[idx]) {
      vUS_actual = vUS[idx];
      US_index = idx;
    }
  }
  dir_obstacle = US_index;
}

/*
   P-controller for the tightening of the belt
*/
void control_loop() {
  error =  (MaxUS - LoadCell_to_USsensor()) - vUS_actual / 100;
  motorSpeed = Kp * error + MinThightening;
  if (motorSpeed > MAXSPEEDMOTOR) {
    motorSpeed = MAXSPEEDMOTOR;
  }
  else if (motorSpeed < -MAXSPEEDMOTOR) {
    motorSpeed = -MAXSPEEDMOTOR;
  }
}

/*
   Send desired motor speed to the motor
*/
void send_motor_speed() {
  if (STOP && motorBackOnSTOP) {
    motorSpeed = -255;
  } else if (STOP && !motorBackOnSTOP) {
    motorSpeed = 0;
  }

  if (motorSpeed >= 0) {
    digitalWrite(controlPin1, HIGH);
    digitalWrite(controlPin2, LOW);
  } else {
    digitalWrite(controlPin1, LOW);
    digitalWrite(controlPin2, HIGH);
  }

  if (motorEnabled == 1) {
    analogWrite(enablePin, abs(motorSpeed));
  } else {
    analogWrite(enablePin, 0);
  }

  if (STOP && motorBackOnSTOP) {
    delay(1500);
    motorBackOnSTOP = false;
  }
}

/*
   Activate US-sensor and read corresponding distances
*/
void read_distance_US(bool print_US) {
  // establish variables for duration of the ping,
  // and the distance result in inches and centimeters:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);
  duration[0] = pulseIn(pingPin1, HIGH, timeout);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);
  duration[1] = pulseIn(pingPin2, HIGH, timeout);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);
  duration[2] = pulseIn(pingPin3, HIGH, timeout);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);
  duration[3] = pulseIn(pingPin4, HIGH, timeout);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);
  duration[4] = pulseIn(pingPin5, HIGH, timeout);

  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);
  pinMode(pingPin6, INPUT);
  duration[5] = pulseIn(pingPin6, HIGH, timeout);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(trigPin, LOW);
  duration[6] = pulseIn(pingPin7, HIGH, timeout);

  //delay(10);
  for (int i = 0; i < nb_US ; i++) {
    if (duration[i] == 0) {
      duration[i] = 58 * MaxUS_cm;
    }
    vUS[i] = (duration[i] / 58);
  }
}

void on_STOP_button_pushed() {
  if (millis() - startDebounceTime > debounceDelay) {
    if (!STOP) {
      analogWrite(enablePin, 0);
      STOP = true;
      motorBackOnSTOP = true;
    }
    startDebounceTime = millis();
  }
}

void read_PC_data() {
  if (Serial.available()) {
    PCdata = Serial.readString();
    d1 = PCdata.charAt(0);

    if (d1 == 'B') {
      STOP = !STOP;
      motorBackOnSTOP = true;
      delayMicroseconds(1);
    }
  }

//Send data at sendDataInterval rate
if ((millis() - sendDataTimerStart) >= sendDataInterval) {
  Serial.print("a");//US1
  Serial.println(vUS[0]);
  Serial.print("b");//US2
  Serial.println(vUS[1]);
  Serial.print("c");
  Serial.println(vUS[2]);
  Serial.print("d");
  Serial.println(vUS[3]);
  Serial.print("e");
  Serial.println(vUS[4]);
  Serial.print("f");
  Serial.println(vUS[5]);
  Serial.print("g");
  Serial.println(vUS[6]);
  Serial.print("h");//Load cell force
  Serial.println(vLC, 3);
  Serial.print("i");//motorSpeed
  if (motorSpeed < 0) {
    Serial.print("n");//if motorSpeed is negatif
  }
  Serial.println(abs(motorSpeed));
  Serial.print("j");//Vibrator
  Serial.println(dir_obstacle);
  Serial.print("k");//Is STOP ?
  Serial.println(STOP);
  sendDataTimerStart = millis();
}
}
