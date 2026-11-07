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
| `UnitWaited` | 待機イベント | ユニットが行動を終えた事実 |
| `PhaseChanged` | フェーズ切替イベント | 新しいフェーズの陣営とターン番号。敵→自軍でターンが進む |
| `WeaponSpec` | 武器仕様 | 威力・命中・射程(最小/最大)。最大射程0は武器なし |
| `IRandomSource` | 乱数源 | 命中判定用 0〜99 の乱数。テストでは固定値を差し込む |
| `Strike` | 一撃 | 攻撃または反撃の1回分(命中有無・ダメージ・被弾後HP) |
| `AttackResolved` | 攻撃結果イベント | 攻撃と反撃の `Strike` 列と撃破されたユニット |
| `BattleOutcome` | 戦闘結果 | `Ongoing` / `PlayerWon` / `PlayerLost` |
| `VictoryConditions` | 勝利条件設定 | 殲滅(既定)・ボス撃破・ターン制限(超過で敗北)。自軍全滅は常に敗北 |
| `BattleSetup` | 戦闘セットアップ | マップ・ユニット配置・勝利条件。`StartBattle` が検証して `Battle` にする |
| `BattleSession` | 戦闘セッション | Command を受けて Event を返す唯一の窓口 |
| `StdRandomSource` | 標準乱数源 | シード指定可能な本番用 `IRandomSource` |
| `DecideNext` | 敵AIの次手決定 | 敵フェーズで次に出す Command を返す。プレイヤーと同じ Command 経路を使う |
| `RunEnemyPhase` | 敵フェーズ実行 | `DecideNext` を繰り返し、フェーズ終了まで Event 列を返す |
| `Phase` | フェーズ | 自軍・敵軍など、陣営ごとの行動順の区間 |
| `Command` | コマンド | プレイヤー/AIの意図(`MoveUnit` など) |
| `Event` | イベント | Domainが返す、起きた事実。Presentationが再生する |

(以降、設計の進行に合わせて追記する)
