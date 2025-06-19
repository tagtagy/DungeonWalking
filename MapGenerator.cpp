#include "stdafx.h"
#include "MapGenerator.hpp"

// ミニマップ生成処理
Array<Array<char>> MapGenerator::generateMiniMap() {
	Array<Array<char>> miniMap(MINI_SIZE, Array<char>(MINI_SIZE, 'O')); // 初期はすべてO（空）
	int roomCount = Random(5, 15); // ランダムに5〜15個の部屋を作成

	// Rの部屋をランダムに配置
	for (int i = 0; i < roomCount; ++i) {
		while (true) {
			int x = Random(0, MINI_SIZE - 1);
			int y = Random(0, MINI_SIZE - 1);
			if (miniMap[y][x] == 'O') {
				miniMap[y][x] = 'R';
				break;
			}
		}
	}

	// R部屋の中からランダムで2つ選び、SとGにする
	Array<Point> roomPositions;
	for (int y = 0; y < MINI_SIZE; ++y)
		for (int x = 0; x < MINI_SIZE; ++x)
			if (miniMap[y][x] == 'R')
				roomPositions.push_back(Point{ x, y });

	roomPositions.shuffle();
	miniMap[roomPositions[0].y][roomPositions[0].x] = 'S';
	miniMap[roomPositions[1].y][roomPositions[1].x] = 'G';

	return miniMap;
}

// 実際のマップを生成する処理
Grid<int> MapGenerator::generateFullMap(const Array<Array<char>>& miniMap) {
	Grid<int> map(MAP_SIZE, MAP_SIZE, 0); // 初期状態はすべて通れないマス（0）
	Array<Array<Optional<Room>>> rooms(MINI_SIZE, Array<Optional<Room>>(MINI_SIZE));

	// 各ミニマップのマスを処理
	for (int y = 0; y < MINI_SIZE; ++y) {
		for (int x = 0; x < MINI_SIZE; ++x) {
			char cell = miniMap[y][x];
			if (cell == 'O') continue; // 空マスはスキップ

			// 隣接する部屋に応じて余白（マージン）を設定
			int marginL = 0, marginR = 0, marginT = 0, marginB = 0;
			if (x > 0 && miniMap[y][x - 1] != 'O') marginL = 4;
			if (x < MINI_SIZE - 1 && miniMap[y][x + 1] != 'O') marginR = 4;
			if (y > 0 && miniMap[y - 1][x] != 'O') marginT = 4;
			if (y < MINI_SIZE - 1 && miniMap[y + 1][x] != 'O') marginB = 4;

			// 余白を考慮した配置可能な領域
			int startX = x * ROOM_UNIT + marginL;
			int startY = y * ROOM_UNIT + marginT;
			int width = ROOM_UNIT - marginL - marginR;
			int height = ROOM_UNIT - marginT - marginB;

			// 部屋のサイズと位置をランダムで決定（最小辺3）
			int roomW = Random(3, width);
			int roomH = Random(3, height);
			int offsetX = Random(0, width - roomW);
			int offsetY = Random(0, height - roomH);

			Rect roomRect(startX + offsetX, startY + offsetY, roomW, roomH);

			// マップ上に部屋を描画（1: 通れる）
			for (int yy = roomRect.y; yy < roomRect.y + roomRect.h; ++yy)
				for (int xx = roomRect.x; xx < roomRect.x + roomRect.w; ++xx)
					map[yy][xx] = 1;

			rooms[y][x] = Room{ roomRect, cell };
		}
	}

	// 通路の生成処理
	for (int y = 0; y < MINI_SIZE; ++y) {
		for (int x = 0; x < MINI_SIZE; ++x) {
			if (!rooms[y][x].has_value()) continue;
			const Room& current = rooms[y][x].value();

			// 上下左右の部屋と接続（SとGは接続しない）
			for (auto [dx, dy] : Array<Point>{ {0, -1}, {0, 1}, {-1, 0}, {1, 0} }) {
				int nx = x + dx, ny = y + dy;
				if (InRange(nx, 0, MINI_SIZE - 1) && InRange(ny, 0, MINI_SIZE - 1)) {
					if (rooms[ny][nx].has_value()) {
						const Room& neighbor = rooms[ny][nx].value();

						if ((current.type == 'S' && neighbor.type == 'G') ||
							(current.type == 'G' && neighbor.type == 'S')) {
							continue; // SとGは繋げない
						}

						// 部屋の中心を取得
						Point p1 = Point((int)current.area.center().x, (int)current.area.center().y);
						Point p2 = Point((int)neighbor.area.center().x, (int)neighbor.area.center().y);

						// 垂直→水平の順に通路を掘る
						for (int y = Min(p1.y, p2.y); y <= Max(p1.y, p2.y); ++y)
							map[y][p1.x] = 1;

						for (int x = Min(p1.x, p2.x); x <= Max(p1.x, p2.x); ++x)
							map[p2.y][x] = 1;
					}
				}
			}
		}
	}

	return map;
}
