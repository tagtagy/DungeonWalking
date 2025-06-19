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

// 共有するデータ
struct GameData
{
	// レベル
	int32 Lv = 0;
	// ハイスコア

};

using App = SceneManager<State, GameData>;
