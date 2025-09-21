/* c_tinybit.c */
// モーター、RGB LED、各種センサーを統括する総合ライブラリ (互換性対応版)

#include "mrubyc.h"
#include <Arduino.h>
#include "c_tinybit.h"

//================================================================
// 外部 I2C / ピン操作 ラッパー関数
//================================================================
extern void my_w_beginTrans(int address);
extern void my_w_wire(int data);
extern void my_w_endTrans(void);

//================================================================
// 定数
//================================================================
#define I2C_ADDR_PCA9685    0x01
#define I2C_CMD_MOTOR       0x02
#define I2C_CMD_RGB_LED     0x01
#define PIN_ULTRASONIC_TRIG 16
#define PIN_ULTRASONIC_ECHO 15
#define PIN_TRACK_L         13
#define PIN_TRACK_R         14

//================================================================
// 内部変数 (司令室)
//================================================================
static int g_left_speed = 0;
static int g_right_speed = 0;

//================================================================
// Part 1: PWMクラス (Smalrubyガイドライン準拠)
//================================================================
typedef struct PwmHandle { int channel; } PwmHandle;

static void send_motor_command() {
  uint8_t i2c_data[5];
  i2c_data[0] = I2C_CMD_MOTOR;
  if (g_left_speed >= 0) { i2c_data[1] = g_left_speed; i2c_data[2] = 0; }
  else { i2c_data[1] = 0; i2c_data[2] = -g_left_speed; }
  if (g_right_speed >= 0) { i2c_data[3] = g_right_speed; i2c_data[4] = 0; }
  else { i2c_data[3] = 0; i2c_data[4] = -g_right_speed; }
  my_w_beginTrans(I2C_ADDR_PCA9685);
  for (int i = 0; i < 5; i++) { my_w_wire(i2c_data[i]); }
  my_w_endTrans();
}

static void c_pwm_initialize(mrbc_vm *vm, mrbc_value v[], int argc) {
  PwmHandle *handle = (PwmHandle *)v[0].instance->data;
  if (argc == 1 && v[1].tt == MRBC_TT_FIXNUM) { handle->channel = v[1].i; }
}

static void c_pwm_duty(mrbc_vm *vm, mrbc_value v[], int argc) {
  PwmHandle *handle = (PwmHandle *)v[0].instance->data;
  if (argc == 1 && v[1].tt == MRBC_TT_FIXNUM) {
    int speed = v[1].i;
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;
    if (handle->channel == 0) g_left_speed = speed;
    else g_right_speed = speed;
    send_motor_command();
  }
}

static void c_pwm_new(mrbc_vm *vm, mrbc_value v[], int argc) {
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(PwmHandle));
  mrbc_instance_call_initialize(vm, v, argc);
}

//================================================================
// Part 2: Tinybitクラス (高水準API)
//================================================================

// Tinybit.new()
static void c_tinybit_new(mrbc_vm *vm, mrbc_value v[], int argc) {
    v[0] = mrbc_instance_new(vm, v[0].cls, 0);
}

// tinybit.move(:direction, left_speed, right_speed)
static void c_tinybit_move(mrbc_vm *vm, mrbc_value v[], int argc) {
  if (argc != 3 || v[1].tt != MRBC_TT_SYMBOL || v[2].tt != MRBC_TT_FIXNUM || v[3].tt != MRBC_TT_FIXNUM) return;
  mrbc_sym direction_sym = v[1].i;
  int left_speed = v[2].i;
  int right_speed = v[3].i;
  if (left_speed < 0) left_speed = 0;
  if (left_speed > 255) left_speed = 255;
  if (right_speed < 0) right_speed = 0;
  if (right_speed > 255) right_speed = 255;
  if (direction_sym == mrbc_str_to_symid("back")) { left_speed = -left_speed; right_speed = -right_speed; }
  else if (direction_sym == mrbc_str_to_symid("stop")) { left_speed = 0; right_speed = 0; }
  g_left_speed = left_speed;
  g_right_speed = right_speed;
  send_motor_command();
}

// tinybit.set_led_color(r, g, b)
static void c_tinybit_set_led_color(mrbc_vm *vm, mrbc_value v[], int argc) {
  if (argc == 3 && v[1].tt == MRBC_TT_FIXNUM && v[2].tt == MRBC_TT_FIXNUM && v[3].tt == MRBC_TT_FIXNUM) {
    my_w_beginTrans(I2C_ADDR_PCA9685);
    my_w_wire(I2C_CMD_RGB_LED);
    my_w_wire(v[1].i); my_w_wire(v[2].i); my_w_wire(v[3].i);
    my_w_endTrans();
  }
}

// tinybit.ultrasonic_distance() -> Integer
static void c_tinybit_ultrasonic_distance(mrbc_vm *vm, mrbc_value v[], int argc) {
  pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

  // ★★★ pulseIn()を安定した手動計測に変更 ★★★
  unsigned long duration = 0;
  unsigned long timeout = 50000; // 50msのタイムアウト
  unsigned long start_time = micros();
  while(digitalRead(PIN_ULTRASONIC_ECHO) == LOW) {
    if (micros() - start_time > timeout) return;
  }
  start_time = micros();
  while(digitalRead(PIN_ULTRASONIC_ECHO) == HIGH) {
    if (micros() - start_time > timeout) return;
  }
  duration = micros() - start_time;

  long distance = duration * 0.034 / 2;
  SET_INT_RETURN(distance);
}

// tinybit.read_track_sensors() -> [Integer, Integer]
static void c_tinybit_read_track_sensors(mrbc_vm *vm, mrbc_value v[], int argc) {
  pinMode(PIN_TRACK_L, INPUT);
  pinMode(PIN_TRACK_R, INPUT);
  int left_val = digitalRead(PIN_TRACK_L);
  int right_val = digitalRead(PIN_TRACK_R);
  mrbc_value array = mrbc_array_new(vm, 2);
  mrbc_array_set(&array, 0, &mrbc_integer_value(left_val));
  mrbc_array_set(&array, 1, &mrbc_integer_value(right_val));
  SET_RETURN(array);
}

//================================================================
// 初期化関数
//================================================================
void tinybit_init(void) {
  mrbc_class *mrbc_class_pwm = mrbc_define_class(0, "PWM", mrbc_class_object);
  mrbc_define_method(0, mrbc_class_pwm, "new", c_pwm_new);
  mrbc_define_method(0, mrbc_class_pwm, "initialize", c_pwm_initialize);
  mrbc_define_method(0, mrbc_class_pwm, "duty", c_pwm_duty);

  //  mrbc_define_moduleからmrbc_define_classに変更 
  mrbc_class *mrbc_class_tinybit = mrbc_define_class(0, "Tinybit", mrbc_class_object);
  mrbc_define_method(0, mrbc_class_tinybit, "new", c_tinybit_new);
  mrbc_define_method(0, mrbc_class_tinybit, "move", c_tinybit_move);
  mrbc_define_method(0, mrbc_class_tinybit, "set_led_color", c_tinybit_set_led_color);
  mrbc_define_method(0, mrbc_class_tinybit, "ultrasonic_distance", c_tinybit_ultrasonic_distance);
  mrbc_define_method(0, mrbc_class_tinybit, "read_track_sensors", c_tinybit_read_track_sensors);
}
