/* c_motor_api.c */
// NOTE: 既存の c_pwm.c を拡張し、新しくTinybitクラスを追加したものです。
// このファイルで c_pwm.c を置き換えてください。

#include "mrubyc.h"
#include <Arduino.h>

//================================================================
// 外部で定義されたI2Cラッパー関数のプロトタイプ宣言
//================================================================
extern void my_w_beginTrans(int address);
extern void my_w_wire(int data);
extern void my_w_endTrans(void);

//================================================================
// Tinybit I2C制御用の定数
//================================================================
#define I2C_ADDR_PWM 0x01
#define I2C_CMD_MOTOR 0x02

//================================================================
// モーターの速度を記憶しておくためのグローバル変数 (司令室)
//================================================================
static int g_left_speed = 0;
static int g_right_speed = 0;

//================================================================
// インスタンスに埋め込むデータ構造 (PWMクラス用)
//================================================================
typedef struct PwmHandle {
  int channel; // 0: 左モーター, 1: 右モーター
} PwmHandle;

//================================================================
// I2Cでモーター制御指令を送信する内部関数
//================================================================
static void send_motor_command() {
  uint8_t i2c_data[5];
  i2c_data[0] = I2C_CMD_MOTOR;

  // 左モーターの速度を設定
  if (g_left_speed >= 0) {
    i2c_data[1] = g_left_speed;
    i2c_data[2] = 0;
  } else {
    i2c_data[1] = 0;
    i2c_data[2] = -g_left_speed;
  }

  // 右モーターの速度を設定
  if (g_right_speed >= 0) {
    i2c_data[3] = g_right_speed;
    i2c_data[4] = 0;
  } else {
    i2c_data[3] = 0;
    i2c_data[4] = -g_right_speed;
  }

  my_w_beginTrans(I2C_ADDR_PWM);
  for (int i = 0; i < 5; i++) {
    my_w_wire(i2c_data[i]);
  }
  my_w_endTrans();
}

//================================================================
// ====== Part 1: 既存のPWMクラス (ガイドライン準拠) ======
//================================================================
static void c_pwm_initialize(mrbc_vm *vm, mrbc_value v[], int argc) {
  PwmHandle *handle = (PwmHandle *)v[0].instance->data;
  if (argc == 1 && v[1].tt == MRBC_TT_FIXNUM) {
    handle->channel = v[1].i;
  }
}

static void c_pwm_duty(mrbc_vm *vm, mrbc_value v[], int argc) {
  PwmHandle *handle = (PwmHandle *)v[0].instance->data;
  if (argc == 1 && v[1].tt == MRBC_TT_FIXNUM) {
    int speed = v[1].i;
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    if (handle->channel == 0) {
      g_left_speed = speed;
    } else {
      g_right_speed = speed;
    }
    send_motor_command();
  }
}

static void c_pwm_new(mrbc_vm *vm, mrbc_value v[], int argc) {
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(PwmHandle));
  mrbc_instance_call_initialize(vm, v, argc);
}


//================================================================
// ====== Part 2: ★★★ Tinybitクラス (インスタンスメソッド版) ★★★ ======
//================================================================
/**
 * @brief tinybit.move(:direction, left_speed, right_speed) に対応
 */
static void c_tinybit_move(mrbc_vm *vm, mrbc_value v[], int argc) {
  // 引数が3つ (direction, left_speed, right_speed) であることを確認
  if (argc != 3 || v[1].tt != MRBC_TT_SYMBOL || v[2].tt != MRBC_TT_FIXNUM || v[3].tt != MRBC_TT_FIXNUM) {
    return; // 引数の型や数が違う場合は何もしない
  }

  // シンボルIDを取得
  mrbc_sym direction_sym = v[1].i;

  // 速度を取得
  int left_speed = v[2].i;
  int right_speed = v[3].i;

  // 速度を 0 から 255 の範囲に収める (方向はシンボルで指定するため、正の値のみ)
  if (left_speed < 0) left_speed = 0;
  if (left_speed > 255) left_speed = 255;
  if (right_speed < 0) right_speed = 0;
  if (right_speed > 255) right_speed = 255;

  // シンボルに応じて、速度の符号を決定する
  if (direction_sym == mrbc_str_to_symid("forward")) {
    // 前進: 速度はそのまま (正)
  } else if (direction_sym == mrbc_str_to_symid("back")) {
    // 後退: 速度を負にする
    left_speed = -left_speed;
    right_speed = -right_speed;
  } else if (direction_sym == mrbc_str_to_symid("stop")) {
    // 停止
    left_speed = 0;
    right_speed = 0;
  } else {
    // 未知のシンボルの場合は何もしない
    return;
  }

  // グローバル変数を更新 (司令室)
  g_left_speed = left_speed;
  g_right_speed = right_speed;

  // I2Cでモーター制御指令を送信
  send_motor_command();
}

/**
 * @brief Tinybit.new() を処理するコンストラクタ
 */
static void c_tinybit_new(mrbc_vm *vm, mrbc_value v[], int argc) {
  // このクラスは状態を持たないので、データ領域は0で良い
  v[0] = mrbc_instance_new(vm, v[0].cls, 0);
}


//================================================================
// mruby/cにクラスとメソッドを登録する初期化関数
//================================================================
void motor_api_init(void) {
  // --- 従来のPWMクラスを定義 ---
  mrbc_class *mrbc_class_pwm = mrbc_define_class(0, "PWM", mrbc_class_object);
  mrbc_define_method(0, mrbc_class_pwm, "new", c_pwm_new);
  mrbc_define_method(0, mrbc_class_pwm, "initialize", c_pwm_initialize);
  mrbc_define_method(0, mrbc_class_pwm, "duty", c_pwm_duty);

  // --- ★★★ 新しいTinybitクラスを定義 (インスタンスメソッド版) ★★★ ---
  mrbc_class *mrbc_class_tinybit = mrbc_define_class(0, "Tinybit", mrbc_class_object);
  // インスタンスメソッドとして "new" と "move" を登録
  mrbc_define_method(0, mrbc_class_tinybit, "new", c_tinybit_new);
  mrbc_define_method(0, mrbc_class_tinybit, "move", c_tinybit_move);
}
