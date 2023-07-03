# ccbt-korogaru-koen-park

## Description

- 転がる公園で使用する無線センサノードのプログラム
- 加速度・角加速度・回転角・マイク入力を取得し、無線で送信する
- 通信規格にOSC/UDPを使用する

## Usage

- 18650C と M5StickC-Plus を接続する
- 電源を入れる
    - センサオフセットはキャリブレーション済みのため，電源を入れるだけで使用可能

### WiFiManager

- 起動時に接続済みのアクセスポイントに接続を試みる
- 接続できなかった場合は `WiFiManager` を実行する
  - `ESP32_XXXXXX` というSSIDのアクセスポイントが立ち上がるので，接続すると設定用のポータルが立ち上がる
  - ポータルにアクセスすると，SSIDとパスワードを入力する画面が表示されるので，入力する
  - この時，OSCサーバーとして登録するIPアドレスとポート番号も変更可能（初期設定は `192.168.100.10:9000` ）
   
### OSC Message

OSCサーバーとして登録されたIPアドレスとポート当てに送信する
デフォルトでは，`192.168.100.10:9000` に送信する

#### IMU

60Hzで加速度・角加速度・回転角を送信する

[このように](https://yamaccu.github.io/tils/20220307-M5stickc-6jiku)センサの軸は左手系（左ねじ）である

- `/{client_name}/imu/acc float(x) float(y) float(z)`
- `/{client_name}/imu/gyro float(x) float(y) float(z)`
- `/{client_name}/imu/rotation float(roll) float(pitch)` // x軸回転角，y軸回転角

#### Microphone

30Hzでマイク入力を送信する

`power` は単位 W ではないが入力値の二乗平均平方根

`dB` は単位 [dB] に変換された値

- `/{client_name}/mic/volume float(power) float(dB)`

#### Battery

30秒に一度，バッテリー残量を送信する (**BETA**)

- 電池残量 [%] と充電中かどうかを返すが，ライブラリのバグで正確ではないため参考値
- 充電中は正確な値が返却されないことがある

- `/{client_name}/status/battery float(battery_level) bool(is_charging)`

### Reset

WiFiの接続に不具合が発生した場合や，OSCサーバーのIPアドレスを変更したい場合はAボタン（M5ボタン）を3秒長押しして話すと設定リセットの確認画面が表示されます．
画面遷移後再びAボタンを押すと設定がリセットされます．

リセット後，WiFi設定がめん(青い画面)に戻らない場合は電源ボタンを6秒長押しし，手動で再起動してください


## Test

TODO:

See [test/README.md](test/README.md)