#pragma once
#include "EnemyDataBase.hpp"

// 敵の行動状態を定義
enum class EnemyState {
	IDLE,    // 待機中
	PATROL,  // 巡回中
	CHASE,   // プレイヤーを追跡中
	ATTACK,  // 攻撃中
	RETREAT  // 退避中（巡回地点へ戻る）
};

// 経路探索用ノード構造体
struct Node {
	Point point;  // ノードの座標
	int g;        // 開始点からのコスト
	int h;        // ゴールまでの推定コスト
	int f;        // 総コスト (f = g + h)

	bool operator>(const Node& other) const { return f > other.f; }
};

class BaseEnemy {
public:
	BaseEnemy(Point _pos, int _ID);  // 敵の初期位置とデータIDで初期化

	int Move(Point _Player, Grid<int32>& mapData);  // 毎フレームの移動処理

	// ステータス取得
	StertsBase GetEnemySterts() { return Status; }
	Point GetEnemyPos() { return Enemy; }
	bool GetDeath() { return NowHP <= 0; }

	int Attack() { return Status.atc; }
	void Damage(int _damage) { NowHP -= _damage; }

	// 描画処理
	void draw(int _PieceSize, int _WallThickness, Point _camera) const;

private:
	void Chase(Point _Player, Grid<int32>& mapData);    // 追跡処理
	void Patrol(Grid<int32>& mapData);                  // 巡回処理
	void Retreat(Grid<int32>& mapData);                 // 退避処理

	bool AStarSearch(Point start, Point goal, const Grid<int32> mapData);  // A*経路探索
	static int Heuristic(Point a, Point b);  // 推定コスト関数

private:
	EnemyDataBase* DataBase = nullptr;  // ステータス参照用

	Point Enemy = { 5, 5 };  // 現在位置

	int AttackRange = 1;   // 攻撃範囲
	int SearchRange = 4;   // 索敵範囲

	StertsBase Status;     // ステータスデータ
	int NowHP;             // 現在のHP

	EnemyState EnemyStateMachine = EnemyState::IDLE;  // 状態管理

	Array<Point> FinalRoute;  // 探索されたルート
	Array<Point> OpenList;    // A* オープンリスト
	Array<Point> ClosedList;  // A* クローズリスト

	Array<Point> PatrolRoute;    // 巡回ルート（複数地点）
	int PatrolIndex = 0;         // 現在の巡回ターゲットのインデックス
	int ChaseCount = 0;          // 追跡しているフレーム数
	const int MaxChaseCount = 5; // これを超えたら退避する
};
