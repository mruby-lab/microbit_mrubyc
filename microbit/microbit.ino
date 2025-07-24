/**
 * @file microbit.ino
 * @brief mruby/cを搭載したTinybitロボット用 最終完成版ファームウェア
 * @version 24.0 (Final Reviewed Version)
 * @date 2025-07-24
 * @details
 * PCからのコマンド受付と、内蔵/書き込みプログラムの実行に対応するデュアルモードシステム。
 * C++でハードウェア制御のラッパーを実装し、mruby/c(C言語)から呼び出せるように連携する。
 */

//  Arduinoの基本機能を明示的にインクルード
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Microbit.h>

// --- mruby/c ヘッダ ---
// C++の世界からC言語のファイルを読み込むための作法
#ifdef __cplusplus
extern "C" {
#endif
#include "mrubyc.h"
#ifdef __cplusplus
}
#endif

// =================================================================
// 定数定義
// =================================================================
#define SERIAL_BAUD_RATE 115200
#define MAX_MRB_SIZE 2048
#define FLASH_ADDR 0x0003F000
#define WRITE_WAIT_TIMEOUT 3000
#define VERSION_STRING "+OK mruby/c v3.1 RITE0300 MRBW1.2\r\n"

// =================================================================
// デフォルトで実行するサンプルプログラム (5x5 LED全面点滅)
// =================================================================
const uint8_t default_mrb[] = {
  0x52,0x49,0x54,0x45,0x30,0x33,0x30,0x30,0x00,0x00,0x01,0x41,0x4d,0x41,0x54,0x5a,
  0x30,0x30,0x30,0x30,0x49,0x52,0x45,0x50,0x00,0x00,0x01,0x04,0x30,0x33,0x30,0x30,
  0x00,0x00,0x00,0xf8,0x00,0x03,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2c,
  0x51,0x01,0x00,0x51,0x02,0x01,0x01,0x04,0x01,0x2d,0x03,0x00,0x01,0x0e,0x04,0x01,
  0xf4,0x2d,0x03,0x01,0x01,0x01,0x04,0x02,0x2d,0x03,0x00,0x01,0x0e,0x04,0x01,0xf4,
  0x2d,0x03,0x01,0x01,0x25,0xff,0xdf,0x11,0x03,0x38,0x03,0x69,0x00,0x02,0x00,0x00,
  0x64,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,
  0x2c,0x32,0x35,0x35,0x0a,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,
  0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x0a,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,
  0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x0a,0x32,0x35,0x35,
  0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,
  0x0a,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,0x2c,0x32,0x35,0x35,
  0x2c,0x32,0x35,0x35,0x0a,0x00,0x00,0x00,0x32,0x30,0x2c,0x30,0x2c,0x30,0x2c,0x30,
  0x2c,0x30,0x0a,0x30,0x2c,0x30,0x2c,0x30,0x2c,0x30,0x2c,0x30,0x0a,0x30,0x2c,0x30,
  0x2c,0x30,0x2c,0x30,0x2c,0x30,0x0a,0x30,0x2c,0x30,0x2c,0x30,0x2c,0x30,0x2c,0x30,
  0x0a,0x30,0x2c,0x30,0x2c,0x30,0x2c,0x30,0x2c,0x30,0x0a,0x00,0x00,0x02,0x00,0x0c,
  0x64,0x69,0x73,0x70,0x6c,0x61,0x79,0x5f,0x73,0x68,0x6f,0x77,0x00,0x00,0x08,0x73,
  0x6c,0x65,0x65,0x70,0x5f,0x6d,0x73,0x00,0x4c,0x56,0x41,0x52,0x00,0x00,0x00,0x21,
  0x00,0x00,0x00,0x02,0x00,0x06,0x61,0x6c,0x6c,0x5f,0x6f,0x6e,0x00,0x07,0x61,0x6c,
  0x6c,0x5f,0x6f,0x66,0x66,0x00,0x00,0x00,0x01,0x45,0x4e,0x44,0x00,0x00,0x00,0x00,0x08,
};


// =================================================================
// グローバル変数・外部関数
// ★修正点2: C++とC言語の連携部分であることをコメントで明確化
// =================================================================
extern "C" {
  // mrubyc.c で定義されているC言語の変数や関数
  uint8_t memory_pool[MAX_MRB_SIZE];
  uint8_t mrbbuf_ram[MAX_MRB_SIZE];
  int flash_write_data(uint32_t addr, const uint8_t *data, int len);
  int flash_erase_page(uint32_t addr);
  int mrubyc(void);

  // mrubyc.c から呼び出される、この.inoファイルで実装するC++のラッパー関数
  void my_w_beginTrans(int address) { Wire.beginTransmission(address); }
  void my_w_wire(int data) { Wire.write(data); }
  void my_w_endTrans() { Wire.endTransmission(); }
  void my_putc(int c) { if (Serial) Serial.write(c); }
  void my_print(char *buf) { if (Serial) Serial.print(buf); }
  void my_println(char *buf) { if (Serial) Serial.println(buf); }
}

// C++のオブジェクト。C言語からは直接アクセスできない。
Adafruit_Microbit_Matrix microbit;

