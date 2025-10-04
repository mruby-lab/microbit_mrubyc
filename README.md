# **mruby/c on micro:bit for Tinybit Robot**

# 概要

本プロジェクトは、BBC micro:bit 上で動作する軽量Ruby「mruby/c」を用いて、Tinybitロボットを直感的かつ自由に制御するためのファームウェア、および高水準APIライブラリ群を提供するものです。

最終目標は、Smalrubyの設計思想に基づき、専門知識がない人でもScratchのように手軽にRuby言語でロボットプログラミングを楽しめる環境を構築することです。

**主な機能**
安定したファームウェア: PCからのコマンドによるプログラム書き込みと、スタンドアローンでの自動実行に対応。

高水準Ruby API: 以下のクラス/モジュールを通じて、ロボットの機能を直感的に操作できます。

GPIO: LEDなど、汎用的な入出力を制御します。

PWM: モーターの速度など、より細かい出力を制御します。

Tinybit: モーター、RGB LED、各種センサーを統括する、ロボット専用の司令塔です。

LEDMatrix: ボード上の5x5 LEDを、1枚の電光掲示板のように扱えます。

## 開発環境
必要なもの
BBC micro:bit v2

Tinybit ロボット

USBケーブル

ソフトウェア
Arduino IDE: バージョン 1.8.19 で動作確認済み。

micro:bit ボード定義: Arduino IDEのボードマネージャから Arduino nRF528x Boards (Mbed OS) をインストールします。

mruby/c クロスコンパイラ: mrbc コマンドが実行できるように、mrubyのリポジトリから環境を構築する必要があります。（ここに詳細な手順を追記）

## 利用方法
smalruby
https://ceres.epi.it.matsue-ct.ac.jp/smt/


Step 1: ファームウェアの書き込み
このリポジトリの microbit/microbit.ino をArduino IDEで開きます。

ボードとして「BBC micro:bit V2」を選択します。

Arduino IDEの「書き込み」ボタンを押し、コンパイルとファームウェアの転送が完了するのを待ちます。

Step 2: Rubyプログラムの実行
Rubyコードの記述:
test_motor_led.rb のような、動かしたい内容のRubyプログラムを作成します。

バイトコードへのコンパイル:
mrbc コマンドを使い、Rubyプログラムをmruby/cが実行できるバイトコード形式に変換します。
```Ruby
mrbc -o test.mrb test_motor_led.rb
```
シリアルコンソールでの実行:
Arduino IDEのシリアルモニタを開きます。（通信速度: 115200 bps, 改行コード: CR）
以下のコマンドを順番に送信します。

write (バイト数): test.mrbのファイルサイズを調べて入力し、送信します。

(バイトコード本体): test.mrbのバイナリデータをそのまま送信します。

execute: プログラムの実行を開始します。

## APIリファレンス
[利用可能なクラス](doc/class.md)

LEDMatrix.display(character): 1文字を表示します。

使用例:
```Ruby
# 'A'を表示したあと、中央のLEDを点灯
LEDMatrix.display('A')
sleep(1)
LEDMatrix.write(2, 2, 1)
```
