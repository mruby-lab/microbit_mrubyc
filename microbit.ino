#include <Wire.h>

extern "C" {
  extern int mrubyc(void);
  extern void my_putc(int c);
  extern void my_print(char *buf);
  extern void my_s_begin(int baudRate);
  extern void my_w_wire(int data);
  extern void my_w_begin(int b);
  extern void my_w_beginTrans(int address);
  extern void my_w_endTrans();

};

void my_putc(int c){
  Serial.write(c);
}

void my_print(char *buf){
  Serial.print(buf);
}
 
void my_s_begin(int baudRate){
  Serial.begin(baudRate);
}

void my_w_wire(int data){
  Wire.write(data);
}

void my_w_begin(int b){
  Wire.begin(b);
}

void my_w_beginTrans(int address){
  Wire.beginTransmission(address);
}
void my_w_endTrans(){
  Wire.endTransmission();
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();

  mrubyc();
}
void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(PIN_BUTTON_A) == LOW && digitalRead(PIN_BUTTON_B) == LOW) {
    Serial.println("A&B Button ON");
    delay(300);
  } else if (digitalRead(PIN_BUTTON_A) == LOW) {
    Serial.println("A Button ON");
    delay(300);
  } else if (digitalRead(PIN_BUTTON_B) == LOW) {
    Serial.println("B Button ON");
    delay(300);
  }
}
