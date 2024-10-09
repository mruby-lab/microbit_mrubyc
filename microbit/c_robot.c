#include "mrubyc.h"
#include <Arduino.h>

#define RGB 0x01
#define PWM_ADD 0x01
#define MOTOR 0x02
#define TRIG_PIN 16  // 超音波センサーのTRIGピン（送信）
#define ECHO_PIN 15  // 超音波センサーのECHOピン（受信）


#if !defined(MRBC_MEMORY_SIZE)
#define MRBC_MEMORY_SIZE (1024*40)
#endif

static uint8_t memory_pool[MRBC_MEMORY_SIZE];

extern const uint8_t mrbbuf[];

int leftSensorValue = 0;
int rightSensorValue = 0;
long distance = 0;

static void c_set_color(struct VM *vm, mrbc_value v[], int argc) {
  if (argc == 3 && v[1].tt == MRBC_TT_FIXNUM && v[2].tt == MRBC_TT_FIXNUM && v[3].tt == MRBC_TT_FIXNUM) {
    int red = v[1].i;
    int green = v[2].i;
    int blue = v[3].i;

    my_w_beginTrans(PWM_ADD); // Wire.beginTransmission
    my_w_wire(RGB); //Wire.write
    my_w_wire(red);
    my_w_wire(green);
    my_w_wire(blue);
    int result =  my_w_endTrans(); //Wire.endTransmission
   
  } 
}


static void c_move(struct VM *vm, mrbc_value v[], int argc) {
  if (argc >= 2 && v[1].tt == MRBC_TT_SYMBOL && v[2].tt == MRBC_TT_HASH) {
    // シンボルIDを取得
    mrbc_sym mode_symbol = v[1].i;  // v[1].iはシンボルID（整数）として扱う

    // デフォルト値を設定
    int left_speed = 0;
    int right_speed = 0;

    // キーワード引数のハッシュから `left` と `right` の値を取得
    mrbc_value left_val = mrbc_hash_get(&v[2], &mrbc_symbol_value(mrbc_str_to_symid("left")));
    mrbc_value right_val = mrbc_hash_get(&v[2], &mrbc_symbol_value(mrbc_str_to_symid("right")));
    
    // `left` と `right` がFixnumかを確認し、速度を取得
    if (left_val.tt == MRBC_TT_FIXNUM) {
      left_speed = left_val.i;
    }
    if (right_val.tt == MRBC_TT_FIXNUM) {
      right_speed = right_val.i;
    }

    int mode = -1;  // 無効なモード

    // シンボルIDに基づいてモードを決定
    if (mode_symbol == mrbc_str_to_symid("forward")) {
      mode = 1;  // forward
    } else if (mode_symbol == mrbc_str_to_symid("backward")) {
      mode = 2;  // backward
    } else if (mode_symbol == mrbc_str_to_symid("stop")) {
      mode = 0;  // stop
    }

    if (mode < 0 || mode > 6) {
      return;  // 無効なモードの場合は処理しない
    }

    my_w_beginTrans(PWM_ADD);
    my_w_wire(MOTOR);

    switch (mode) {
      case 0: // stop
        my_w_wire((byte)0);
        my_w_wire((byte)0);
        my_w_wire((byte)0);
        my_w_wire((byte)0);
        break;
      case 1: // go (forward)
        my_w_wire((byte)left_speed);
        my_w_wire((byte)0);
        my_w_wire((byte)right_speed);
        my_w_wire((byte)0);
        break;
      case 2: // back (backward)
        my_w_wire((byte)0);
        my_w_wire((byte)left_speed);
        my_w_wire((byte)0);
        my_w_wire((byte)right_speed);
        break;
      default:
        break;
    }

    my_w_endTrans();
  }
}




static void c_track_sensor(struct VM *vm, mrbc_value v[], int argc) {
  const int leftSensorPin = 13; // 左側のトラッキングセンサーのピン番号 (P13)
  const int rightSensorPin = 14; // 右側のトラッキングセンサーのピン番号 (P14)

  pinMode(leftSensorPin, INPUT); // 左トラッキングセンサーのピンを入力モードに設定
  pinMode(rightSensorPin, INPUT); // 右トラッキングセンサーのピンを入力モードに設定

  leftSensorValue = digitalRead(leftSensorPin); // 左側のセンサー値を読み取る
  rightSensorValue = digitalRead(rightSensorPin); // 右側のセンサー値を読み取る

  // トラッキングセンサーの値をシリアルモニターに表示
  my_print("Left Sensor: ");
  my_print(leftSensorValue);
  my_print(" - Right Sensor: ");
  my_println(rightSensorValue);

  delay(1000); // 1秒待機
}

static void c_get_left_sensor_value(struct VM *vm, mrbc_value v[], int argc) {
  v[0] = mrbc_integer_value(leftSensorValue);
}

static void c_get_right_sensor_value(struct VM *vm, mrbc_value v[], int argc) {
  v[0] = mrbc_integer_value(rightSensorValue);
}

static void c_ultrasonic_distance(struct VM *vm, mrbc_value v[], int argc) {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // TRIGピンにパルスを送信
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // パルスの持続時間を手動で計測
  unsigned long startTime, endTime;
  startTime = micros();
  while (digitalRead(ECHO_PIN) == LOW);  // ECHOピンがHIGHになるまで待機
  startTime = micros();  // パルス受信開始時間を記録
  while (digitalRead(ECHO_PIN) == HIGH); // ECHOピンがLOWになるまで待機
  endTime = micros();  // パルス受信終了時間を記録

  // パルスの持続時間を計算
  long duration = endTime - startTime;

  // 距離を計算 (音速343m/s => 0.034 cm/μs)
  distance = duration * 0.034 / 2;

  // 測定した距離を返す
  v[0] = mrbc_integer_value(distance);
}




int robot_init(void){

  // Define your own class.
  mrbc_define_method(0, mrbc_class_object, "set_color", c_set_color);
  mrbc_define_method(0, mrbc_class_object, "move", c_move);
  mrbc_define_method(0, mrbc_class_object, "track_sensor",  c_track_sensor);
  mrbc_define_method(0, mrbc_class_object, "get_left_sensor_value", c_get_left_sensor_value);
  mrbc_define_method(0, mrbc_class_object, "get_right_sensor_value", c_get_right_sensor_value);
  mrbc_define_method(0, mrbc_class_object, "ultrasonic_distance", c_ultrasonic_distance);


  return 0;
}
