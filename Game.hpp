# pragma once﻿
# include "Common.hpp"
#include "Camera.hpp"
# include"BaseEnemy.hpp"
# include"BasePlayer.hpp"
#include "MapGenerator.hpp"
#include"Particle.hpp"

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
	~Game(); // Declare destructor

	void update() override;

	void InputMove(int _x, int _y);

	// void Map(); // Removed
	void draw() const override;

private:
	void GenerateAndSetupNewMap(); // Added

	//マップ系
	Grid<int32> currentMapGrid; // 動的に生成されたマップを保存します。
	// 壁の厚さ
	int WallThickness = 5;
	//ピースのサイズ
	int PieceSize = 30;
	//現在のステージ
	static short s_currentStage; // Changed to static

	MapGenerator generator;


	//ピースカラー
	ColorF PieceColor = Palette::White;
	//////////////////////////////////
	//ウィンドウ
	const Rect MessageWindow{ 0,450,800,150 };
	const Rect MiniMessageWindow{ 700, 0, 100, 80 };
	const Rect CharaWindow{ 650, 80, 150, 370 };

	RectF getPaddle(int _x, int _y)const;

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

	Array<BaseEnemy*> Enemys;

	Camera* camera = nullptr;

	// Full map display toggle
	bool showFullMap = false;

	// Attack Effects & Timers
	s3d::Effect m_hitEffects;
	s3d::Optional<s3d::Vec2> m_cameraShakeOffset;
	s3d::Timer m_cameraShakeTimer{ 0.2s, s3d::StartImmediately::No };
	s3d::Optional<s3d::Vec2> m_playerLungeDirection; // To be used for Bump animation
	s3d::Timer m_playerLungeTimer{ 0.2s, s3d::StartImmediately::No };   // Adjusted for Bump (was 0.15s for Lunge)

	// Continuous Movement
	s3d::Timer m_initialMoveDelayTimer{ 0.4s, s3d::StartImmediately::No };
	s3d::Timer m_moveRepeatTimer{ 0.12s, s3d::StartImmediately::No };
	s3d::Optional<Point> m_heldMoveDirection;
	bool m_isWaitingForInitialRepeat = false;
	bool m_isAttackIntent = false; // Added for controlling attack on initial press

	// Player Slide Animation
	s3d::Optional<s3d::Vec2> m_playerSlideAnimDirection;
	s3d::Timer m_playerSlideAnimTimer{ 0.12s, s3d::StartImmediately::No }; // Duration of slide

public: // Made public for access in Game.cpp for now, can be refactored if Game class owns render consts
	static constexpr int FullMapTileRenderSize = 8;
};
