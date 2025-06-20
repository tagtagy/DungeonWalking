# pragma once
# include "Common.hpp"
#include "Camera.hpp"
# include"BaseEnemy.hpp"
# include"BasePlayer.hpp"
#include "MapGenerator.hpp"

enum class MoveMode
{
	Walk,
	Run,
	Hit,
};

// ゲームシーン
class Game : public App::Scene
{
public:

	Game(const InitData& init);

	void update() override;

	void InputMove(int _x, int _y);

	// void Map(); // Removed
	void draw() const override;

private:
	void GenerateAndSetupNewMap(); // Added

	//マップ系
	Grid<int32> currentMapGrid; // Will store the dynamically generated map
	// 壁の厚さ
	int WallThickness = 5;
	//ピースのサイズ
	int PieceSize = 30;
	//現在のステージ
	short NowStage = 0;

	MapGenerator generator;


	//ピースカラー
	ColorF PieceColor = Palette::White;
	//////////////////////////////////
	//ウィンドウ
	const Rect MessageWindow{ 0,450,800,150 };
	const Rect MiniMessageWindow{ 700, 0, 100, 80 };
	const Rect CharaWindow{ 650, 80, 150, 370 };

	Rect getPaddle(int _x, int _y)const;

	//////////////////////////////////
	//キャラ
	const Texture texture[8]{
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_通常.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_笑顔.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_怒り.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_呆れ.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_苦笑い.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_驚き.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_口開き.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_目閉じ.png")).thresholded_Otsu()}
	};


	BasePlayer* Player = nullptr;
	bool IsMove = false;
	MoveMode Move;

	Array<BaseEnemy*> Enemys;
	
	Camera* camera = nullptr;

	// Full map display toggle
	bool showFullMap = false;
public: // Made public for access in Game.cpp for now, can be refactored if Game class owns render consts
	static constexpr int FullMapTileRenderSize = 8;
};
