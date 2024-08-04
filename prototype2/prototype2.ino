/*
Prototype 2:

  CB status trips based on signal and manual button inputs.
  Out shift register code modified from https://www.instructables.com/3-Arduino-pins-to-24-output-pins/
*/
#include <ezAnalogKeypad.h>

// Defines to make code more readable
bool spring_charge_timer_running = 0; // Timer for spring charged status re-charging
unsigned long spring_charge_start_time;

#define trip_input_pin 2  // Digital pin used for CB trip
#define close_input_pin 12

// 74HC595 shift register pins and variables for LEDs
#define LED_data_pin 9  //pin 14 DS
#define LED_latch_pin 10 //pin 12 ST_CP
#define LED_clock_pin 11  //pin 11 SH_CP
#define numOfLEDRegisters 3
#define numOfLEDRegisterPins numOfLEDRegisters * 8
boolean LEDregisters[numOfLEDRegisterPins];

struct Status {
  int green_LED;  // shift register pin that the LEDs are connected to  
  int red_LED;
  int green_button;   // position of button 
  int red_button;
  boolean state;
};

#define NUM_STATUSES 10

Status statuses_array[NUM_STATUSES];

// analog pins are A0(14) to A5(19)
ezAnalogKeypad buttonSet1(A0);   
ezAnalogKeypad buttonSet2(A1);  

// array index of each status 
#define CB_status 0  
#define gas_pressure_status 1 
#define earth_switch_status 2  
#define circuit_supervision_status 3  
#define service_position_status 4  
#define spring_charge_status 5  
#define generic_status1 6
#define generic_status2 7
#define generic_status3 8
#define generic_status4 9

boolean prev_CB_status = HIGH;  

// write LED shift registers according to each status
void writeLEDOutputs() {
  for (int i = 0; i < NUM_STATUSES; i++) {
    LEDregisters[statuses_array[i].green_LED] = statuses_array[i].state;
    LEDregisters[statuses_array[i].red_LED] = !statuses_array[i].state;
  }

  digitalWrite(LED_latch_pin, LOW);
  // write outputs to shift register data pin
  for(int i = numOfLEDRegisterPins - 1; i >=  0; i--){
    digitalWrite(LED_clock_pin, LOW);

    int val = LEDregisters[i];

    digitalWrite(LED_data_pin, val);
    digitalWrite(LED_clock_pin, HIGH);

  }
  digitalWrite(LED_latch_pin, HIGH);

}

// set cb status to high and set spring charge status to discharged
void closeCB() {
  prev_CB_status = statuses_array[CB_status].state;
  statuses_array[CB_status].state = HIGH;  
  if (prev_CB_status == LOW) {
    statuses_array[spring_charge_status].state = LOW;
    spring_charge_start_time = millis();    // start timer
    spring_charge_timer_running = 1;
  }
}

// set cb status to low and saves previous cb status
void openCB() {
    prev_CB_status = statuses_array[CB_status].state;
    statuses_array[CB_status].state = LOW; 
}

void processButton(unsigned char key) {
  for (int i = 0; i < NUM_STATUSES*2; i++) {
    if (key == statuses_array[i].green_LED) {
      statuses_array[i].state = HIGH;
    }
    if (key == statuses_array[i].red_LED) {
      statuses_array[i].state = LOW;
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(trip_input_pin, INPUT);
  pinMode(close_input_pin, INPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);

  // LED shift registers
  pinMode(LED_latch_pin, OUTPUT);
  pinMode(LED_clock_pin, OUTPUT);
  pinMode(LED_data_pin, OUTPUT);

  // Left hand side buttons
  buttonSet1.setNoPressValue(1023);  // analog value when no button is pressed
  buttonSet1.registerKey(1, 0); // button for CB manual close
  buttonSet1.registerKey(2, 100); // button for gas pressure normal
  buttonSet1.registerKey(3, 200); // button for earth switch not earthed
  buttonSet1.registerKey(4, 300); // button for generic status1 HIGH
  buttonSet1.registerKey(5, 400); // button for generic status2 HIGH
  buttonSet1.registerKey(6, 500); // button for generic status2 LOW
  buttonSet1.registerKey(7, 600); // button for generic status1 LOW
  buttonSet1.registerKey(8, 700); // button for earth switch HIGH
  buttonSet1.registerKey(9, 800); // button for gas pressure low
  buttonSet1.registerKey(10, 900); // button for CB manual LOW

  // Right hand side buttons 
  buttonSet2.setNoPressValue(1023);  // analog value when no button is pressed
  buttonSet2.registerKey(11, 0);  // service position status racked in
  buttonSet2.registerKey(12, 100); // spring charge status charged
  buttonSet2.registerKey(13, 200); // trip circuit supervision status normal
  buttonSet2.registerKey(14, 300); // generic status3 HIGH
  buttonSet2.registerKey(15, 400); // generic status4 HIGH
  buttonSet2.registerKey(16, 500); // generic status4 LOW
  buttonSet2.registerKey(17, 600); // generic status3 LOW
  buttonSet2.registerKey(18, 700); // trip circuit supervision status fault
  buttonSet2.registerKey(19, 800); // spring charge status discharged
  buttonSet2.registerKey(20, 900);  // service position status racked out

  statuses_array[CB_status].green_LED = 2;
  statuses_array[CB_status].red_LED = 1;
  statuses_array[CB_status].green_button = 1;
  statuses_array[CB_status].red_button = 10;
  statuses_array[CB_status].state = HIGH;

  // add more statuses
}

void loop() {
  // Left set of buttons
  unsigned char key1 = buttonSet1.getKey();
  Serial.println(key1);
  processButton(key1);
  delay(500);

  // Right set of buttons
  unsigned char key2 = buttonSet2.getKey();
  processButton(key2);

  int trip_signal = digitalRead(trip_input_pin);
  if (trip_signal == HIGH) {  
    openCB();
  }

  int close_signal = digitalRead(close_input_pin);
  if (close_signal == HIGH) {
    closeCB();
  }

  if ((statuses_array[spring_charge_status].state == LOW) && (spring_charge_timer_running)) {
      if ((millis() - spring_charge_start_time) >= 4000) {
        statuses_array[spring_charge_status].state = HIGH;  // if 4 seconds have passed since CB HIGH, spring finishes charging
        spring_charge_timer_running = 0;
      }
  }
  writeLEDOutputs();
}
