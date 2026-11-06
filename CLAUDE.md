# CLAUDE.md

Commander3D: Unreal Engine 5.8 / C++ で作る3Dターン制SRPG(初代Fire Emblem風のシンプルなシステム)。
キャラクター・ストーリー・世界観・マップ・アート・音楽・ゲームデータはすべてオリジナル。

- 識別子は英語、ドキュメントとチャットは日本語。
- UEプロジェクト本体は `Proj/`(`Proj/Commander3D.uproject`)。ビルド環境・コマンドは `.claude/CLAUDE.md` を参照。
- 設計の詳細は `docs/architecture/overview.md`、用語は `docs/domain/glossary.md`。

## 開発ルール

1. **計画を先に出す。** 複数ファイルにまたがる変更は、実装前に方針と変更対象を提示する。
2. **テスト先行。** ルール(Domain/Application)は、失敗するテストを先に書いてから実装する。
3. **Domain/ApplicationにUE依存を入れない。** `CoreMinimal.h`、`U/A/F` 型、`TArray`、`FString` などは禁止。C++標準のみ。`Tools/CheckDomainPurity` で検出する(作成予定)。
4. **ルールはDomainに置く。** Actor / Component / UI / Blueprint は表示だけを担当し、ルールとCommand発行を書かない。PresentationはDomainが返すEventを再生する。
5. **推測で抽象化しない。** 具体的な2つ目の用途が出るまで、インターフェースや汎用化を作らない。ダメージ式の事前インターフェース化、ECS、Event Sourcing、汎用ルールエンジン、DIコンテナは作らない。
6. **例外を使わない。** 失敗は `Result` で返す。
7. **`.uasset` / `.umap` を編集・生成しない。** バイナリアセットはエディタで扱う。
8. **マスターデータは JSON/CSV。** `.uasset` にしない(AIが編集できる形式にする)。
9. **テストを削除・無効化してエラーを消さない。** 失敗は原因を直す。
10. **確認できていないことを推測で埋めない。** 調べていない点は「未確認」と書く。
11. **コミット・pushは、ユーザーが明示的に依頼したときだけ行う。**
12. **コミットは Conventional Commits。** 1コミット = 1ルールまたは1縦スライス。リファクタ・機能追加・フォーマット・バイナリは別コミットにする。
