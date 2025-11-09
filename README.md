# KobitoKey_QWERTY

## これは何？
このリポジトリは、[小人キー](https://note.com/11_50iii/n/n75cff4d3502c#48b22323-074f-4017-9605-dbc51b4714ae)と呼ばれる無線・分割ダブルトラックボールキーボードのための ZMK ファームウェア構成です。2 台の Seeeduino XIAO BLE を左右に載せ、左手側をセントラル／右手側をペリフェラルとして動作させつつ、両側に搭載した PMW3610 センサーをポイントデバイスとして扱います。

ZMK の `shield` 構成を自前で用意しており、キー配列・行列・トラックボール設定から BLE スプリット、入力プロセッサ（センサー回転や自動マウスレイヤー制御）までをひとまとめにしています。`west.yml` では PMW3610 ドライバや sensor_rotation、RGB LED ウィジェットといった外部モジュールも取り込んでいます。

## レイヤー構成
- Layer 0: QWERTY（メイン）
- Layer 1: Number & Arrow
- Layer 2: Bluetooth & Function（BT ペアリング、ブートローダー等）
- Layer 3: Auto Mouse（自動でマウスレイヤーへ遷移）

Layer 0
<img width="1280" height="690" alt="Image" src="https://github.com/user-attachments/assets/ef0797b7-a63f-4632-912d-9b5d0115769f" />

Layer 1
<img width="1280" height="690" alt="Image" src="https://github.com/user-attachments/assets/d6347b3c-a238-4278-bacd-e58195774d0e" />

Layer 2
<img width="1280" height="690" alt="Image" src="https://github.com/user-attachments/assets/f1f7cc93-fbd8-4a98-84ea-c8c36ad3952d" />

Layer 3
<img width="1280" height="690" alt="Image" src="https://github.com/user-attachments/assets/2efe5275-e460-41bc-ae45-0c0665435268" />

## ファイル構成と役割
### ルート
- `build.yaml` ― `west build -b seeeduino_xiao_ble -s app -d build` 用のビルドマトリクス。左（`KobitoKey_left rgbled_adapter`）、右（`KobitoKey_right rgbled_adapter`）、`settings_reset` の 3 つのターゲットを一括ビルドします。
- `zephyr/module.yml` ― `board_root: config` を宣言し、`config/boards` 以下の独自シールド定義を Zephyr/ZMK に認識させます.

### boards ディレクトリ
- `boards/shields/KobitoKey/KobitoKey.zmk.yml` ― 公開用のシールドメタデータ。`requires: seeeduino_xiao_ble` など、ZMK に対する基本情報を提供します。

### config ディレクトリ
- `config/west.yml` ― ZMK v0.3.0 をベースに PMW3610 ドライバ、`sensor_rotation`、RGB LED ウィジェットを追加で取得する West マニフェスト。
- `config/KobitoKey.keymap` ― 実際のレイヤー定義。`default_layer`（QWERTY）、`num`、`symbol`、`arrow`、`config` を含み、BT 操作やオートマウス用モディファイアもここで割り当てています。
- `config/KobitoKey.json` ― keymap-drawer / ZMK Studio が参照するレイアウト座標。4 行×10 列の物理位置を記述。

### config/boards/shields/KobitoKey
- `Kconfig.shield` / `Kconfig.defconfig` ― `SHIELD_KOBITOKEY_LEFT/RIGHT` の自動選択と、スプリット／ポイントデバイス／SPI 等の必須オプションをデフォルトで有効化。
- `KobitoKey.zmk.yml` ― 上記と同様のシールド宣言（`config` 側を board_root から参照）。
- `kobitokey.dtsi` ― 4×10 行列スキャン、`matrix-transform`、右手トラックボールを `zmk,input-split` 経由で扱う設定、物理レイアウト選択（`chosen { zmk,physical-layout = ... }`）をまとめた共通 DTSI。
- `KobitoKey-layouts.dtsi` ― `zmk,physical-layout` 形式でキーサイズと座標を記述。keymap 内の順番に対応します。
- `KobitoKey_left.overlay` ― 左手側（セントラル）用デバイスツリー拡張。PMW3610 トラックボール（`tb_left`）、センサー回転、`zip_temp_layer` を用いた自動マウスレイヤー遷移、スクロール方向の変換などの入力プロセッサを定義。
- `KobitoKey_right.overlay` ― 右手側（ペリフェラル）用。列オフセットの適用、右トラックボール（`tb_right`）設定、スプリット入力として中央へ転送するリスナーを宣言。
- `KobitoKey_left.conf` / `KobitoKey_right.conf` ― 各半分の `prj.conf` 相当。BLE スプリット役割、PMW3610 のパラメータ、RGB LED ウィジェット、バッテリーレポートなどを制御します。

これらのファイルを編集することで、キー配列・ポイントデバイス・BLE/Split の挙動・LED 表示など、KobitoKey の動作を ZMK 上で細かく調整できます。
