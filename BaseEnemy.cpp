#include "stdafx.h"
#include "BaseEnemy.hpp"

// コンストラクタ
BaseEnemy::BaseEnemy(Point _pos, int _ID) {
	DataBase = new EnemyDataBase;
	Status = DataBase->EnemyData[_ID];
	Enemy = _pos;
	NowHP = Status.HP;

	// 巡回ポイント（例）：正方形を描くように巡回
	PatrolRoute = { _pos, _pos + Point{2,0}, _pos + Point{2,2}, _pos + Point{0,2} };
}

int BaseEnemy::Heuristic(Point a, Point b) {
	double dx = a.x - b.x;
	double dy = a.y - b.y;
	return static_cast<int>(std::sqrt(dx * dx + dy * dy));
}

// 移動処理
int BaseEnemy::Move(Point _Player, Grid<int32>& mapData) {
	int dx = _Player.x - Enemy.x;
	int dy = _Player.y - Enemy.y;
	int distance = std::sqrt(dx * dx + dy * dy);

	// 攻撃範囲にプレイヤーがいる → 攻撃状態
	if (distance <= AttackRange) {
		EnemyStateMachine = EnemyState::ATTACK;
		return Attack();
	}

	// 索敵範囲内 → 追跡開始
	if (distance <= SearchRange) {
		EnemyStateMachine = EnemyState::CHASE;
		++ChaseCount;

		// 一定時間追跡したら退避状態に移行
		if (ChaseCount > MaxChaseCount) {
			EnemyStateMachine = EnemyState::RETREAT;
		}
		else {
			Chase(_Player, mapData);
		}
		return 0;
	}
	else {
		// プレイヤーが見えなくなったら追跡カウンタをリセット
		ChaseCount = 0;
	}

	// 状態に応じた行動
	switch (EnemyStateMachine) {
	case EnemyState::RETREAT:
		Retreat(mapData);
		break;
	default:
		EnemyStateMachine = EnemyState::PATROL;
		Patrol(mapData);
		break;
	}

	return 0;
}


// プレイヤーを追いかける
void BaseEnemy::Chase(Point _Player, Grid<int32>& mapData) {
	FinalRoute.clear();
	OpenList.clear();
	ClosedList.clear();

	// 経路探索
	if (AStarSearch(Enemy, _Player, mapData)) {
		// 経路が見つかった場合、次のマスに進む
		if (FinalRoute.size() > 1) {
			mapData[Enemy.y][Enemy.x] = 0;
			Enemy = FinalRoute[1];
			mapData[Enemy.y][Enemy.x] = 3;
			
		}
	}
}

// ヒューリスティック関数（ここではマンハッタン距離を使用）
int Heuristic(Point a, Point b) {
	return Abs(a.x - b.x) + Abs(a.y - b.y);
}

