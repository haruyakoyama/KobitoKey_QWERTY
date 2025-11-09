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

### 編集ガイド

| ファイル | 役割 | 読み込み順・依存関係 | 変更したくなるタイミング |
| --- | --- | --- | --- |
| [Kconfig.shield](config/boards/shields/KobitoKey/Kconfig.shield) | `SHIELD_KOBITOKEY_LEFT` と `SHIELD_KOBITOKEY_RIGHT` を `west build` に伝える | `west build -S` で Shield 列挙 → `shields_list_contains` マクロが評価 | 新しいハーフ名を増やす時や、Shield 名を変更したい時 |
| [Kconfig.defconfig](config/boards/shields/KobitoKey/Kconfig.defconfig) | Shield 有効時の既定 Kconfig を定義 | `Kconfig.shield` で真になった時のみ取り込まれる | スプリット機能や SPI、ポイントデバイス機能の既定値を変えたい時 |
| [KobitoKey.zmk.yml](config/boards/shields/KobitoKey/KobitoKey.zmk.yml) | Shield のメタデータ（`requires`, `features`）を宣言 | `west build` で Shield を解決する際に参照 | 対応ボード追加や URL を更新したい時 |
| [kobitokey.dtsi](config/boards/shields/KobitoKey/kobitokey.dtsi) | 4×10 行列スキャン、`matrix-transform`, split 入力定義、`chosen` の物理レイアウト指定を集約 | 両 overlay から `#include` され、`KobitoKey-layouts.dtsi` をさらに取り込む | 行列配線を変更する、split 入力 ID を変える、物理レイアウトを切り替える場合 |
| [KobitoKey-layouts.dtsi](config/boards/shields/KobitoKey/KobitoKey-layouts.dtsi) | `zmk,physical-layout` とキー座標を列挙 | `kobitokey.dtsi` 内の `physical_layout_0` が参照 | Studio や keymap-drawer のキー配置を修正したい時 |
| [KobitoKey_left.overlay](config/boards/shields/KobitoKey/KobitoKey_left.overlay) | 左手側のデバイスツリー。PMW3610（`tb_left`）、センサー回転、`zip_temp_layer` を定義し、RGB マウス操作を制御 | `kobitokey.dtsi` を読み込み、`tb_left_listener` と zip 系プロセッサを有効化 | 左側トラックボールの感度、オートマウスへの遷移時間、スクロール方向を変える時 |
| [KobitoKey_right.overlay](config/boards/shields/KobitoKey/KobitoKey_right.overlay) | 右手側のデバイスツリー。列オフセット、右トラックボール、split 送信を設定 | `kobitokey.dtsi` を読み込み、`tb_right_split` を通じて中央へ出力 | 右トラックボールの CPI、split 送信方式、列オフセットを調整する時 |
| [KobitoKey_left.conf](config/boards/shields/KobitoKey/KobitoKey_left.conf) | 左手（セントラル）の `prj.conf`。BLE 役割、PMW3610 設定、RGB LED ウィジェット、バッテリー報告を制御 | `west build -b seeeduino_xiao_ble -- -DSHIELD=KobitoKey_left` でマージ | セントラル側の BLE 機能、RGB 層色、PMW3610 パラメータを変更したい時 |
| [KobitoKey_right.conf](config/boards/shields/KobitoKey/KobitoKey_right.conf) | 右手（ペリフェラル）の `prj.conf`。PMW3610 と BLE ペリフェラル設定を担当 | `west build ... -DSHIELD=KobitoKey_right` で読み込まれる | 右手の CPI や BLE 動作、バッテリー報告方針を調整する時 |

上記以外の関連ファイル:
- `config/west.yml` で ZMK 本体と追加モジュールを取得。
- `config/KobitoKey.keymap` が論理レイヤーを定義し、`kobitokey.dtsi` の行列と結び付ける。
- `zephyr/module.yml` が `config` を board root に登録する。

この README を足掛かりに、「どこを編集すれば目的の挙動になるか」「どのファイルがどこに読み込まれるか」を即座に追えるようにしました。
