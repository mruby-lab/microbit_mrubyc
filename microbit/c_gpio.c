/* c_gpio.c */

#include "mrubyc.h"
#include <Arduino.h> // pinMode, digitalWrite, digitalRead関数などを使うために必要

//================================================================
// グローバルなGPIO関数
//================================================================

/**
 * @brief Rubyの pin_mode(pin, mode) に対応
 */
static void c_gpio_pin_mode(mrbc_vm *vm, mrbc_value v[], int argc) {
  if (argc == 2 && v[1].tt == MRBC_TT_FIXNUM && v[2].tt == MRBC_TT_FIXNUM) {
    int pin = v[1].i;
    int mode = v[2].i;
    pinMode(pin, mode);
  }
}

/**
 * @brief Rubyの gpio_write(pin, value) に対応
 */
static void c_gpio_write(mrbc_vm *vm, mrbc_value v[], int argc) {
  if (argc == 2 && v[1].tt == MRBC_TT_FIXNUM && v[2].tt == MRBC_TT_FIXNUM) {
    int pin = v[1].i;
    int value = v[2].i;
    digitalWrite(pin, value);
  }
}

/**
 * @brief Rubyの gpio_read(pin) に対応
 */
static void c_gpio_read(mrbc_vm *vm, mrbc_value v[], int argc) {
  if (argc == 1 && v[1].tt == MRBC_TT_FIXNUM) {
    int pin = v[1].i;
    int return_value = digitalRead(pin);
    SET_INT_RETURN(return_value);
  } else {
    SET_NIL_RETURN();
  }
}

//================================================================
// GPIO関連の初期化
//================================================================

void gpio_init(void) {
  // グローバルオブジェクトにメソッドを登録する
  mrbc_define_method(0, mrbc_class_object, "pin_mode",   c_gpio_pin_mode);
  mrbc_define_method(0, mrbc_class_object, "gpio_write", c_gpio_write);
  mrbc_define_method(0, mrbc_class_object, "gpio_read",  c_gpio_read);

  // グローバル定数を登録する
  mrbc_set_const(mrbc_str_to_symid("GPIO_IN"),   &mrbc_integer_value(INPUT));
  mrbc_set_const(mrbc_str_to_symid("GPIO_OUT"),  &mrbc_integer_value(OUTPUT));
  mrbc_set_const(mrbc_str_to_symid("GPIO_HIGH"), &mrbc_integer_value(HIGH));
  mrbc_set_const(mrbc_str_to_symid("GPIO_LOW"),  &mrbc_integer_value(LOW));
}
