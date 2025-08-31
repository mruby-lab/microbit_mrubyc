/* c_pwm.c */

#include "mrubyc.h"
#include <Arduino.h>

//================================================================
// 外部で定義されたI2Cラッパー関数のプロトタイプ宣言
// NOTE: 本体は microbit.ino でC++のWireライブラリをラップして実装されている
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
// モーターの速度を記憶しておくためのグローバル変数
//================================================================
static int g_left_speed = 0;
static int g_right_speed = 0;

//================================================================
// インスタンスに埋め込むデータ構造
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

  // ★★★ 修正点: I2C通信を既存のラッパー関数に置き換え ★★★
  my_w_beginTrans(I2C_ADDR_PWM);
  for (int i = 0; i < 5; i++) {
    my_w_wire(i2c_data[i]);
  }
  my_w_endTrans();
}

//================================================================
// PWMクラスのメソッドに対応するC言語関数
//================================================================

/**
 * @brief PWM.new(channel) の実質的な処理部分 (initialize)
 * @param channel 0=左モーター, 1=右モーター
 */
static void c_pwm_initialize(mrbc_vm *vm, mrbc_value v[], int argc) {
  PwmHandle *handle = (PwmHandle *)v[0].instance->data;
  if (argc == 1 && v[1].tt == MRBC_TT_FIXNUM) {
    handle->channel = v[1].i;
  }
}

/**
 * @brief pwm.duty(speed) に対応
 * @param speed -255(後進最大) から 255(前進最大)
 */
static void c_pwm_duty(mrbc_vm *vm, mrbc_value v[], int argc) {
  PwmHandle *handle = (PwmHandle *)v[0].instance->data;
  if (argc == 1 && v[1].tt == MRBC_TT_FIXNUM) {
    int speed = v[1].i;

    // 速度を -255 から 255 の範囲に収める
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    // どちらのモーターの速度を更新するか判断
    if (handle->channel == 0) { // 左モーター
      g_left_speed = speed;
    } else { // 右モーター
      g_right_speed = speed;
    }
    
    // I2Cでモーター制御指令を送信
    send_motor_command();
  }
}

/**
 * @brief PWM.new(...) を処理するコンストラクタ
 */
static void c_pwm_new(mrbc_vm *vm, mrbc_value v[], int argc) {
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(PwmHandle));
  mrbc_instance_call_initialize(vm, v, argc);
}

//================================================================
// PWMクラスの初期化
//================================================================
void pwm_init(void) {
  // 1. PWMクラスを定義する
  mrbc_class *mrbc_class_pwm = mrbc_define_class(0, "PWM", mrbc_class_object);

  // 2. メソッドを登録する
  mrbc_define_method(0, mrbc_class_pwm, "new", c_pwm_new);
  mrbc_define_method(0, mrbc_class_pwm, "initialize", c_pwm_initialize);
  mrbc_define_method(0, mrbc_class_pwm, "duty", c_pwm_duty);
}
