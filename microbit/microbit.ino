
extern "C" {
  extern int mrubyc(void);
  extern void my_putc(int c);
  extern void my_print(char *buf);
};

void my_putc(int c){
  Serial.write(c);
}

void my_print(char *buf){
  Serial.print(buf);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(PIN_BUTTON_A, INPUT);
  pinMode(PIN_BUTTON_B, INPUT);

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
