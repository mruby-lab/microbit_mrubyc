/**
 * @file microbit.ino
 * @brief mrbwrite.exeと連携し、mruby/cのバイトコードをFlashに書き込み、実行するファームウェア
 * @version 2.1
 * @date 2025-07-04
 * @details
 * このファームウェアは、mrbwriteの通信プロトコルに完全準拠しており、WebAPI(Smalruby等)との
 * 連携を想定しています。各コマンドへの応答メッセージを仕様と完全に一致させています。
 */

#include <Arduino.h>
#include <Wire.h>

// --- mruby/c ヘッダ ---
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
#define VERSION_STRING "+OK mruby/c v3.3 RITE0300 MRBW1.2\r\n"

// =================================================================
// グローバル変数・外部関数
// =================================================================
extern "C" {
  // mruby/c VMが使用するメモリプール
  uint8_t memory_pool[MAX_MRB_SIZE];
  // Flashからバイトコードを読み込むための一時RAM領域
  uint8_t mrbbuf_ram[MAX_MRB_SIZE];

  // ボード固有のFlash書き込み/消去関数 (別途実装が必要)
  int flash_write_data(uint32_t addr, const uint8_t *data, int len);
  int flash_erase_page(uint32_t addr);

  // mruby/c VM実行関数 (本体はmrubyc.cで定義)
  int mrubyc(void);

  // I2C ラッパー関数 (c_robot.c などから使用)
  void my_w_beginTrans(int address) { Wire.beginTransmission(address); }
  void my_w_wire(int data) { Wire.write(data); }
  void my_w_endTrans() { Wire.endTransmission(); }

  // シリアル出力ラッパー関数 (mrubyc.c などから使用)
  void my_putc(int c) { if (Serial) Serial.write(c); }
  void my_print(char *buf) { if (Serial) Serial.print(buf); }
  void my_println(char *buf) { if (Serial) Serial.println(buf); }
}

// シリアルコマンド受信用バッファ
char command_buf[64];
int cmd_pos = 0;

// =================================================================
// コマンド処理関数群
// =================================================================

/**
 * @brief 'help' コマンドを処理し、対応コマンド一覧を返す
 */
void cmd_help() {
  Serial.write("+OK\r\n");
  Serial.write("Commands:\r\n");
  Serial.write("  version\r\n");
  Serial.write("  clear\r\n");
  Serial.write("  write <size>\r\n");
  Serial.write("  execute\r\n");
  Serial.write("  showprog\r\n");
  Serial.write("+DONE\r\n");
}

/**
 * @brief 'version' コマンドを処理し、バージョン情報を返す
 */
void cmd_version() {
  Serial.write(VERSION_STRING);
}

/**
 * @brief 'clear' コマンドを処理し、Flashを消去する
 */
void cmd_clear() {
  if (flash_erase_page(FLASH_ADDR) == 0) {
    Serial.write("+OK\r\n");
  } else {
    Serial.write("-ERR Flash erase failed\r\n");
  }
}

/**
 * @brief 'write' コマンドを処理し、バイトコードをFlashに書き込む
 * @param size 書き込むバイトコードのサイズ
 */
void cmd_write(int size) {
  if (size <= 0 || size > MAX_MRB_SIZE) {
    Serial.write("-ERR invalid size\r\n");
    return;
  }

  // プロトコル準拠の応答: "+OK Write bytecode"
  Serial.write("+OK Write bytecode\r\n");
  Serial.flush();

  // バイトコード受信前にシリアルバッファをクリア
  while (Serial.available()) {
    Serial.read();
  }

  uint8_t *buf = (uint8_t *)malloc(size);
  if (!buf) {
    Serial.write("-ERR malloc failed\r\n");
    return;
  }

  int received = Serial.readBytes((char *)buf, size);
  if (received != size) {
    Serial.write("-ERR receive timeout\r\n");
    free(buf);
    return;
  }

  if (memcmp(buf, "RITE", 4) != 0) {
    Serial.write("-ERR No RITE header\r\n");
    free(buf);
    return;
  }
  
  if (flash_erase_page(FLASH_ADDR) != 0) {
    Serial.write("-ERR Flash erase failed\r\n");
    free(buf);
    return;
  }

  if (flash_write_data(FLASH_ADDR, buf, size) != 0) {
    Serial.write("-ERR Flash write failed\r\n");
    free(buf);
    return;
  }

  // プロトコル準拠の応答: "+DONE"
  Serial.write("+DONE\r\n");
  free(buf);
}

/**
 * @brief 'execute' コマンドを処理し、Flash上のプログラムを実行する
 */
void cmd_execute() {
  const uint8_t *p = (const uint8_t *)FLASH_ADDR;
  if (memcmp(p, "RITE", 4) != 0) {
    Serial.write("-ERR No bytecode\r\n");
    return;
  }
  
  Serial.write("+OK Execute mruby/c.\r\n");
  
  memcpy(mrbbuf_ram, p, MAX_MRB_SIZE);
  mrubyc(); // mruby/c VMを起動
}

/**
 * @brief 受信したコマンド文字列を解釈し、対応する関数を呼び出す
 * @param cmd 受信したコマンド文字列
 */
void process_command(char *cmd) {
  int size;
  if (strcasecmp(cmd, "help") == 0) {
    cmd_help();
  } else if (strcasecmp(cmd, "version") == 0) {
    cmd_version();
  } else if (strcasecmp(cmd, "clear") == 0) {
    cmd_clear();
  } else if (strcasecmp(cmd, "execute") == 0) {
    cmd_execute();
  } else if (strcasecmp(cmd, "showprog") == 0) {
    Serial.write("+OK\r\n");
  } else if (sscanf(cmd, "write %d", &size) == 1) {
    cmd_write(size);
  } else if (strlen(cmd) > 0) {
    Serial.write("-ERR Illegal command '");
    Serial.write(cmd);
    Serial.write("'\r\n");
  }
}

// =================================================================
// Arduino メイン処理 (setup, loop)
// =================================================================

/**
 * @brief 初期化処理
 */
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(5000);
  Wire.begin();
  pinMode(LED_BUILTIN, OUTPUT);

  // mrbwriteとの同期: 起動時にCRLFを受信するまで待機
  unsigned long start_time = millis();
  while (millis() - start_time < 3000) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\r' || c == '\n') {
        break;
      }
    }
  }
  Serial.write(VERSION_STRING);
}

/**
 * @brief メインループ。シリアルからのコマンド入力を待機する
 */
void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (cmd_pos > 0) {
        command_buf[cmd_pos] = '\0';
        process_command(command_buf);
      }
      cmd_pos = 0;
    } else if (cmd_pos < sizeof(command_buf) - 1) {
      command_buf[cmd_pos++] = c;
    }
  }
}
