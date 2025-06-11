#include <Wire.h>
#ifdef __cplusplus
extern "C" {
#endif
  #include "mrubyc.h"
#ifdef __cplusplus
}
#endif

#define TIMEOUT_COUNT 50
#define TIMEOUT_DELAY_MS 30
#define MAX_MRB_SIZE 2048
uint8_t memory_pool[MAX_MRB_SIZE];
#define FLASH_ADDR 0x0003F000
int irep_size = 0;

extern "C" {
  extern int mrubyc(void);
  extern void my_putc(int c);
  extern void my_print(char *buf);
  extern void my_println(char *buf);
  extern void my_s_begin(int baudRate);
  extern void my_w_wire(int data);
  extern void my_w_begin(int b);
  extern void my_w_beginTrans(int address);
  extern void my_w_endTrans();
  int uart_can_read_line(void);
  int check_timeout(void);
  int flash_write_data(uint32_t addr, const uint8_t *data, int len);
  int flash_erase_page(uint32_t addr);
  extern void mrb_load_irep_from_flash(uint32_t addr);
  extern uint8_t mrbbuf_ram[]; 

};

void my_putc(int c){
  Serial.write(c);
}

void my_print(char *buf){
  Serial.print(buf);
}

void my_println(char *buf){
  Serial.println(buf);
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

int uart_can_read_line(void) {
  return Serial.available() > 0;
}

int check_timeout(void) {
  for (int i = 0; i < TIMEOUT_COUNT; i++) {
    // LEDをチカチカさせて受信待ちを視覚化（なくてもOK）
    digitalWrite(LED_BUILTIN, HIGH);
    delay(TIMEOUT_DELAY_MS);
    digitalWrite(LED_BUILTIN, LOW);
    delay(TIMEOUT_DELAY_MS);

    if (uart_can_read_line()) {
      return 1;  // UART入力あり
    }
  }
  return 0;  // タイムアウト（UART入力なし）
}

int receive_bytecode(void) {
  Serial.println("受信待機中...");
  
  while (true) {
    if (Serial.available()) {
      uint8_t b = Serial.read();
      if (b == 'R') break;  // 'R'が来たら受信開始
    }
      }

  memory_pool[0] = 'R';
  int index = 1;

  while (index < MAX_MRB_SIZE) {
    if (Serial.available()) {
      memory_pool[index++] = Serial.read();
    }
  }

  Serial.println("受信完了");
  return index;
}



void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("+OK mruby/c v3.3 RITE0300 MRBW1.2");

  char command[32];
  int cmd_pos = 0;
  unsigned long start = millis();

  // 一定時間だけコマンド受付待機
  while (millis() - start < 5000) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        command[cmd_pos] = '\0';
        cmd_pos = 0;

        if (strncmp(command, "write", 5) == 0) {
          int size = atoi(&command[6]);
          if (size <= 0 || size > MAX_MRB_SIZE) {
            Serial.println("-ERR invalid size");
            return;
          }

          Serial.println("+OK Write bytecode.");

          // バイトコード受信
          int received = 0;
          while (received < size) {
            if (Serial.available()) {
              memory_pool[received++] = Serial.read();
            }
          }

          if (strncmp((char *)memory_pool, "RITE", 4) != 0) {
            Serial.println("-ERR No RITE header");
            return;
          }

          if (flash_erase_page(FLASH_ADDR) != 0) {
            Serial.println("-ERR Flash erase failed");
            return;
          }

          if (flash_write_data(FLASH_ADDR, memory_pool, size) != 0) {
            Serial.println("-ERR Flash write failed");
            return;
          }

          irep_size = size;
          Serial.println("+DONE");
        }
        else if (strncmp(command, "execute", 7) == 0) {
          const uint32_t *p = (const uint32_t*)FLASH_ADDR;
          if (*p == 0x45744952) {
            memcpy(mrbbuf_ram, (const void*)FLASH_ADDR, MAX_MRB_SIZE);
            Serial.println("+OK Execute");
          } else {
            Serial.println("-ERR No bytecode");
            return;
          }
        }
        else {
          Serial.println("-ERR unknown command");
        }
      }
      else if (cmd_pos < sizeof(command) - 1) {
        command[cmd_pos++] = c;
      }
    }
  }

  // 実行準備ができていたらmruby VM実行
  if (strncmp((char *)mrbbuf_ram, "RITE", 4) == 0) {
    Serial.println("Flash内のバイトコードを実行します...");
    mrubyc();
  } else {
    Serial.println("バイトコードなし。LED点滅します。");
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_BUILTIN, HIGH); delay(150);
      digitalWrite(LED_BUILTIN, LOW); delay(150);
    }
  }
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
