#include "mrubyc.h"
#include <string.h> // strcpy, strcat, sprintfのため
#include <ctype.h>  // toupperのため

//================================================================
// 外部 (microbit.ino) で定義された表示関数のプロトタイプ宣言
//================================================================
extern void c_if_display_show(const char* image_data);

//================================================================
// 内部変数 (画面バッファ)
//================================================================
static int screen_buffer[5][5] = {0};

//================================================================
// フォント定義 (アルファベット A-Z)
//================================================================
// 5x5ドットのフォントデータ
static const char FONT_ALPHABET[26][5][6] = {
    { "01110", "10001", "10001", "11111", "10001" }, // A
    { "11110", "10001", "11110", "10001", "11110" }, // B
    { "01111", "10000", "10000", "10000", "01111" }, // C
    { "11110", "10001", "10001", "10001", "11110" }, // D
    { "11111", "10000", "11110", "10000", "11111" }, // E
    { "11111", "10000", "11110", "10000", "10000" }, // F
    { "01111", "10000", "10011", "10001", "01110" }, // G
    { "10001", "10001", "11111", "10001", "10001" }, // H
    { "01110", "00100", "00100", "00100", "01110" }, // I
    { "00001", "00001", "00001", "10001", "01110" }, // J
    { "10001", "10010", "11100", "10010", "10001" }, // K
    { "10000", "10000", "10000", "10000", "11111" }, // L
    { "10001", "11011", "10101", "10001", "10001" }, // M
    { "10001", "11001", "10101", "10011", "10001" }, // N
    { "01110", "10001", "10001", "10001", "01110" }, // O
    { "11110", "10001", "11110", "10000", "10000" }, // P
    { "01110", "10001", "10101", "10010", "01101" }, // Q
    { "11110", "10001", "11110", "10100", "10001" }, // R
    { "01111", "10000", "01110", "00001", "11110" }, // S
    { "11111", "00100", "00100", "00100", "00100" }, // T
    { "10001", "10001", "10001", "10001", "01110" }, // U
    { "10001", "10001", "10001", "01010", "00100" }, // V
    { "10001", "10001", "10101", "11011", "10001" }, // W
    { "10001", "01010", "00100", "01010", "10001" }, // X
    { "10001", "01010", "00100", "00100", "00100" }, // Y
    { "11111", "00010", "00100", "01000", "11111" }  // Z
};

//================================================================
// 内部ヘルパー関数
//================================================================

// 画面バッファの内容を元に、物理的なLEDを更新する
static void update_display() {
  char display_str[100] = "";
  char temp_buf[8];

  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      int brightness = (screen_buffer[y][x] > 0) ? 255 : 0;
      sprintf(temp_buf, "%d", brightness);
      strcat(display_str, temp_buf);

      if (x < 4) strcat(display_str, ",");
    }
    if (y < 4) strcat(display_str, "\n");
  }
  c_if_display_show(display_str);
}

// 画面バッファを特定パターンで埋めるヘルパー関数
static void fill_buffer(const char pattern[5][6]) {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            screen_buffer[y][x] = (pattern[y][x] == '1') ? 1 : 0;
        }
    }
}

//================================================================
// LEDMatrixクラスのメソッド
//================================================================

// led.clear()
static void c_ledmatrix_clear(mrbc_vm *vm, mrbc_value v[], int argc) {
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      screen_buffer[y][x] = 0;
    }
  }
  update_display();
}

// led.write(x, y, value)
static void c_ledmatrix_write(mrbc_vm *vm, mrbc_value v[], int argc) {
  if (argc == 3 && v[1].tt == MRBC_TT_FIXNUM && v[2].tt == MRBC_TT_FIXNUM && v[3].tt == MRBC_TT_FIXNUM) {
    int x = v[1].i;
    int y = v[2].i;
    int value = v[3].i;

    if (x >= 0 && x < 5 && y >= 0 && y < 5) {
      screen_buffer[y][x] = value;
      update_display();
    }
  }
}

// led.display(char or string)
static void c_ledmatrix_display(mrbc_vm *vm, mrbc_value v[], int argc) {
    if (argc == 1 && v[1].tt == MRBC_TT_STRING) {
        const char* s = (const char*)v[1].string->data;
        char c = s[0];
        
        // --- 記号パターンの定義 ---
        const char CIRCLE_PATTERN[5][6]   = { "01110", "10001", "10001", "10001", "01110" };
        const char CROSS_PATTERN[5][6]    = { "10001", "01010", "00100", "01010", "10001" };
        const char TRIANGLE_PATTERN[5][6] = { "00100", "01010", "10001", "11111", "00000" };
        const char CHECK_PATTERN[5][6]    = { "00000", "00001", "00010", "10100", "01000" };

        const char UP_PATTERN[5][6]       = { "00100", "01110", "10101", "00100", "00100" };
        const char DOWN_PATTERN[5][6]     = { "00100", "00100", "10101", "01110", "00100" };
        const char LEFT_PATTERN[5][6]     = { "00100", "01000", "11111", "01000", "00100" };
        const char RIGHT_PATTERN[5][6]     = { "00100", "00010", "11111", "00010", "00100" };

        const char SMILE_PATTERN[5][6]    = { "00000", "01010", "00000", "10001", "01110" };
        const char SAD_PATTERN[5][6]      = { "00000", "01010", "00000", "01110", "10001" };
        const char HEART_PATTERN[5][6]    = { "01010", "11111", "11111", "01110", "00100" };

        // 画面クリア
        c_ledmatrix_clear(vm, v, 0);

        // 1. キーワードマッチ (記号系)
        if (strcmp(s, "circle") == 0)      fill_buffer(CIRCLE_PATTERN);
        else if (strcmp(s, "cross") == 0)  fill_buffer(CROSS_PATTERN);
        else if (strcmp(s, "triangle") == 0) fill_buffer(TRIANGLE_PATTERN);
        else if (strcmp(s, "check") == 0)  fill_buffer(CHECK_PATTERN);
        
        else if (strcmp(s, "up") == 0)     fill_buffer(UP_PATTERN);
        else if (strcmp(s, "down") == 0)   fill_buffer(DOWN_PATTERN);
        else if (strcmp(s, "left") == 0)   fill_buffer(LEFT_PATTERN);
        else if (strcmp(s, "right") == 0)  fill_buffer(RIGHT_PATTERN);
        
        else if (strcmp(s, "smile") == 0)  fill_buffer(SMILE_PATTERN);
        else if (strcmp(s, "sad") == 0)    fill_buffer(SAD_PATTERN);
        else if (strcmp(s, "heart") == 0)  fill_buffer(HEART_PATTERN);
        
        // 2. アルファベット (A-Z, a-z)
        else {
            // 1文字目を取得し、大文字に変換
            char upper_c = toupper((unsigned char)c);
            
            if (upper_c >= 'A' && upper_c <= 'Z') {
                int index = upper_c - 'A';
                fill_buffer(FONT_ALPHABET[index]);
            }
        }

        update_display();
    }
}

//================================================================
// 初期化
//================================================================
void ledmatrix_init(void) {
  mrbc_class *mrbc_class_led = mrbc_define_class(0, "LEDMatrix", mrbc_class_object);
  mrbc_define_method(0, mrbc_class_led, "clear",   c_ledmatrix_clear);
  mrbc_define_method(0, mrbc_class_led, "write",   c_ledmatrix_write);
  mrbc_define_method(0, mrbc_class_led, "display", c_ledmatrix_display);
}
