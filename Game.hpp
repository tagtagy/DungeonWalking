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

	void InputMove(const Point _moveInput);

	// void Map(); // Removed
	void draw() const override;

private:
	void GenerateAndSetupNewMap(); // Added

	//タイル別イベント関数///////////////
	// 明確化のためのタイルタイプ定義（Game.cppの新定義と対応）
	
	void Wall(const Point _moveInput);// 0: ゲーム壁（通行不可、描画不可）
	void Floor(const Point _moveInput);// 1: ゲーム床（白） 2: プレイヤー開始位置（緑）5: デバッグエリア（マゼンタ）
	void Enemy(const Point _moveInput);// 3: 敵（攻撃意図のための通行可、移動不可。敵は別エンティティ）
	void Gool(const Point _moveInput);// 4: ゴール（通行可能、黄色で描画）
	
	/////////////////////////////////////
	
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
	//////////////////////////////////////

	BasePlayer* Player = nullptr;

	Array<BaseEnemy*> Enemys;

	Camera* camera = nullptr;

	// 全マップ表示の切り替え
	bool showFullMap = false;

	// 攻撃効果とタイマー
	Effect m_hitEffects;
	Optional<s3d::Vec2> m_cameraShakeOffset;
	Timer m_cameraShakeTimer{ 0.2s, s3d::StartImmediately::No };
	Optional<s3d::Vec2> m_playerLungeDirection; // バンプアニメーションに使用する
	Timer m_playerLungeTimer{ 0.2s, s3d::StartImmediately::No };   // バンプ調整済み（ランジ時は0.15秒）

	// 連続的な動き
	Timer m_initialMoveDelayTimer{ 0.4s, s3d::StartImmediately::No };
	Timer m_moveRepeatTimer{ 0.12s, s3d::StartImmediately::No };
	Optional<Point> m_heldMoveDirection;
	bool m_isWaitingForInitialRepeat = false;
	bool m_isAttackIntent = false; // 初期押下時の攻撃制御用に追加

	// プレイヤーのスライドアニメーション
	Optional<s3d::Vec2> m_playerSlideAnimDirection;
	Timer m_playerSlideAnimTimer{ 0.12s, s3d::StartImmediately::No }; // スライドの持続時間

public: // 現時点ではGame.cppでアクセス可能に公開されています。Gameクラスがレンダリング定数を所有する場合、リファクタリングが可能です。
	static constexpr int FullMapTileRenderSize = 8;
};
