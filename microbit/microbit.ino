/**
 * @file microbit.ino
 * @brief mrbwrite.exeおよびWebAPI(kaniwriter等)と連携する、mruby/c書き込みファームウェア
 * @version 10.2 (Final Fix)
 * @date 2025-07-10
 * @details
 * ログの乱れを修正するため、コマンド受信ロジックを堅牢なラインバッファ方式に変更。
 * これにより、CRLF, CR, LFを単一の区切りとして正確に処理し、通信を完全に安定させた最終版。
 * helpコマンドの処理を追加。
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
// kaniwriterで使用するmrbcバージョン(3.1.0)に合わせる
#define VERSION_STRING "+OK mruby/c v3.1 RITE0300 MRBW1.2\r\n"

// =================================================================
// グローバル変数・外部関数
// =================================================================
extern "C" {
  uint8_t memory_pool[MAX_MRB_SIZE];
  uint8_t mrbbuf_ram[MAX_MRB_SIZE];

  int flash_write_data(uint32_t addr, const uint8_t *data, int len);
  int flash_erase_page(uint32_t addr);
  int mrubyc(void);

  void my_w_beginTrans(int address) { Wire.beginTransmission(address); }
  void my_w_wire(int data) { Wire.write(data); }
  void my_w_endTrans() { Wire.endTransmission(); }

  void my_putc(int c) { if (Serial) Serial.write(c); }
  void my_print(char *buf) { if (Serial) Serial.print(buf); }
  void my_println(char *buf) { if (Serial) Serial.println(buf); }
}

// =================================================================
// コマンド処理関数群
// =================================================================

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

void cmd_version() {
  Serial.write(VERSION_STRING);
}

void cmd_clear() {
  if (flash_erase_page(FLASH_ADDR) == 0) {
    Serial.write("+OK\r\n");
  } else {
    Serial.write("-ERR Flash erase failed\r\n");
  }
}

void cmd_write(int size) {
  if (size <= 0 || size > MAX_MRB_SIZE) {
    Serial.write("-ERR invalid size\r\n");
    return;
  }
  Serial.write("+OK Write bytecode\r\n");
  Serial.flush();
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
  Serial.write("+DONE\r\n");
  free(buf);
}

void cmd_execute() {
  const uint8_t *p = (const uint8_t *)FLASH_ADDR;
  if (memcmp(p, "RITE", 4) != 0) {
    Serial.write("-ERR No bytecode\r\n");
    return;
  }
  Serial.write("+OK Execute mruby/c.\r\n");
  memcpy(mrbbuf_ram, p, MAX_MRB_SIZE);
  mrubyc();
}

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

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(5000);
  Wire.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  // 起動時は何も送信せず、ホストからの最初のCRLFをloop()で待ち受ける
}

void loop() {
  static char command_buf[64];
  static int cmd_pos = 0;
  static bool line_ended = false; // CRLFによる二重処理を防ぐためのフラグ

  if (Serial.available()) {
    char c = Serial.read();

    // CR('\r') または LF('\n') を行の終わりとして処理する
    if (c == '\r' || c == '\n') {
      // ただし、直前に改行文字を処理していない場合のみ実行
      if (!line_ended) {
        command_buf[cmd_pos] = '\0'; // 文字列を終端

        if (cmd_pos > 0) {
          // バッファに何かあれば、それはコマンド
          process_command(command_buf);
        } else {
          // バッファが空なら、それは同期のためのCRLF
          cmd_version();
        }
        
        // 次のコマンドのためにバッファをリセット
        cmd_pos = 0;
      }
      // CR/LFの連続を1つの改行として扱うためにフラグを立てる
      line_ended = true;
    } else {
      // 通常の文字が来たら、改行は終わったと判断
      line_ended = false;
      if (cmd_pos < sizeof(command_buf) - 1) {
        command_buf[cmd_pos++] = c;
      }
    }
  }
}