// C言語の世界(mrubyc.c)から、C++のmicrobitオブジェクトを操作するための「橋渡し役」関数
extern "C" void c_if_display_show(const char* image_data_const) {
  char image_data[strlen(image_data_const) + 1];
  strcpy(image_data, image_data_const);
  microbit.clear();
  char *saveptr_row;
  char *row = strtok_r(image_data, "\n", &saveptr_row);
  for (int y = 0; y < 5 && row != NULL; y++) {
    char *saveptr_pixel;
    char *pixel = strtok_r(row, ",", &saveptr_pixel);
    for (int x = 0; x < 5 && pixel != NULL; x++) {
      if (atoi(pixel) > 0) {
        microbit.drawPixel(x, y, LED_ON);
      }
      pixel = strtok_r(NULL, ",", &saveptr_pixel);
    }
    row = strtok_r(NULL, "\n", &saveptr_row);
  }
}


// =================================================================
// プログラム実行/コマンド処理関数
// =================================================================
void process_command(String cmd);
void cmd_version() { Serial.write(VERSION_STRING); }
void cmd_clear() { if (flash_erase_page(FLASH_ADDR) == 0) Serial.write("+OK\r\n"); else Serial.write("-ERR Flash erase failed\r\n"); }
void cmd_write(int size) {
  if (size <= 0 || size > MAX_MRB_SIZE) { Serial.write("-ERR invalid size\r\n"); return; }
  Serial.write("+OK Write bytecode\r\n");
  Serial.flush();
  while (Serial.available()) { Serial.read(); }
  uint8_t *buf = (uint8_t *)malloc(size);
  if (!buf) { Serial.write("-ERR malloc failed\r\n"); return; }
  int received = Serial.readBytes((char *)buf, size);
  if (received != size) { Serial.write("-ERR receive timeout\r\n"); free(buf); return; }
  if (memcmp(buf, "RITE", 4) != 0) { Serial.write("-ERR No RITE header\r\n"); free(buf); return; }
  if (flash_erase_page(FLASH_ADDR) != 0) { Serial.write("-ERR Flash erase failed\r\n"); free(buf); return; }
  if (flash_write_data(FLASH_ADDR, buf, size) != 0) { Serial.write("-ERR Flash write failed\r\n"); free(buf); return; }
  Serial.write("+DONE\r\n");
  free(buf);
}
void cmd_execute() {
  const uint8_t *p = (const uint8_t *)FLASH_ADDR;
  if (memcmp(p, "RITE", 4) != 0) { Serial.write("-ERR No bytecode\r\n"); return; }
  Serial.write("+OK Execute mruby/c.\r\n");
  memcpy(mrbbuf_ram, p, MAX_MRB_SIZE);
  mrubyc();
}
void execute_program(const uint8_t *bytecode) {
  memcpy(mrbbuf_ram, bytecode, MAX_MRB_SIZE);
  mrubyc();
}

void process_command(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) {
    cmd_version();
    return;
  }
  int size;
  if (cmd.equalsIgnoreCase("help")) {
    Serial.write("+OK\r\n");
    Serial.write("Commands:\r\n");
    Serial.write("  help\r\n");
    Serial.write("  version\r\n");
    Serial.write("  clear\r\n");
    Serial.write("  write\r\n");
    Serial.write("  execute\r\n");
    Serial.write("  showprog\r\n");
    Serial.write("+DONE\r\n");
  } else if (cmd.equalsIgnoreCase("version")) {
    cmd_version();
  } else if (cmd.equalsIgnoreCase("clear")) {
    cmd_clear();
  } else if (cmd.equalsIgnoreCase("execute")) {
    cmd_execute();
  } else if (cmd.equalsIgnoreCase("showprog")) {
    Serial.write("+OK\r\n");
  } else if (sscanf(cmd.c_str(), "write %d", &size) == 1) {
    cmd_write(size);
  } else {
    Serial.write("-ERR Illegal command '");
    Serial.print(cmd);
    Serial.write("'\r\n");
  }
}

// =================================================================
// Arduino メイン処理 (setup, loop)
// =================================================================
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(5000);
  Wire.begin();
  microbit.begin();

  unsigned long start_time = millis();
  // ★修正点3: デュアルモードの動作をコメントで解説
  // 起動後3秒間、PCからのシリアルデータ受信を待つ
  while (millis() - start_time < WRITE_WAIT_TIMEOUT) {
    if (Serial.available()) {
      // データを受信したら、コマンドモードに移行し、無限ループに入る。
      // これにより、リセットされるまでコマンドを受け付け続ける。
      while(true) {
        if (Serial.available() > 0) {
          String command = Serial.readStringUntil('\n');
          process_command(command);
        }
      }
    }
  }

  // 3秒経ってもデータが来なかった場合、スタンドアローンモードとして動作する
  const uint8_t *p = (const uint8_t *)FLASH_ADDR;
  if (memcmp(p, "RITE", 4) == 0) {
    // Flashに書き込まれたプログラムがあれば実行
    execute_program(p);
  } else {
    // なければデフォルトのプログラムを実行
    execute_program(default_mrb);
  }
}

void loop() {
  // 通常、mrubyc()が実行され続けるため、このループには到達しない。
  // もしmruby/cプログラムが終了した場合のフェールセーフとして機能する。
}
