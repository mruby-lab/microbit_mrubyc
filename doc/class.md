## **GPIO クラス**

汎用的なデジタル入出力を制御します。

- GPIO.new(pin, mode): オブジェクトを生成します。

pin: ピン番号 (Integer)

mode: GPIO::IN または GPIO::OUT

gpio.write(value): ピンに値を出力します。

value: GPIO::HIGH または GPIO::LOW

gpio.read(): ピンから値を読み取ります。

使用例:
```Ruby
# 0番ピンに接続されたLEDを1秒ごとに点滅させる
led = GPIO.new(0, GPIO::OUT)

loop do
  led.write(GPIO::HIGH)
  sleep(1)
  led.write(GPIO::LOW)
  sleep(1)
end
```
## PWM クラス
モーターなど、より細かい出力を制御します。

PWM.new(channel): オブジェクトを生成します。

channel: 0 (左モーター), 1 (右モーター)

pwm.duty(speed): 速度と回転方向を指定します。

speed: -255 (最大後進) 〜 255 (最大前進)

使用例:
```Ruby
# 左右のモーターを準備
left_motor = PWM.new(0)
right_motor = PWM.new(1)

# 右にスピン
left_motor.duty(200)
right_motor.duty(-200)
```
## Tinybit クラス(メインクラス）
ロボットの機能を統括する司令塔です。

Tinybit.new(): 司令塔オブジェクトを生成します。

tinybit.move(:direction, left_speed, right_speed): ロボットを動かします。

:direction: :forward, :back, :stop

left_speed, right_speed: 0 〜 255

tinybit.set_led_color(r, g, b): RGB LEDの色を設定します。

tinybit.ultrasonic_distance(): 超音波センサーで距離(cm)を測定します。

tinybit.read_track_sensors(): ライントレースセンサーの値を [左, 右] の配列で返します。

使用例:
```Ruby
tinybit = Tinybit.new()
tinybit.set_led_color(0, 0, 255) # 青色に
tinybit.move(:forward, 150, 150)
```
```Ruby
# インスタンス生成
car = Tinybit.new()
led = LEDMatrix.new()
led.display("GO") # 開始表示
loop do
dist = car.ultrasonic_distance()
# 距離が20以上cmかつエラー,(-1)でない場合
if dist >= 20 || dist ==-1
car.move(:forward, 100, 100)
led.clear()
else
# 障害物を検知して停止
car.move(:stop, 0, 0)
led.display("sad")
end
sleep 0.1
end
```

```Ruby
# インスタンスの生成
car = Tinybit.new()

# 無限ループでライントレースを実行
while true
  # センサの値を取得 (1: 黒, 0: 白 と仮定)
  left_val = car.line_sensor_left()
  right_val = car.line_sensor_right()

  if left_val == 1 && right_val == 1
    # 両方黒なら前進
    car.move(:forward, 80, 80)
    
  elsif left_val == 1 && right_val == 0
    # 左だけ黒なら左へ曲がる
    car.move(:forward, 0, 80)
    
  elsif left_val == 0 && right_val == 1
    # 右だけ黒なら右へ曲がる
    car.move(:forward, 80, 0)
    
  else
    # 両方白なら停止（コースアウト）
    car.move(:stop, 0, 0)
    break
  end

  sleep(0.05)
end
```

## LEDMatrix モジュール
ボード上の5x5 LEDを制御します。(newは不要)

LEDMatrix.clear(): 全てのLEDを消灯します。

LEDMatrix.write(x, y, value): 指定した座標のLEDを点灯/消灯します。

LEDMatrix.display(character): 1文字を表示します。

使用例:
```Ruby
# 'A'を表示したあと、中央のLEDを点灯
LEDMatrix.display('A')
sleep(1)
LEDMatrix.write(2, 2, 1)
```
