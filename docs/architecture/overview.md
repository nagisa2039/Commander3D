# アーキテクチャ概要

## 流れ

```
[Input/UI] → Command → [Application] → [Domain] → Events + 新しい状態
                                                      ↓
                        [Presentation: Event再生(アニメ/エフェクト/UI更新)]
```

## Bounded Context

| Context | 責務 | 実装時期 |
|---|---|---|
| Battle(中核) | Grid、移動、ターン、攻撃、AI、勝敗 | 今 |
| MasterData | クラス、武器、地形などの定義(読み取り専用) | 最小限で今 |
| Roster | 永続キャラクター、レベル、経験値、所持品 | 後で |
| Campaign | 章進行、マップ間引き継ぎ、セーブ | 後で |

当面はフォルダと namespace で分け、UEモジュールは分割しない。

## 主な型

- Aggregate: `Battle`(マップ、ユニット、ターン/フェーズを含む)
- Entity: `BattleUnit`
- Value Object: `GridCoord`、`UnitId`、`Hp`、`Stats`、`WeaponSpec`、`Damage` など
- Domain Service: `MovementRangeCalculator`、`AttackRangeCalculator`、`CombatResolver`、`TurnProgression`、`VictoryConditionEvaluator`、`IRandomSource`
- Application: `BattleSession` ファサード。Command(`MoveUnit`、`AttackUnit`、`WaitUnit`、`EndPhase` など)、Query、`Result<Events>`
- 敵AIは後で実装する。プレイヤーと同じ Command 経路を使う。

## UEモジュール(予定)

```
CommanderDomain(UE依存なし)
   ← CommanderApplication
      ← Commander3D(Infrastructure + Presentation + Adapters)
CommanderTests(Editor専用)
```

現状は `Commander3D` のみ存在する。

## テスト方針

- Domain / Application: CMake + CTest(UE不要、`Tests/Core`)
- Infrastructure: UE Automation Test

## 前提(未確認・要確認)

- 正方形グリッドで論理的には2D。高低差は地形属性。
- シングルプレイ専用、1マップ数十ユニットまで。
- 初代FE風(移動やり直しなし、武器耐久あり)。
