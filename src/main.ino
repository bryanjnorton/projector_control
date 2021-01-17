#include <EEPROM.h>
int eeAddress = 0;

#include "CmdMessenger.h"
unsigned long time_on = 0; 
unsigned long time_off = 0;
unsigned long time_delta = 0;

unsigned int max_pulse = 2000;

uint8_t duty = 5;

uint8_t led_state = 0;
uint8_t error_too_long = 0;

/* Define available CmdMessenger commands */
enum {
  led_on,
  led_on_return,

  led_off,
  long_time,

  grab_state,
  long_state,

  grab_config,
  put_config,

  set_config,
  put_config2,

  error,
};

/* Initialize CmdMessenger -- this should match PyCmdMessenger instance */
const int BAUD_RATE = 9600;
CmdMessenger c = CmdMessenger(Serial,',',';','/');

void on_led_on(void){
  if(led_state == 0) {
    if( (5*time_delta) < (millis() - time_off) ) {  
      time_on = millis();
      digitalWrite(2, HIGH);       // sets the digital pin 13 on
      led_state = 1;
      c.sendCmd(led_on_return,"1");
    } else {
      c.sendCmd(led_on_return,"slowdown");
    }
  } else {
    digitalWrite(2, LOW);       // sets the digital pin 13 on
    led_state = 0;
    time_off = millis();    
    time_delta = time_off - time_on;
    c.sendCmd(led_on_return,"Wrong led state");
  }
}

/* callback */
void on_led_off(void){
  if( led_state == 0 ){
    c.sendCmdStart(long_time);
    c.sendCmdArg("led already off");
    c.sendCmdBinArg((unsigned long)0);
    c.sendCmdEnd(); 
  } else { 
    time_off = millis();    
    time_delta = time_off - time_on;
    led_state = 0;
    digitalWrite(2, LOW);       // sets the digital pin 2 off
    //c.sendBinCmd(long_time,time_delta);// millis());

    c.sendCmdStart(long_time);
    c.sendCmdArg("Pulse delta (ms)");
    c.sendCmdBinArg(time_delta);

    c.sendCmdEnd();
  }
}

/* callback */
void on_unknown_command(void){
 c.sendCmd(error,"Command without callback.");
}

/* Attach callbacks for CmdMessenger commands */
void attach_callbacks(void) {   
    c.attach(led_on,on_led_on);
    c.attach(led_off,on_led_off);
    c.attach(grab_state,on_grab_state);
    c.attach(grab_config,on_grab_config);
    c.attach(set_config, on_set_config);
    c.attach(on_unknown_command);
}

void setup() {
  Serial.begin(BAUD_RATE);
  attach_callbacks();  
  pinMode(13, OUTPUT); 
  pinMode(2, OUTPUT); 
  pinMode(3, OUTPUT); 
  pinMode(4, OUTPUT); 

}

void loop() {
  c.feedinSerialData();
  digitalWrite(3, led_state);
  digitalWrite(4, ((5*time_delta) > (millis() - time_off)) );
  if( ( (millis()-time_on) > max_pulse ) && led_state )
  {
    time_off = millis();
    time_delta = max_pulse;
    digitalWrite(2, LOW);
    led_state = 0;
    error_too_long = 1; 
  }
}

void on_grab_state(void){  
  c.sendCmdStart(long_state);

  c.sendCmdArg("Pulse delta (ms)");
  c.sendCmdBinArg(time_delta);

  c.sendCmdArg("onset (ms)");
  c.sendCmdBinArg(time_on);

  c.sendCmdArg("offset (ms)");
  c.sendCmdBinArg(time_off);

  c.sendCmdArg("led_state (0/1)");
  c.sendCmdBinArg((int)led_state);

  c.sendCmdEnd();
}

void on_grab_config(void){ 

  EEPROM.get( eeAddress, duty );
  EEPROM.get( eeAddress+1, max_pulse );

  c.sendCmdStart(put_config);

  c.sendCmdArg("Max pulse (ms)");
  c.sendCmdBinArg(max_pulse);

  c.sendCmdArg("(off time)/(on time) =");
  c.sendCmdBinArg(duty);

  c.sendCmdEnd();
}

void on_set_config(void){ 

  uint8_t duty = c.readBinArg<byte>();
  unsigned int max_pulse = c.readBinArg<unsigned int>();

  EEPROM.put( eeAddress, duty );
  EEPROM.put( eeAddress+1, max_pulse );

  c.sendCmdStart(put_config2);

  c.sendCmdArg("Max pulse (ms)");
  c.sendCmdBinArg(max_pulse);

  c.sendCmdArg("(off time)/(on time) =");
  c.sendCmdBinArg(duty);

  c.sendCmdEnd();
}