// A*アルゴリズムによる経路探索
bool BaseEnemy::AStarSearch(Point start, Point goal, const Grid<int32> mapData) {
	// Open List（探索予定リスト）とClosed List（探索済みリスト）
	std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList; // 優先度付きキュー
	std::unordered_set<Point> closedList;  // 探索済みリスト
	std::unordered_map<Point, Point> cameFrom;  // 親ノードを格納するマップ

	// 初期ノードの設定
	Node startNode = { start, 0, Heuristic(start, goal), 0 + Heuristic(start, goal) };
	openList.push(startNode);

	while (!openList.empty()) {
		// 1. 最も優先度の高いノードを取り出す
		Node current = openList.top();
		openList.pop();

		// 2. ゴールに到達したら経路探索終了
		if (current.point == goal) {
			// ゴールに到達したので、経路をバックトラッキングしてFinalRouteに追加
			Point currentPoint = goal;
			FinalRoute.clear();  // 最後の経路をクリア
			while (currentPoint != start) {
				FinalRoute.push_back(currentPoint);
				currentPoint = cameFrom[currentPoint];  // 親ノードに辿る
			}
			FinalRoute.push_back(start);  // スタートノードを追加
			std::reverse(FinalRoute.begin(), FinalRoute.end());  // 最後に逆順にする
			return true;
		}

		// 3. 現在のノードを探索済みリストに追加
		closedList.insert(current.point);
		ClosedList << current.point;

		// 4. 隣接ノードを評価して次のノードを探索
		for (int dx = -1; dx <= 1; dx++) {
			for (int dy = -1; dy <= 1; dy++) {
				// 垂直または水平方向に隣接
				if (dx == 0 && dy == 0) continue;

				Point neighbor = { current.point.x + dx, current.point.y + dy };

				// マップ外や障害物がある場合はスキップ
				if (neighbor.x < 0 || neighbor.y < 0 || neighbor.x >= mapData.width() || neighbor.y >= mapData.height() || mapData[neighbor.y][neighbor.x] == 1) {
					continue;
				}

				// 既に探索したノード（closedList）には進まない
				if (closedList.find(neighbor) != closedList.end()) continue;

				// 新しいノードを評価し、オープンリストに追加
				int g = current.g + 1;  // 移動コストは1と仮定
				int h = Heuristic(neighbor, goal);
				int f = g + h;

				Node neighborNode = { neighbor, g, h, f };
				openList.push(neighborNode);
				OpenList << neighborNode.point;
				// 親ノードを保存
				cameFrom[neighbor] = current.point;  // このノードの親を記録
			}
		}
	}

	// ゴールに到達できなかった場合
	return false;
}


// 敵の描画（探索状況の可視化付き）
void BaseEnemy::draw(int _PieceSize, int _WallThickness, Point _camera) const {
	// OpenList（水色）
	for (const auto& p : OpenList) {
		Circle{ p.x * _PieceSize + (_PieceSize / 2) + (p.x + 1) * _WallThickness - _camera.x,
			  p.y * _PieceSize + (_PieceSize / 2) + (p.y + 1) * _WallThickness - _camera.y,
			  5 }.draw(Palette::Skyblue);
	}

	// ClosedList（灰色）
	for (const auto& p : ClosedList) {
		Circle{ p.x * _PieceSize + (_PieceSize / 2) + (p.x + 1) * _WallThickness - _camera.x,
			  p.y * _PieceSize + (_PieceSize / 2) + (p.y + 1) * _WallThickness - _camera.y,
			  5 }.draw(Palette::Gray);
	}

	// FinalRoute（赤線）
	Array<Point> LineRoute;
	for (const auto& fr : FinalRoute) {
		LineRoute << Point{ (fr.x * _PieceSize) + (_PieceSize / 2) + ((fr.x + 1) * _WallThickness) - _camera.x,
							(fr.y * _PieceSize) + (_PieceSize / 2) + ((fr.y + 1) * _WallThickness) - _camera.y };
	}

	LineString{ LineRoute }.draw(5, Palette::Red);
}

void BaseEnemy::Patrol(Grid<int32>& mapData) {
	if (PatrolRoute.isEmpty()) return;

	Point target = PatrolRoute[PatrolIndex];
	if (AStarSearch(Enemy, target, mapData) && FinalRoute.size() > 1) {
		mapData[Enemy.y][Enemy.x] = 0;
		Enemy = FinalRoute[1];
		mapData[Enemy.y][Enemy.x] = 3;

		// 目的地に到達したら次の巡回ポイントへ
		if (Enemy == target) {
			PatrolIndex = (PatrolIndex + 1) % PatrolRoute.size();
		}
	}
}

// 退避処理：巡回ルートに戻る
void BaseEnemy::Retreat(Grid<int32>& mapData) {
	if (PatrolRoute.isEmpty()) return;

	Point target = PatrolRoute[PatrolIndex]; // 現在の巡回ポイントへ戻る
	if (AStarSearch(Enemy, target, mapData) && FinalRoute.size() > 1) {
		mapData[Enemy.y][Enemy.x] = 0;
		Enemy = FinalRoute[1];
		mapData[Enemy.y][Enemy.x] = 3;

		if (Enemy == target) {
			EnemyStateMachine = EnemyState::PATROL;
			ChaseCount = 0;
		}
	}
}
