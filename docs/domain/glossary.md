# 用語集

コード上の識別子(英語)と日本語の対応。用語を追加・変更したらここを更新する。

| 識別子 | 日本語 | 説明 |
|---|---|---|
| `Battle` | 戦闘(マップ戦) | 1マップ分の状態。Aggregate Root |
| `BattleUnit` | 戦闘ユニット | マップ上のキャラクター。Entity |
| `GridCoord` | グリッド座標 | マス目の位置。Value Object |
| `UnitId` | ユニットID | `BattleUnit` の識別子 |
| `Faction` | 陣営 | `Player` / `Enemy`。敵陣営ユニットは移動を阻む |
| `MovementRange` | 移動範囲 | ユニットが到達できるマスと最小コスト・経路。通過できても止まれないマスがある |
| `UnitMoved` | ユニット移動イベント | 移動経路(開始〜終了)を持つ Event |
| `Phase` | フェーズ | 自軍・敵軍など、陣営ごとの行動順の区間 |
| `Command` | コマンド | プレイヤー/AIの意図(`MoveUnit` など) |
| `Event` | イベント | Domainが返す、起きた事実。Presentationが再生する |

(以降、設計の進行に合わせて追記する)
