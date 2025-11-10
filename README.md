# KobitoKey_QWERTY

## プロジェクト概要
[小人キー](https://note.com/11_50iii/n/n75cff4d3502c#48b22323-074f-4017-9605-dbc51b4714ae)向けの ZMK ファームウェアです。作者の方のリポジトリをフォークし、自分用にカスタム中。
左右の Seeeduino XIAO BLE に PMW3610 トラックボールを載せ、左手側をセントラル、右手側をペリフェラルとして動かします。`config` ディレクトリを board root に指定し、独自シールドと周辺モジュール（PMW3610 ドライバ、sensor_rotation、RGB LED ウィジェット）を West で取得します。

ビルドは `build.yaml` のマトリクス（`KobitoKey_left rgbled_adapter`、`KobitoKey_right rgbled_adapter`、`settings_reset`）を `west build` で順に生成する想定です。キー配列は `config/KobitoKey.keymap`、レイアウト図は `config/KobitoKey.json` がソースになります。

## レイヤー配置

| Layer | 名称 | 主な機能 |
| --- | --- | --- |
| 0 | QWERTY | ベース入力 |
| 1 | num+symbol | 数字列、記号 |
| 2 | shift_symbol+move | シフトを押す記号、矢印、ウィンドウ操作等 |
| 3 | Auto Mouse | 自動遷移するマウスレイヤー |
| 4 | Config | 設定レイヤー |

## 主要ディレクトリ
- `build.yaml` ― `west build -b seeeduino_xiao_ble -s app -d build` 用のビルドマトリクス。左（`KobitoKey_left rgbled_adapter`）、右（`KobitoKey_right rgbled_adapter`）、`settings_reset` を一括ビルド。
- `zephyr/module.yml` ― `board_root: config` を宣言し、シールド定義をビルドに含める。

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
| [KobitoKey_left.overlay](config/boards/shields/KobitoKey/KobitoKey_left.overlay) | 左手側トラックボール、センサー回転、`zip_temp_layer` を制御 | `kobitokey.dtsi` を include し、`tb_left_listener` や zip 系プロセッサを有効化 | 左トラックボールの CPI、スクロール方向、オートマウス遷移条件を調整したい時 |
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

関連ファイルの位置づけ:
- `config/west.yml` が ZMK 本体と PMW3610 ドライバ、sensor_rotation、RGB LED ウィジェットを取得。
- `zephyr/module.yml` が `config` を board root として登録する。

どの設定をどこで行うか迷った場合は、まず「よく調整するファイル」の表を確認し、配線や Shield 自体を作り直す時のみ「基本的に固定」の表を参照してください。
