# Phase 0 後半: 設定・プロジェクト整備の記録

日付: 2026-11-07。対象コミット: `9d0d3a6` `ab738db` `d784e09` `91ef279` `b8bbd75`

## Build.cs 依存の整理(9d0d3a6)
- `Commander3D.Build.cs` の `Core/CoreUObject/Engine/InputCore/EnhancedInput` を Public から Private へ移した。
- `CommanderDomain` / `CommanderApplication` を Private に追加した。依存方向は Commander3D -> Application -> Domain。
- 理由: モジュールにはまだ公開ヘッダがない。Public にするのは、公開ヘッダがその型を露出するときだけ(`.claude/rules/module-layout.md`)。
- 確認: `Commander3DEditor Win64 Development` のビルド成功。

## レンダリング軽量化(ab738db)
- `DefaultEngine.ini`: `r.RayTracing=False`、`r.RayTracing.RayTracingProxies.ProjectEnabled=False`、`r.Substrate=False`。`r.Substrate.ProjectGBufferFormat` は削除。
- Lumen(GI/Reflection)と Virtual Shadow Maps は維持。
- 理由: SRPG にはレイトレーシングも Substrate も不要で、シェーダー負荷と GPU 要件を下げられる。
- 未確認: エディタ起動後の実際の挙動。初回起動でシェーダー再コンパイルが走る可能性がある。

## デフォルトマップ(d784e09, b8bbd75)
- テストマップ `Proj/Content/Map/PL_Test.umap` を追加(エディタで作成。`.umap` は LFS 追跡)。
- `GameDefaultMap` と `EditorStartupMap` を `/Game/Map/PL_Test.PL_Test` に設定。
- 未確認: エディタ/ゲーム起動でこのマップが開くこと。

## AndroidFileServer プラグイン無効化(91ef279)
- 背景: エディタが `DefaultEngine.ini` に `[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]`(`SecurityToken` 含む)を再生成した。0-1 で意図的に削除した項目。
- 対応: `Commander3D.uproject` の `Plugins` に `AndroidFileServer` を `Enabled: false` で追加し、ini のセクションを削除。
- 同コミットで `EngineAssociation` が GUID から `"5.8"` に変わった(エディタによる書き換え。マシン依存の GUID が消える)。
- 未確認: エディタ再起動後にセクションが再生成されないこと。`SecurityToken` は過去のコミットに残ったまま。

## 注意点
- エディタを開くと `.vsconfig` が書き換わることがある(SDK 26100 -> 22621、Clang 追加など)。0-3 で揃えた内容と食い違うため、差分が出たら `git checkout Proj/.vsconfig` で戻す。
- CI(0-10, `.github/workflows/ci.yml`)は作成したが、`.github` ごと消えており、コミットされていない。再作成するかはユーザー判断待ち。
- CI の構成案(domain-purity と core-tests の2ジョブ)はローカルで検証済み(MSVC のみ)。UE ビルドは CI に入れない。
