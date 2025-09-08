#include "mrubyc.h"
#include <string.h> // strcpy, strcat, sprintfのため

//================================================================
// 外部 (microbit.ino) で定義された表示関数のプロトタイプ宣言
//================================================================
extern void c_if_display_show(const char* image_data);

//================================================================
// 内部変数 (画面バッファ)
//================================================================
// 5x5 LEDの状態を記憶する配列。0=OFF, 0以外=ON
static int screen_buffer[5][5] = {0};

//================================================================
// 内部ヘルパー関数
//================================================================

// 画面バッファの内容を元に、物理的なLEDを更新する
static void update_display() {
  char display_str[100] = ""; // "255,0,..." という文字列を格納するバッファ
  char temp_buf[8];

  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 5; x++) {
      // バッファの値が0より大きいなら255(ON), それ以外は0(OFF)
      int brightness = (screen_buffer[y][x] > 0) ? 255 : 0;
      sprintf(temp_buf, "%d", brightness);
      strcat(display_str, temp_buf);

      // 最後でなければカンマを追加
      if (x < 4) {
        strcat(display_str, ",");
      }
    }
    // 最後の行でなければ改行を追加
    if (y < 4) {
      strcat(display_str, "\n");
    }
  }
  // 最終的に生成された文字列で表示を更新
  c_if_display_show(display_str);
}

//================================================================
// LEDMatrixクラスのメソッドに対応するC言語関数
//================================================================

// LEDMatrix.new()
static void c_ledmatrix_new(mrbc_vm *vm, mrbc_value v[], int argc) {
  // LEDはボードに1つしかないので、状態を持つ必要がない
  // そのため、引数なしで空のインスタンスを生成するだけ
  v[0] = mrbc_instance_new(vm, v[0].cls, 0);
}

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

    // 座標が範囲内かチェック
    if (x >= 0 && x < 5 && y >= 0 && y < 5) {
      screen_buffer[y][x] = value;
      update_display();
    }
  }
}

// led.display(char)
static void c_ledmatrix_display(mrbc_vm *vm, mrbc_value v[], int argc) {
    if (argc == 1 && v[1].tt == MRBC_TT_STRING) {
        const char* s = (const char*)v[1].string->data;
        char c = s[0]; // 文字列の最初の1文字を取得

        // まず画面をクリア
        c_ledmatrix_clear(vm, v, 0);

        // 簡単なフォントマップ (例として 'A' と 'B' を実装)
        if (c == 'A') {
            screen_buffer[0][1] = screen_buffer[0][2] = screen_buffer[0][3] = 1;
            screen_buffer[1][0] = screen_buffer[1][4] = 1;
            screen_buffer[2][0] = screen_buffer[2][1] = screen_buffer[2][2] = screen_buffer[2][3] = screen_buffer[2][4] = 1;
            screen_buffer[3][0] = screen_buffer[3][4] = 1;
            screen_buffer[4][0] = screen_buffer[4][4] = 1;
        } else if (c == 'B') {
            screen_buffer[0][0] = screen_buffer[0][1] = screen_buffer[0][2] = screen_buffer[0][3] = 1;
            screen_buffer[1][0] = screen_buffer[1][4] = 1;
            screen_buffer[2][0] = screen_buffer[2][1] = screen_buffer[2][2] = screen_buffer[2][3] = 1;
            screen_buffer[3][0] = screen_buffer[3][4] = 1;
            screen_buffer[4][0] = screen_buffer[4][1] = screen_buffer[4][2] = screen_buffer[4][3] = 1;
        }
        update_display();
    }
}


//================================================================
// LEDMatrixクラスの初期化関数
//================================================================
void ledmatrix_init(void) {
  // "LEDMatrix"という名前のクラスをmruby/cに定義
  mrbc_class *mrbc_class_led = mrbc_define_class(0, "LEDMatrix", mrbc_class_object);

  // メソッドを登録
  mrbc_define_method(0, mrbc_class_led, "new",     c_ledmatrix_new);
  mrbc_define_method(0, mrbc_class_led, "clear",   c_ledmatrix_clear);
  mrbc_define_method(0, mrbc_class_led, "write",   c_ledmatrix_write);
  mrbc_define_method(0, mrbc_class_led, "display", c_ledmatrix_display);
}
