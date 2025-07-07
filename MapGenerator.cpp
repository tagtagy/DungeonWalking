#include "stdafx.h"
#include "MapGenerator.hpp"
#include <queue> // For std::queue in BFS
#include <utility> // For std::pair

// Helper function to carve L-shaped paths
// Static because it doesn't depend on MapGenerator instance members
static void carvePath(Grid<int>& map, Point p1, Point p2) {
    if (RandomBool()) { // Horizontal then Vertical
        for (int x = Min(p1.x, p2.x); x <= Max(p1.x, p2.x); ++x) {
            if (InRange(x, 0, MapGenerator::MAP_SIZE - 1) && InRange(p1.y, 0, MapGenerator::MAP_SIZE - 1)) {
                map[p1.y][x] = 1;
            }
        }
        for (int y = Min(p1.y, p2.y); y <= Max(p1.y, p2.y); ++y) {
            if (InRange(p2.x, 0, MapGenerator::MAP_SIZE - 1) && InRange(y, 0, MapGenerator::MAP_SIZE - 1)) {
                map[y][p2.x] = 1;
            }
        }
    } else { // Vertical then Horizontal
        for (int y = Min(p1.y, p2.y); y <= Max(p1.y, p2.y); ++y) {
            if (InRange(p1.x, 0, MapGenerator::MAP_SIZE - 1) && InRange(y, 0, MapGenerator::MAP_SIZE - 1)) {
                map[y][p1.x] = 1;
            }
        }
        for (int x = Min(p1.x, p2.x); x <= Max(p1.x, p2.x); ++x) {
            if (InRange(x, 0, MapGenerator::MAP_SIZE - 1) && InRange(p2.y, 0, MapGenerator::MAP_SIZE - 1)) {
                map[p2.y][x] = 1;
            }
        }
    }
}

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
	startTile_generated.reset();
	goalTile_generated.reset();
	this->generatedRoomAreas.clear();

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

			// Add the generated room's rectangle to the list
			this->generatedRoomAreas.push_back(roomRect);

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

						Point c1, c2;

						if (dx == 1) { // Neighbor to the right
							c1 = Point(current.area.x + current.area.w - 1, Random(current.area.y, current.area.y + current.area.h - 1));
							c2 = Point(neighbor.area.x, Random(neighbor.area.y, neighbor.area.y + neighbor.area.h - 1));
						} else if (dx == -1) { // Neighbor to the left
							c1 = Point(current.area.x, Random(current.area.y, current.area.y + current.area.h - 1));
							c2 = Point(neighbor.area.x + neighbor.area.w - 1, Random(neighbor.area.y, neighbor.area.y + neighbor.area.h - 1));
						} else if (dy == 1) { // Neighbor below
							c1 = Point(Random(current.area.x, current.area.x + current.area.w - 1), current.area.y + current.area.h - 1);
							c2 = Point(Random(neighbor.area.x, neighbor.area.x + neighbor.area.w - 1), neighbor.area.y);
						} else { // dy == -1, Neighbor above
							c1 = Point(Random(current.area.x, current.area.x + current.area.w - 1), current.area.y);
							c2 = Point(Random(neighbor.area.x, neighbor.area.x + neighbor.area.w - 1), neighbor.area.y + neighbor.area.h - 1);
						}

						// Clamp points to map boundaries
						c1.x = Clamp(c1.x, 0, MAP_SIZE - 1);
						c1.y = Clamp(c1.y, 0, MAP_SIZE - 1);
						c2.x = Clamp(c2.x, 0, MAP_SIZE - 1);
						c2.y = Clamp(c2.y, 0, MAP_SIZE - 1);

						carvePath(map, c1, c2);
					}
				}
			}
		}
	}

	// --- BEGIN: Ensure all rooms are interconnected ---
	// 連結性解析中にアクティブな部屋の情報を格納するためのヘルパー構造体。
	struct RoomInfoInternal {
		const Room* roomPtr;    // 実際のRoomオブジェクトへのポインタ。
		Point miniMapPos;       // ミニマップグリッド上の元の位置（デバッグ/ログ用）。
		int id;                 // この部屋の一意の識別子（activeRooms配列のインデックス）。
	};

	// ミニマップに実際に配置されたすべての部屋を収集する。
	Array<RoomInfoInternal> activeRooms;
	// ミニマップ座標(y,x)をactiveRooms内の部屋のID（インデックス）に迅速にマッピングするためのグリッド。
	Grid<Optional<int>> roomMiniMapToActiveID(MINI_SIZE, MINI_SIZE);

	for (int y = 0; y < MINI_SIZE; ++y) {
		for (int x = 0; x < MINI_SIZE; ++x) {
			if (rooms[y][x].has_value()) {
				int currentId = static_cast<int>(activeRooms.size());
				activeRooms.push_back(RoomInfoInternal{ &rooms[y][x].value(), Point(x,y), currentId });
				roomMiniMapToActiveID[y][x] = currentId;
			}
		}
	}

	if (activeRooms.isEmpty()) {
		Console << U"Warning: No active rooms found to interconnect.";
	} else {
		// 初期の通路によって接続された部屋のグラフを表す隣接リストを構築する。
		Array<Array<int>> roomAdjList(activeRooms.size());
		// アクティブな部屋を反復処理して、最初の通路生成フェーズでどの部屋が接続されたかを確認する。
		for (int y = 0; y < MINI_SIZE; ++y) {
			for (int x = 0; x < MINI_SIZE; ++x) {
				if (!rooms[y][x].has_value()) continue; // ここに部屋がなければスキップ
				int currentRoomActiveId = roomMiniMapToActiveID[y][x].value();
				const Room& currentRoomData = rooms[y][x].value();

				// 接続の二重カウントを避け、自己ループを防ぐため、隣接チェックは右と下方向の隣人のみ行う。
				for (auto [dx, dy] : Array<Point>{ {1, 0}, {0, 1} }) {
					int nx = x + dx;
					int ny = y + dy;
					if (InRange(nx, 0, MINI_SIZE - 1) && InRange(ny, 0, MINI_SIZE - 1)) {
						if (rooms[ny][nx].has_value()) {
							const Room& neighborRoomData = rooms[ny][nx].value();
							// Check S-G rule again, as original loop did
							if (!((currentRoomData.type == 'S' && neighborRoomData.type == 'G') ||
								(currentRoomData.type == 'G' && neighborRoomData.type == 'S'))) {
								int neighborRoomActiveId = roomMiniMapToActiveID[ny][nx].value();
								roomAdjList[currentRoomActiveId].push_back(neighborRoomActiveId);
								roomAdjList[neighborRoomActiveId].push_back(currentRoomActiveId);
							}
						}
					}
				}
			}
		}

		// 隣接リストのクリーンアップ：ソートし、対称的なpush_backによって発生する可能性のある重複を削除する。
		for (auto& adj : roomAdjList) {
			adj.sort_and_unique();
		}

		// 部屋グラフ上でBFSを使用して、連結された部屋のコンポーネントを特定する。
		Array<int> roomComponent(activeRooms.size(), -1); // 各部屋のコンポーネントIDを格納する。
		int totalComponents = 0;                          // 個別のコンポーネントのカウンター。
		for (int i = 0; i < activeRooms.size(); ++i) {
			if (roomComponent[i] == -1) { // まだ部屋 'i' がコンポーネントに割り当てられていない場合
				totalComponents++;
				std::queue<int> component_q; // BFS用のキュー
				component_q.push(i);
				roomComponent[i] = totalComponents; // 新しいコンポーネントIDを割り当てる
				// このコンポーネント内のすべての部屋を見つけるためにBFSを実行する
				while (!component_q.empty()) {
					int u = component_q.front();
					component_q.pop();
					for (int v : roomAdjList[u]) {
						if (roomComponent[v] == -1) {
							roomComponent[v] = totalComponents;
							component_q.push(v);
						}
					}
				}
			}
		}

		// 部屋の連結されていないコンポーネントが複数存在する場合、それらを接続する。
		if (totalComponents > 1) {
			//Console << Format(U"複数の部屋コンポーネントが検出されました（{}）。接続を試みます。")(totalComponents);
			// 最初のアクティブな部屋のコンポーネントをメインコンポーネントとして指定する。
			// (または、簡単に見分けられるならスタート部屋のコンポーネント)
			int mainComponentID = roomComponent[0];

			// 見つかった他のすべてのコンポーネントIDを反復処理する。
			for (int currentCompID = 1; currentCompID <= totalComponents; ++currentCompID) {
				if (currentCompID == mainComponentID) continue; // メインコンポーネント自体ならスキップ

				double minDistance = Math::Inf;
				// activeRooms配列内の{メインコンポーネントのインデックス, 現在のコンポーネントのインデックス}を格納する
				s3d::Optional<std::pair<int, int>> closestPairActiveIndices;

				// 最も近いペアの部屋を見つける：一方はmainComponentIDから、もう一方はcurrentCompIDから。
				for (int i = 0; i < activeRooms.size(); ++i) {
					if (roomComponent[i] == mainComponentID) {
						for (int j = 0; j < activeRooms.size(); ++j) {
							if (roomComponent[j] == currentCompID) {
								Point center1 = activeRooms[i].roomPtr->area.center().asPoint();
								Point center2 = activeRooms[j].roomPtr->area.center().asPoint();
								double dist = center1.distanceFrom(center2);
								if (dist < minDistance) {
									minDistance = dist;
									closestPairActiveIndices.emplace(i, j);
								}
							}
						}
					}
				}

				// 最も近いペアが見つかった場合、それらを接続する。
				if (closestPairActiveIndices.has_value()) {
					int roomIdx1 = closestPairActiveIndices.value().first;
					int roomIdx2 = closestPairActiveIndices.value().second;

					Point p1 = activeRooms[roomIdx1].roomPtr->area.center().asPoint();
					Point p2 = activeRooms[roomIdx2].roomPtr->area.center().asPoint();

					// Clamp points before carving path, just in case room centers are at edge
					p1.x = Clamp(p1.x, 0, MAP_SIZE - 1); p1.y = Clamp(p1.y, 0, MAP_SIZE - 1);
					p2.x = Clamp(p2.x, 0, MAP_SIZE - 1); p2.y = Clamp(p2.y, 0, MAP_SIZE - 1);

					carvePath(map, p1, p2); // 実際のタイルマップに経路を掘ることでそれらを接続する。
					//Console << Format(U"Forcibly connecting room {} (comp {}) to room {} (comp {}).")(activeRooms[roomIdx1].miniMapPos, roomComponent[roomIdx1], activeRooms[roomIdx2].miniMapPos, roomComponent[roomIdx2]);
					// この簡略化されたアプローチでは、ここではroomComponent配列内のコンポーネントIDを明示的にマージしない。
					// 物理的な経路が存在することを確認するだけ。後のS-G BFSがこれらの新しい経路を使用する。
				}
			}
		}
	}
	// --- END: Ensure all rooms are interconnected ---

	// S-G Connectivity Check and Enforcement
	Optional<Room> sRoomOpt, gRoomOpt;
	for (int y = 0; y < MINI_SIZE; ++y) {
		for (int x = 0; x < MINI_SIZE; ++x) {
			if (rooms[y][x].has_value()) {
				if (rooms[y][x].value().type == 'S') {
					sRoomOpt = rooms[y][x].value();
				} else if (rooms[y][x].value().type == 'G') {
					gRoomOpt = rooms[y][x].value();
				}
			}
		}
	}

	if (!sRoomOpt.has_value() || !gRoomOpt.has_value()) {
		Console << U"Error: Start ('S') or Goal ('G') room not found in miniMap.";
		// Reset them again here just in case, though they should be reset at function start
		startTile_generated.reset();
		goalTile_generated.reset();
		return map; // Return map as is
	}

	// Store S and G tile locations
	// Using .tl() (top-left) as it's a defined point of the room's Rect.
	// Clamping to ensure they are within map boundaries.
	Point sPos = sRoomOpt.value().area.tl();
	sPos.x = Clamp(sPos.x, 0, MAP_SIZE - 1);
	sPos.y = Clamp(sPos.y, 0, MAP_SIZE - 1);
	startTile_generated = sPos;

	Point gPos = gRoomOpt.value().area.tl();
	gPos.x = Clamp(gPos.x, 0, MAP_SIZE - 1);
	gPos.y = Clamp(gPos.y, 0, MAP_SIZE - 1);
	goalTile_generated = gPos;

	const Room& sRoom = sRoomOpt.value();
	const Room& gRoom = gRoomOpt.value();
	bool s_g_connected = false;

	Grid<bool> visited(MAP_SIZE, MAP_SIZE, false);
	std::queue<Point> q;

	Optional<Point> bfsStartPointOpt;
	Point sRoomCenter = sRoom.area.center().asPoint();
	// Clamp sRoomCenter to be within map bounds before accessing map with it
	sRoomCenter.x = Clamp(sRoomCenter.x, 0, MAP_SIZE - 1);
	sRoomCenter.y = Clamp(sRoomCenter.y, 0, MAP_SIZE - 1);

	if (map[sRoomCenter.y][sRoomCenter.x] == 1) {
		bfsStartPointOpt = sRoomCenter;
	} else {
		// Iterate through all tiles in sRoom.area to find a passable tile
		for (int ry = sRoom.area.y; ry < sRoom.area.y + sRoom.area.h; ++ry) {
			for (int rx = sRoom.area.x; rx < sRoom.area.x + sRoom.area.w; ++rx) {
				Point currentTile(Clamp(rx, 0, MAP_SIZE - 1), Clamp(ry, 0, MAP_SIZE - 1));
				if (map[currentTile.y][currentTile.x] == 1) {
					bfsStartPointOpt = currentTile;
					goto found_passable_start_tile; // Exit nested loops
				}
			}
		}
	found_passable_start_tile:;
	}

	if (!bfsStartPointOpt.has_value()) {
		Console << U"Error: No passable tile found in Start ('S') room. Cannot perform BFS.";
		// Proceed to force connection as they are considered disconnected
	} else {
		Point startTile = bfsStartPointOpt.value();
		q.push(startTile);
		visited[startTile.y][startTile.x] = true;

		Point DIRS[] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} }; // N, S, W, E

		while (!q.empty()) {
			Point current = q.front();
			q.pop();

			if (gRoom.area.contains(current)) {
				s_g_connected = true;
				break;
			}

			for (const auto& dir : DIRS) {
				Point next = current + dir;
				if (InRange(next.x, 0, MAP_SIZE - 1) && InRange(next.y, 0, MAP_SIZE - 1) &&
					map[next.y][next.x] == 1 && !visited[next.y][next.x]) {
					visited[next.y][next.x] = true;
					q.push(next);
				}
			}
		}
	}

	if (!s_g_connected) {
		Console << U"S and G not connected. Forcing connection.";
		Point sCenter = sRoom.area.center().asPoint();
		Point gCenter = gRoom.area.center().asPoint();

		// Clamp points to map boundaries before carving path
		sCenter.x = Clamp(sCenter.x, 0, MAP_SIZE - 1);
		sCenter.y = Clamp(sCenter.y, 0, MAP_SIZE - 1);
		gCenter.x = Clamp(gCenter.x, 0, MAP_SIZE - 1);
		gCenter.y = Clamp(gCenter.y, 0, MAP_SIZE - 1);

		carvePath(map, sCenter, gCenter);
	}

	// --- BEGIN: Convert internal walls to floor ---
	for (int y = 0; y < MAP_SIZE; ++y) {
		for (int x = 0; x < MAP_SIZE; ++x) {
			// Check if the current tile is an "internal" tile (not on the absolute border)
			if (x > 0 && x < MAP_SIZE - 1 && y > 0 && y < MAP_SIZE - 1) {
				// If this internal tile is currently a wall (type 0 in MapGenerator's convention)
				if (map[y][x] == 0) {
					map[y][x] = 1; // Convert it to a floor tile (type 1)
				}
			}
			// Tiles on the border (x=0, y=0, x=MAP_SIZE-1, y=MAP_SIZE-1) that are walls (type 0)
			// will remain walls. If they were carved as floor (type 1), they remain floor.
		}
	}
	// --- END: Convert internal walls to floor ---

	return map;
}
