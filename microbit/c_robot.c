#include "mrubyc.h"
#include <Arduino.h>

#define RGB 0x01
#define PWM_ADD 0x01
#define MOTOR 0x02

#if !defined(MRBC_MEMORY_SIZE)
#define MRBC_MEMORY_SIZE (1024*40)
#endif

static uint8_t memory_pool[MRBC_MEMORY_SIZE];

extern const uint8_t mrbbuf[];

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
  if (argc >= 3 && v[1].tt == MRBC_TT_FIXNUM && v[2].tt == MRBC_TT_FIXNUM && v[3].tt == MRBC_TT_FIXNUM) {
    int mode = v[1].i;
    int speed1 = v[2].i;
    int speed2 = v[3].i;

    if (mode < 0 || mode > 6) {
      return;
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
      case 1: // go
        my_w_wire((byte)speed1);
        my_w_wire((byte)0);
        my_w_wire((byte)speed2);
        my_w_wire((byte)0);
        break;
      case 2: // back
        my_w_wire((byte)0);
        my_w_wire((byte)speed1);
        my_w_wire((byte)0);
        my_w_wire((byte)speed2);
        break;
      default:
        break;
    }

    int result = my_w_endTrans();
  }
}

static void c_track_control(struct VM *vm, mrbc_value v[], int argc) {
  const int leftSensorPin = 13; // 左側のトラッキングセンサーのピン番号 (P13)
  const int rightSensorPin = 14; // 右側のトラッキングセンサーのピン番号 (P14)

  while (1) {
    int leftSensorValue = digitalRead(leftSensorPin); // 左側のセンサー値を読み取る
    int rightSensorValue = digitalRead(rightSensorPin); // 右側のセンサー値を読み取る

    // トラッキングセンサーの値をシリアルモニターに表示
    my_print("Left Sensor: ");
    my_print(leftSensorValue);
    my_print(" - Right Sensor: ");
    my_print(rightSensorValue);

    // トラッキングセンサーの値に基づいてカーを制御
    if (leftSensorValue == LOW && rightSensorValue == LOW) {
      // 両方のセンサーが1の場合：直進
      c_move(1,100, 100);
    } else if (leftSensorValue == LOW && rightSensorValue == HIGH) {
      // 左のセンサーが1で右のセンサーが0の場合：左旋回
      c_move(1,100, 0);
    } else if (leftSensorValue == HIGH && rightSensorValue == LOW) {
      // 左のセンサーが0で右のセンサーが1の場合：右旋回
      c_move(1,0, 100);
    } else {
      // それ以外の場合：停止
      c_move(1,0,0);
    }

    delay(1000); // 1秒待機
  }
}



int robot_init(void){

  // Define your own class.
  mrbc_define_method(0, mrbc_class_object, "set_color", c_set_color);
  mrbc_define_method(0, mrbc_class_object, "move", c_move);
  mrbc_define_method(0, mrbc_class_object, "track_control",  c_track_control);
  

  return 0;
}
