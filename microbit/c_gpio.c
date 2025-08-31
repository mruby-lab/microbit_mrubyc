/* c_gpio.c */

#include "mrubyc.h"
#include <Arduino.h> // pinMode, digitalWrite, digitalRead関数などを使うために必要

//================================================================
// インスタンスに直接埋め込むデータ構造
// NOTE: オブジェクトごとに、この構造体の分のメモリが確保される
//================================================================
typedef struct GpioHandle {
  int pin_num;
} GpioHandle;


//================================================================
// GPIOクラスのメソッドに対応するC言語関数
//================================================================

/**
 * @brief GPIO.new(pin, mode) の実質的な処理部分 (initialize)
 * @note newから呼び出され、ピン番号をインスタンスデータに保存し、
 * ピンモードを設定する。
 */
static void c_gpio_initialize(mrbc_vm *vm, mrbc_value v[], int argc) {
  // インスタンスに埋め込まれたデータ領域へのポインタを取得
  GpioHandle *handle = (GpioHandle *)v[0].instance->data;

  // 引数が2つ(pin, mode)で、どちらも整数であることを確認
  if (argc == 2 && v[1].tt == MRBC_TT_FIXNUM && v[2].tt == MRBC_TT_FIXNUM) {
    int pin = v[1].i;
    int mode = v[2].i;

    // 取得したピン番号を、インスタンスのデータ領域に保存
    handle->pin_num = pin;
    
    // ピンモードを設定
    pinMode(pin, mode);
  }
}


/**
 * @brief gpio.write(value) に対応
 */
static void c_gpio_write(mrbc_vm *vm, mrbc_value v[], int argc) {
  // インスタンスのデータ領域から、保存しておいたピン番号を取り出す
  GpioHandle *handle = (GpioHandle *)v[0].instance->data;
  int pin = handle->pin_num;

  // 引数が1つ(value)で、整数であることを確認
  if (argc == 1 && v[1].tt == MRBC_TT_FIXNUM) {
    int value = v[1].i;
    digitalWrite(pin, value);
  }
}

/**
 * @brief gpio.read() に対応
 */
static void c_gpio_read(mrbc_vm *vm, mrbc_value v[], int argc) {
  // インスタンスのデータ領域から、保存しておいたピン番号を取り出す
  GpioHandle *handle = (GpioHandle *)v[0].instance->data;
  int pin = handle->pin_num;

  int return_value = digitalRead(pin);
  SET_INT_RETURN(return_value);
}


/**
 * @brief GPIO.new(...) を処理するコンストラクタ
 * @note 新しいインスタンスを生成し、initializeを呼び出すのが役割
 */
static void c_gpio_new(mrbc_vm *vm, mrbc_value v[], int argc) {
  // GpioHandle構造体のサイズで、新しいインスタンスを生成
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(GpioHandle));
  
  // 引数をそのまま渡して c_gpio_initialize を呼び出す
  mrbc_instance_call_initialize(vm, v, argc);
}


//================================================================
// GPIOクラスの初期化
//================================================================
void gpio_init(void) {
  // 1. GPIOクラスを定義する
  mrbc_class *mrbc_class_gpio = mrbc_define_class(0, "GPIO", mrbc_class_object);

  // 2. メソッドを登録する
  //    クラスメソッドとして "new" を登録
  mrbc_define_method(0, mrbc_class_gpio, "new", c_gpio_new);
  //    インスタンスメソッドとして "initialize", "write", "read" を登録
  mrbc_define_method(0, mrbc_class_gpio, "initialize", c_gpio_initialize);
  mrbc_define_method(0, mrbc_class_gpio, "write",      c_gpio_write);
  mrbc_define_method(0, mrbc_class_gpio, "read",       c_gpio_read);

  // 3. クラス定数を登録する (GPIO::OUT のように書ける)
  mrbc_set_class_const(mrbc_class_gpio, mrbc_str_to_symid("IN"),   &mrbc_integer_value(INPUT));
  mrbc_set_class_const(mrbc_class_gpio, mrbc_str_to_symid("OUT"),  &mrbc_integer_value(OUTPUT));
  mrbc_set_class_const(mrbc_class_gpio, mrbc_str_to_symid("HIGH"), &mrbc_integer_value(HIGH));
  mrbc_set_class_const(mrbc_class_gpio, mrbc_str_to_symid("LOW"),  &mrbc_integer_value(LOW));
}
