# pragma once
# include <Siv3D.hpp>

// シーンのステート
enum class State
{
	Title,
	Game, 
	Ranking,
};
//ステータス
struct StertsBase {
	String name;
	int HP;
	int atc;
	ColorF color;
};

// 明確化のためのタイルタイプ定義（Game.cppの新定義と対応）
// 0: ゲーム壁（通行不可、描画不可）
// 1: ゲーム床（通行可、ピースカラーで描画）
// 2: プレイヤー開始位置（通行可、緑色で描画）
// 3: 敵（攻撃意図のための通行可、移動不可。敵は別エンティティ）
// 4: ゴール（通行可能、黄色で描画）
// 5: デバッグルームエリア（通行可能、マゼンタで描画）

// 共有するデータ
struct GameData
{
	// レベル
	int32 Lv = 0;
	// ハイスコア

};

using App = SceneManager<State, GameData>;
