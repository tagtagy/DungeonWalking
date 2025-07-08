# pragma once﻿
// 共通ヘッダーファイル (Siv3Dの基本的な機能を含む)
# include "Common.hpp"
// カメラ機能のヘッダーファイル
#include "Camera.hpp"
// 敵キャラクターの基底クラスのヘッダーファイル
# include"BaseEnemy.hpp"
// プレイヤーキャラクターの基底クラスのヘッダーファイル
# include"BasePlayer.hpp"
// マップ生成機能のヘッダーファイル
#include "MapGenerator.hpp"
// パーティクルエフェクト機能のヘッダーファイル
#include"Particle.hpp"

// 移動モードを表す列挙型 (現在は未使用の可能性あり)
enum class MoveMode
{
	Walk, // 歩き
	Run,  // 走り
	Hit,  // 攻撃時など
};

// ゲームシーンを管理するクラス (Siv3Dのシーン管理機能 App::Scene を継承)
class Game : public App::Scene
{
public:
	// コンストラクタ (シーン初期化時に呼ばれる)
	Game(const InitData& init);
	// デストラクタ (シーン終了時に呼ばれる)
	~Game();

	// 毎フレームの更新処理 (Siv3Dのシーン管理から呼ばれる)
	void update() override;

	// プレイヤーの移動入力を処理するメソッド
	// _x: X方向の移動量 (-1, 0, 1)
	// _y: Y方向の移動量 (-1, 0, 1)
	void InputMove(int _x, int _y);

	// void Map(); // 古いマップ処理メソッド (削除済み)

	// 毎フレームの描画処理 (Siv3Dのシーン管理から呼ばれる)
	void draw() const override;

private:
	// 新しいマップを生成し、関連する設定を行うメソッド
	void GenerateAndSetupNewMap();

	// --- マップ関連のメンバ変数 ---
	// 現在のマップの状態を保持するグリッド (タイル種別を格納)
	Grid<int32> currentMapGrid;
	// マップの壁の厚さ (ピクセル単位)
	int WallThickness = 5;
	// マップの1マスのサイズ (ピクセル単位)
	int PieceSize = 30;
	// 現在のステージ番号 (静的メンバ変数として全ゲームインスタンスで共有される可能性あり)
	static short s_currentStage;

	// マップ生成を行うための MapGenerator オブジェクト
	MapGenerator generator;

	// マップ上のピース（床など）のデフォルト色
	ColorF PieceColor = Palette::White;

	// --- UIウィンドウ関連のメンバ変数 ---
	// メッセージ表示用ウィンドウの矩形領域
	const Rect MessageWindow{ 0,450,800,150 };
	// ミニメッセージ表示用ウィンドウの矩形領域
	const Rect MiniMessageWindow{ 700, 0, 100, 80 };
	// キャラクター表示用ウィンドウの矩形領域
	const Rect CharaWindow{ 650, 80, 150, 370 };

	// 指定されたグリッド座標 (_x, _y) に対応する描画用の矩形領域を取得するメソッド
	RectF getPaddle(int _x, int _y)const;

	// --- キャラクターテクスチャ関連のメンバ変数 ---
	// UI表示用のキャラクターテクスチャ配列 (8種類の表情差分)
	const Texture texture[8]{
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_通常.png")).thresholded_Otsu()}, // 通常顔
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_笑顔.png")).thresholded_Otsu()}, // 笑顔
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_怒り.png")).thresholded_Otsu()}, // 怒り顔
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_呆れ.png")).thresholded_Otsu()}, // 呆れ顔
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_苦笑い.png")).thresholded_Otsu()}, // 苦笑い
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_驚き.png")).thresholded_Otsu()}, // 驚き顔
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_口開き.png")).thresholded_Otsu()}, // 口開き
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_目閉じ.png")).thresholded_Otsu()}  // 目閉じ
	};

	// --- ゲームオブジェクト関連のメンバ変数 ---
	// プレイヤーオブジェクトへのポインタ
	BasePlayer* Player = nullptr;
	// 敵オブジェクトの配列 (ポインタを格納)
	Array<BaseEnemy*> Enemys;
	// カメラオブジェクトへのポインタ
	Camera* camera = nullptr;

	// --- ゲーム状態・フラグ関連のメンバ変数 ---
	// フルマップ表示の切り替えフラグ
	bool showFullMap = false;

	// --- エフェクト・タイマー関連のメンバ変数 ---
	// ヒットエフェクト管理オブジェクト
	s3d::Effect m_hitEffects;
	// カメラシェイク時のオフセット量 (Optional: 値が存在しない場合もある)
	s3d::Optional<s3d::Vec2> m_cameraShakeOffset;
	// カメラシェイクのタイマー (0.2秒間、自動開始なし)
	s3d::Timer m_cameraShakeTimer{ 0.2s, s3d::StartImmediately::No };
	// プレイヤーのランジ（突進）方向 (アニメーション用)
	s3d::Optional<s3d::Vec2> m_playerLungeDirection;
	// プレイヤーのランジ（突進）アニメーション用タイマー (0.2秒間、自動開始なし)
	s3d::Timer m_playerLungeTimer{ 0.2s, s3d::StartImmediately::No };

	// --- 連続移動制御関連のメンバ変数 ---
	// 連続移動開始までの遅延タイマー (0.4秒間、自動開始なし)
	s3d::Timer m_initialMoveDelayTimer{ 0.4s, s3d::StartImmediately::No };
	// 連続移動中のリピート間隔タイマー (0.12秒間、自動開始なし)
	s3d::Timer m_moveRepeatTimer{ 0.12s, s3d::StartImmediately::No };
	// 現在ホールドされている移動方向 (Optional: ホールドされていない場合は値なし)
	s3d::Optional<Point> m_heldMoveDirection;
	// 初回リピート待ち状態かどうかのフラグ
	bool m_isWaitingForInitialRepeat = false;
	// 攻撃の意図があるかどうかのフラグ (キー初回押し込み時のみtrueになる想定)
	bool m_isAttackIntent = false;

	// --- プレイヤースライドアニメーション関連のメンバ変数 (現在は未使用の可能性あり) ---
	// プレイヤースライドアニメーションの方向
	s3d::Optional<s3d::Vec2> m_playerSlideAnimDirection;
	// プレイヤースライドアニメーションのタイマー (0.12秒間、自動開始なし)
	s3d::Timer m_playerSlideAnimTimer{ 0.12s, s3d::StartImmediately::No };

public:
	// フルマップ表示時の1タイルの描画サイズ (Game.cppからアクセスされるためpublicになっているが、リファクタリングの余地あり)
	static constexpr int FullMapTileRenderSize = 8;
};
