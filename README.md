# KobitoKey_QWERTY

## プロジェクト概要
[小人キー](https://note.com/11_50iii/n/n75cff4d3502c#48b22323-074f-4017-9605-dbc51b4714ae)向けの ZMK ファームウェアです。左右の Seeeduino XIAO BLE に PMW3610 トラックボールを載せ、左手側をセントラル、右手側をペリフェラルとして動かします。`config` ディレクトリを board root に指定し、独自シールドと周辺モジュール（PMW3610 ドライバ、sensor_rotation、RGB LED ウィジェット）を West で取得します。

ビルドは `build.yaml` のマトリクス（`KobitoKey_left rgbled_adapter`、`KobitoKey_right rgbled_adapter`、`settings_reset`）を `west build` で順に生成する想定です。キー配列は `config/KobitoKey.keymap`、レイアウト図は `config/KobitoKey.json` がソースになります。

## レイヤー配置

| Layer | 名称 | 主な機能 | レイアウト |
| --- | --- | --- | --- |
| 0 | QWERTY | ベース入力 | <img width="210" alt="Layer0" src="https://github.com/user-attachments/assets/ef0797b7-a63f-4632-912d-9b5d0115769f" /> |
| 1 | Number & Arrow | 数字列、矢印、メディアキー | <img width="210" alt="Layer1" src="https://github.com/user-attachments/assets/d6347b3c-a238-4278-bacd-e58195774d0e" /> |
| 2 | Bluetooth & Function | BT ペアリング、ブートローダー、設定操作 | <img width="210" alt="Layer2" src="https://github.com/user-attachments/assets/f1f7cc93-fbd8-4a98-84ea-c8c36ad3952d" /> |
| 3 | Auto Mouse | 自動遷移するマウスレイヤーとスクロール処理 | <img width="210" alt="Layer3" src="https://github.com/user-attachments/assets/2efe5275-e460-41bc-ae45-0c0665435268" /> |
| 5 | Trackball Keys | `config` レイヤー（Layer 4）1 行目 3 列目の `&tog 5` でオン／オフ。右トラックボールが矢印キー、左は Globe+矢印マクロを吐き出す。 | 手入力レイヤーのため図なし |

## 主要ディレクトリ
- `build.yaml` ― `west build -b seeeduino_xiao_ble -s app -d build` 用のビルドマトリクス。左（`KobitoKey_left rgbled_adapter`）、右（`KobitoKey_right rgbled_adapter`）、`settings_reset` を一括ビルド。
- `zephyr/module.yml` ― `board_root: config` を宣言しつつ、`trackball_module` を Zephyr モジュールとして登録してカスタム input processor をビルドに含める。
- `trackball_module/` ― `src/dir_key_processor.c` に「トラックボール移動 → 方向キー／マクロ出力」用のカスタム input processor を実装。`dts/bindings/input/zmk,input-processor-dir-keys.yaml` で Devicetree から使えるようにし、`CMakeLists.txt` で ZMK に組み込みます。Layer 5 の閾値や発火ロジックを変える際はここを編集。

## トラックボール動作
- **通常時（Layer 0〜4）**  
  - 右トラックボール → `sensor_rotation_right` ＋ `zip_temp_layer` のチェーン経由でマウスカーソル。  
  - 左トラックボール → `tb_left_listener` による縦 15 倍／横 6 倍スケーリング＋`zip_xy_to_scroll_mapper` で縦横スクロール（ホイール／ホイール横列の両方を送出）。
- **Trackball Keys（Layer 5）**  
  - `config` レイヤー（Layer 4）1 行目 3 列目に追加した `&tog 5` をタップして有効化／再タップで解除。  
  - 右トラックボール → `tb_right_arrow_mapper` が移動量を閾値ごとに `&kp RIGHT/LEFT/UP/DOWN` へ変換し、スプレッドシートなどでセルを滑らかに移動。  
  - 左トラックボール → `tb_left_globe_mapper` が `&globe_swipe_*` マクロを呼び出し、Globe+矢印ショートカット（iPad のウィンドウ／ステージ移動）を方向別に送信。  
  - 閾値は `KobitoKey_left.overlay` 内の `threshold = <6>;` を編集するだけで調整可能。さらに挙動を変えたい場合は `trackball_module/src/dir_key_processor.c` を編集。

## `config/boards/shields/KobitoKey` の構成

```
config/boards/shields/KobitoKey
├─ Kconfig.shield          ── Shield 選択フラグを定義
├─ Kconfig.defconfig       ── Shield 有効時の既定 Kconfig
├─ KobitoKey.zmk.yml       ── ZMK へ公開するメタデータ
├─ KobitoKey_left.conf     ── 左手側の prj.conf
├─ KobitoKey_right.conf    ── 右手側の prj.conf
├─ kobitokey.dtsi          ── 行列・split 入力の共通定義
├─ KobitoKey-layouts.dtsi  ── 物理レイアウト座標
├─ KobitoKey_left.overlay  ── 左手用デバイスツリー拡張
└─ KobitoKey_right.overlay ── 右手用デバイスツリー拡張
```

依存関係の流れ:

```
shields_list → [Kconfig.shield] → SHIELD_KOBITOKEY_* → [Kconfig.defconfig]
                                           │
                                           └─ [KobitoKey_left.conf | KobitoKey_right.conf]

[KobitoKey_left.overlay] ─┐
[KobitoKey_right.overlay] ├─ include → [kobitokey.dtsi] ──→ [KobitoKey-layouts.dtsi]
[KobitoKey.zmk.yml]       ┘
```

### 編集カテゴリ別ガイド

#### よく調整するファイル（キー配列・ポインティング・BLE 挙動を変えたい時）

| パス | 役割 | 読み込み順・依存関係 | 変更理由の例 |
| --- | --- | --- | --- |
| [config/KobitoKey.keymap](config/KobitoKey.keymap) | 論理レイヤーとコンボを定義 | `zmk,keymap` ノードとして直接ビルドに含まれる | レイヤー構成、モディファイア、BT 操作の割り当てを変えたい時 |
| [config/KobitoKey.json](config/KobitoKey.json) | Studio や keymap-drawer 向けのキー座標 | `keymap.yaml` 生成や図の描画で参照 | レイアウト図やキーの描画位置を更新したい時 |
| [KobitoKey_left.overlay](config/boards/shields/KobitoKey/KobitoKey_left.overlay) | 左手側トラックボール、センサー回転、`zip_temp_layer`、Layer 5 での Globe マクロ実行を制御 | `kobitokey.dtsi` を include し、`tb_left_listener` や zip 系プロセッサ、`tb_left_globe_mapper` を有効化 | 左トラックボールの CPI、スクロール方向、Trackball Keys レイヤーのマクロ内容や閾値を調整したい時 |
| [KobitoKey_right.overlay](config/boards/shields/KobitoKey/KobitoKey_right.overlay) | 右手側トラックボール、列オフセット、split 送信を制御 | `kobitokey.dtsi` を include し、`tb_right_split` を介して送信 | 右トラックボールの CPI や送信方法、行列オフセットを変えたい時 |
| [KobitoKey_left.conf](config/boards/shields/KobitoKey/KobitoKey_left.conf) | セントラル側の `prj.conf`。BLE、RGB LED、PMW3610 を設定 | `west build -b seeeduino_xiao_ble -- -DSHIELD=KobitoKey_left` でマージ | セントラル役割の BLE 動作、RGB 層色、PMW3610 パラメータを調整したい時 |
| [KobitoKey_right.conf](config/boards/shields/KobitoKey/KobitoKey_right.conf) | ペリフェラル側の `prj.conf`。PMW3610 と BLE ペリフェラルを設定 | `west build ... -DSHIELD=KobitoKey_right` でマージ | 右手の CPI、BLE 設定、バッテリー報告を変えたい時 |

#### 基本的に固定で、配線や構成を大幅に変える場合のみ触るファイル

| パス | 役割 | 読み込み順・依存関係 | 変更理由の例 |
| --- | --- | --- | --- |
| [Kconfig.shield](config/boards/shields/KobitoKey/Kconfig.shield) | `SHIELD_KOBITOKEY_LEFT/RIGHT` を `west build` に登録 | `west build -S` 時に `shields_list_contains` が評価 | Shield 名称を変える、新しいバリエーションを追加する時 |
| [Kconfig.defconfig](config/boards/shields/KobitoKey/Kconfig.defconfig) | Shield 有効時の既定 Kconfig を宣言 | `SHIELD_KOBITOKEY_*` が真になった際のみ読み込み | スプリット機能や SPI を既定で無効化したい特別事情がある時 |
| [KobitoKey.zmk.yml](config/boards/shields/KobitoKey/KobitoKey.zmk.yml) | Shield メタデータ（`requires`, `features`）を ZMK に提示 | `west build` が Shield を解決する際に参照 | 対応ボードを追加する、リポジトリ URL を変える時 |
| [kobitokey.dtsi](config/boards/shields/KobitoKey/kobitokey.dtsi) | 4×10 行列スキャン、`matrix-transform`, split 入力、物理レイアウト選択を一括定義 | 左右 overlay から `#include` され、`KobitoKey-layouts.dtsi` を参照 | 行列配線や split デバイス構造を作り直す時 |
| [KobitoKey-layouts.dtsi](config/boards/shields/KobitoKey/KobitoKey-layouts.dtsi) | `zmk,physical-layout` とキー座標を列挙 | `kobitokey.dtsi` の `physical_layout_0` が参照 | 物理キー配置を大きく変更する、キー数を増減させる時 |
| [trackball_module/src/dir_key_processor.c](trackball_module/src/dir_key_processor.c) | 相対入力を方向別のキー／マクロに変換する input processor | `zephyr/module.yml` 経由でビルドに登録したカスタムモジュール | しきい値計算やキー発火ロジック自体を刷新する時 |

関連ファイルの位置づけ:
- `config/west.yml` が ZMK 本体と PMW3610 ドライバ、sensor_rotation、RGB LED ウィジェットを取得。
- `zephyr/module.yml` が `config` を board root として登録しつつ、`trackball_module` の C ソースと独自 Devicetree バインディングを追加する。

どの設定をどこで行うか迷った場合は、まず「よく調整するファイル」の表を確認し、配線や Shield 自体を作り直す時のみ「基本的に固定」の表を参照してください。
