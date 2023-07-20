void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON_A, INPUT);
  pinMode(PIN_BUTTON_B, INPUT);
}

void loop() {
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
