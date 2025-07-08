
#include "MapGenerator.hpp"
#include <queue>   // 幅優先探索(BFS)のための std::queue をインクルード
#include <utility> // std::pair をインクルード (現在は直接使用されていないが、将来的に役立つ可能性あり)

// L字型の経路を掘るヘルパー関数 (1タイル幅)
// この関数は MapGenerator のインスタンスメンバーに依存しないため static (静的) 関数となっている。
// 引数:
//   map: 変更対象のマップグリッド (参照渡し)
//   p1: 経路の開始点
//   p2: 経路の終了点
static void carvePath(Grid<int>&map, Point p1, Point p2) {
	Point current = p1; // 現在の掘削位置

	// p1.y の高さで、p1.x から p2.x まで水平に掘る
	while (current.x != p2.x) {
		// 座標がマップ範囲内か確認
		if (InRange(current.x, 0, MapGenerator::MAP_SIZE - 1) && InRange(current.y, 0, MapGenerator::MAP_SIZE - 1)) {
			map[current.y][current.x] = 1; // 床/通路 (タイルID:1)としてマーク
		}
		current.x += (p2.x > current.x) ? 1 : -1; // p2.x に向かって1マス進む
	}
	// 水平移動と垂直移動の接続点 (p2.x, p1.y) も確実に掘る
	if (InRange(current.x, 0, MapGenerator::MAP_SIZE - 1) && InRange(current.y, 0, MapGenerator::MAP_SIZE - 1)) {
		map[current.y][current.x] = 1;
	}

	// p2.x の位置で、p1.y から p2.y まで垂直に掘る
	while (current.y != p2.y) {
		// 座標がマップ範囲内か確認
		if (InRange(current.x, 0, MapGenerator::MAP_SIZE - 1) && InRange(current.y, 0, MapGenerator::MAP_SIZE - 1)) {
			map[current.y][current.x] = 1; // 床/通路としてマーク
		}
		current.y += (p2.y > current.y) ? 1 : -1; // p2.y に向かって1マス進む
	}
	// 最終目的地 p2 も確実に掘る
	if (InRange(current.x, 0, MapGenerator::MAP_SIZE - 1) && InRange(current.y, 0, MapGenerator::MAP_SIZE - 1)) {
		map[current.y][current.x] = 1;
	}
}

// ミニマップを生成する処理
// 戻り値: 文字の2次元配列で表現されたミニマップ
Array<Array<char>> MapGenerator::generateMiniMap() {
	// MINI_SIZE x MINI_SIZE のミニマップを 'O' (空) で初期化
	Array<Array<char>> miniMap(MINI_SIZE, Array<char>(MINI_SIZE, 'O'));
	// 作成する部屋の数をランダムに決定 (5～15個)
	int roomCount = Random(5, 15);

	// 'R' (通常の部屋) をミニマップ上にランダムに配置
	for (int i = 0; i < roomCount; ++i) {
		while (true) { // 空いているマスが見つかるまでループ
			int x = Random(0, MINI_SIZE - 1); // ランダムなX座標
			int y = Random(0, MINI_SIZE - 1); // ランダムなY座標
			if (miniMap[y][x] == 'O') { // そのマスが空('O')なら
				miniMap[y][x] = 'R';      // 'R'を配置
				break;                    // 次の部屋の配置へ
			}
		}
	}

	// 配置された 'R' の部屋の中からランダムに2つ選び、'S' (スタート) と 'G' (ゴール) にする
	Array<Point> roomPositions; // 'R'部屋の座標を格納する配列
	for (int y = 0; y < MINI_SIZE; ++y) {
		for (int x = 0; x < MINI_SIZE; ++x) {
			if (miniMap[y][x] == 'R') {
				roomPositions.push_back(Point{ x, y });
			}
		}
	}

	roomPositions.shuffle(); // 部屋の座標リストをシャッフル
	if (roomPositions.size() >= 2) { // R部屋が2つ以上ある場合のみSとGを設定
		miniMap[roomPositions[0].y][roomPositions[0].x] = 'S'; // 最初の部屋をスタート 'S' に
		miniMap[roomPositions[1].y][roomPositions[1].x] = 'G'; // 次の部屋をゴール 'G' に
	} else if (roomPositions.size() == 1) { // R部屋が1つの場合、Sのみ設定 (Gは別途考慮が必要)
		miniMap[roomPositions[0].y][roomPositions[0].x] = 'S';
		// この場合、ゴールが設定されないため、generateFullMapでのエラーハンドリングが重要になる
	}
	// R部屋が0の場合はSもGも設定されない

	return miniMap;
}

// 実際のゲームマップ (フルマップ) を生成する処理
// 引数: miniMap - generateMiniMap() で生成されたミニマップのレイアウト
// 戻り値: 整数の2次元グリッドで表現されたフルマップ
Grid<int> MapGenerator::generateFullMap(const Array<Array<char>>& miniMap) {
	// 生成結果を格納するメンバ変数をリセット
	startTile_generated.reset();
	goalTile_generated.reset();
	this->generatedRoomAreas.clear();

	// MAP_SIZE x MAP_SIZE のフルマップを 0 (壁) で初期化
	Grid<int> map(MAP_SIZE, MAP_SIZE, 0);
	// ミニマップの各セルに対応するフルマップ上の部屋情報を格納する配列 (Optionalで部屋が存在しない場合も表現)
	Array<Array<Optional<Room>>> rooms(MINI_SIZE, Array<Optional<Room>>(MINI_SIZE));

	// --- 1. 部屋の生成と配置 ---
	// ミニマップの各マスを処理
	for (int y = 0; y < MINI_SIZE; ++y) {
		for (int x = 0; x < MINI_SIZE; ++x) {
			char cell = miniMap[y][x]; // ミニマップのセルの種類 ('S', 'G', 'R', 'O')
			if (cell == 'O') continue; // 'O' (空) マスはスキップ

			// 隣接する部屋（ミニマップ上）に応じて、部屋間の余白（マージン）を設定
			// これにより、フルマップ上で部屋同士が近すぎないようにする
			int marginL = 0, marginR = 0, marginT = 0, marginB = 0; // 左、右、上、下のマージン
			if (x > 0 && miniMap[y][x - 1] != 'O') marginL = 4;             // 左に部屋があれば左マージン
			if (x < MINI_SIZE - 1 && miniMap[y][x + 1] != 'O') marginR = 4; // 右に部屋があれば右マージン
			if (y > 0 && miniMap[y - 1][x] != 'O') marginT = 4;             // 上に部屋があれば上マージン
			if (y < MINI_SIZE - 1 && miniMap[y + 1][x] != 'O') marginB = 4; // 下に部屋があれば下マージン

			// マージンを考慮した、このミニマップセル内に配置可能な部屋の領域を計算
			int startX = x * ROOM_UNIT + marginL;                     // 配置可能領域の開始X座標
			int startY = y * ROOM_UNIT + marginT;                     // 配置可能領域の開始Y座標
			int maxWidth = ROOM_UNIT - marginL - marginR;             // 配置可能な最大幅
			int maxHeight = ROOM_UNIT - marginT - marginB;            // 配置可能な最大高さ

			// 部屋の実際のサイズと位置をランダムに決定 (最小辺の長さは3タイル)
			int roomW = Random(3, maxWidth);                          // 部屋の幅
			int roomH = Random(3, maxHeight);                         // 部屋の高さ
			int offsetX = (maxWidth > roomW) ? Random(0, maxWidth - roomW) : 0;     // 配置可能領域内でのXオフセット
			int offsetY = (maxHeight > roomH) ? Random(0, maxHeight - roomH) : 0;   // 配置可能領域内でのYオフセット

			// フルマップ上での部屋の矩形領域を決定
			Rect roomRect(startX + offsetX, startY + offsetY, roomW, roomH);

			// マップ上に部屋を描画 (タイルIDを 1:床/通路 に設定)
			for (int yy = roomRect.y; yy < roomRect.y + roomRect.h; ++yy) {
				for (int xx = roomRect.x; xx < roomRect.x + roomRect.w; ++xx) {
					if (InRange(xx, 0, MAP_SIZE - 1) && InRange(yy, 0, MAP_SIZE - 1)) { // 範囲チェック
						map[yy][xx] = 1;
					}
				}
			}
			// 生成された部屋の矩形情報をリストに追加 (デバッグや敵のスポーンなどに使用)
			this->generatedRoomAreas.push_back(roomRect);
			// このミニマップセルに対応する部屋情報を保存
			rooms[y][x] = Room{ roomRect, cell };
		}
	}

	// --- 2. 通路の生成 ---
	// 各部屋を隣接する部屋とL字型の通路で接続する
	for (int y = 0; y < MINI_SIZE; ++y) {
		for (int x = 0; x < MINI_SIZE; ++x) {
			if (!rooms[y][x].has_value()) continue; // このセルに部屋がなければスキップ
			const Room& currentRoom = rooms[y][x].value(); // 現在処理中の部屋

			// 上下左右の隣接する部屋との接続を試みる
			// Array<Point>{ {0, -1}, {0, 1}, {-1, 0}, {1, 0} } は 上, 下, 左, 右 の相対座標
			for (auto [dx, dy] : Array<Point>{ {0, -1}, {0, 1}, {-1, 0}, {1, 0} }) {
				int nx = x + dx; // 隣接セルのX座標
				int ny = y + dy; // 隣接セルのY座標

				// 隣接セルがミニマップ範囲内か確認
				if (InRange(nx, 0, MINI_SIZE - 1) && InRange(ny, 0, MINI_SIZE - 1)) {
					if (rooms[ny][nx].has_value()) { // 隣接セルに部屋が存在する場合
						const Room& neighborRoom = rooms[ny][nx].value(); // 隣接する部屋

						// スタート(S)部屋とゴール(G)部屋を直接接続しないようにする
						if ((currentRoom.type == 'S' && neighborRoom.type == 'G') ||
							(currentRoom.type == 'G' && neighborRoom.type == 'S')) {
							continue; // S-G間の直接通路は作らない
						}

						// 通路の始点と終点を部屋の辺からランダムに選択
						Point pathStart, pathEnd;
						if (dx == 1) { // 隣が右の部屋の場合
							pathStart = Point(currentRoom.area.x + currentRoom.area.w - 1, Random(currentRoom.area.y, currentRoom.area.y + currentRoom.area.h - 1));
							pathEnd = Point(neighborRoom.area.x, Random(neighborRoom.area.y, neighborRoom.area.y + neighborRoom.area.h - 1));
						} else if (dx == -1) { // 隣が左の部屋の場合
							pathStart = Point(currentRoom.area.x, Random(currentRoom.area.y, currentRoom.area.y + currentRoom.area.h - 1));
							pathEnd = Point(neighborRoom.area.x + neighborRoom.area.w - 1, Random(neighborRoom.area.y, neighborRoom.area.y + neighborRoom.area.h - 1));
						} else if (dy == 1) { // 隣が下の部屋の場合
							pathStart = Point(Random(currentRoom.area.x, currentRoom.area.x + currentRoom.area.w - 1), currentRoom.area.y + currentRoom.area.h - 1);
							pathEnd = Point(Random(neighborRoom.area.x, neighborRoom.area.x + neighborRoom.area.w - 1), neighborRoom.area.y);
						} else { // dy == -1, 隣が上の部屋の場合
							pathStart = Point(Random(currentRoom.area.x, currentRoom.area.x + currentRoom.area.w - 1), currentRoom.area.y);
							pathEnd = Point(Random(neighborRoom.area.x, neighborRoom.area.x + neighborRoom.area.w - 1), neighborRoom.area.y + neighborRoom.area.h - 1);
						}

						// 通路の座標をマップ境界内にクランプ (念のため)
						pathStart.x = Clamp(pathStart.x, 0, MAP_SIZE - 1);
						pathStart.y = Clamp(pathStart.y, 0, MAP_SIZE - 1);
						pathEnd.x = Clamp(pathEnd.x, 0, MAP_SIZE - 1);
						pathEnd.y = Clamp(pathEnd.y, 0, MAP_SIZE - 1);

						// L字型の経路を掘る
						carvePath(map, pathStart, pathEnd);
					}
				}
			}
		}
	}

	// --- 3. 全部屋の連結性確保 ---
	// このセクションでは、最初の通路生成ステップ後に分断されている可能性のある部屋群を接続する。
	// まず、アクティブな部屋（実際にミニマップに配置された部屋）をリストアップする。
	struct RoomInfoInternal { // 連結性解析中に使用する部屋情報
		const Room* roomPtr;    // Roomオブジェクトへのポインタ
		Point miniMapPos;       // ミニマップ上の座標 (デバッグ用)
		int id;                 // activeRooms配列内のインデックス (一意なIDとして使用)
	};
	Array<RoomInfoInternal> activeRooms; // アクティブな部屋のリスト
	Grid<Optional<int>> roomMiniMapToActiveID(MINI_SIZE, MINI_SIZE); // ミニマップ座標からactiveRoomsのIDへのマッピング

	for (int y_mini = 0; y_mini < MINI_SIZE; ++y_mini) {
		for (int x_mini = 0; x_mini < MINI_SIZE; ++x_mini) {
			if (rooms[y_mini][x_mini].has_value()) {
				int currentId = static_cast<int>(activeRooms.size());
				activeRooms.push_back(RoomInfoInternal{ &rooms[y_mini][x_mini].value(), Point(x_mini,y_mini), currentId });
				roomMiniMapToActiveID[y_mini][x_mini] = currentId;
			}
		}
	}

	if (activeRooms.isEmpty()) { // アクティブな部屋がない場合は警告を出して終了
		Console << U"警告: 相互接続するアクティブな部屋が見つかりません。";
	} else {
		// 部屋間の接続関係を表す隣接リストを構築 (初期通路に基づく)
		Array<Array<int>> roomAdjList(activeRooms.size());
		for (int y_mini = 0; y_mini < MINI_SIZE; ++y_mini) {
			for (int x_mini = 0; x_mini < MINI_SIZE; ++x_mini) {
				if (!rooms[y_mini][x_mini].has_value()) continue;
				int currentRoomActiveId = roomMiniMapToActiveID[y_mini][x_mini].value();
				const Room& currentRoomData = rooms[y_mini][x_mini].value();

				// 重複カウントを避けるため、右と下の隣人のみチェック
				for (auto [dx, dy] : Array<Point>{ {1, 0}, {0, 1} }) {
					int nx_mini = x_mini + dx;
					int ny_mini = y_mini + dy;
					if (InRange(nx_mini, 0, MINI_SIZE - 1) && InRange(ny_mini, 0, MINI_SIZE - 1)) {
						if (rooms[ny_mini][nx_mini].has_value()) {
							const Room& neighborRoomData = rooms[ny_mini][nx_mini].value();
							// S-G間の直接接続ルールを再確認
							if (!((currentRoomData.type == 'S' && neighborRoomData.type == 'G') ||
								  (currentRoomData.type == 'G' && neighborRoomData.type == 'S'))) {
								// ミニマップ上で隣接しており、S-G間の直接接続でなければ、
								// 初期通路生成によって接続された可能性があるとみなし、隣接リストに追加する。
								// この隣接リストは部屋レベルの連結性を示し、後のBFSで使用される。
								// タイルレベルでの厳密な連結性は、この後の強制接続ステップやS-G接続チェックで保証される。
								int neighborRoomActiveId = roomMiniMapToActiveID[ny_mini][nx_mini].value();
								roomAdjList[currentRoomActiveId].push_back(neighborRoomActiveId);
								roomAdjList[neighborRoomActiveId].push_back(currentRoomActiveId);
							}
						}
					}
				}
			}
		}
		// 隣接リストをソートし、重複を削除
		for (auto& adj : roomAdjList) {
			adj.sort_and_unique();
		}

		// BFSを使用して連結された部屋のコンポーネントを特定
		// roomComponent 配列: 各アクティブな部屋がどの連結コンポーネントに属するかを示すIDを格納。-1は未割り当て。
		Array<int> roomComponent(activeRooms.size(), -1);
		int totalComponents = 0; // 発見された連結コンポーネントの総数

		for (int i = 0; i < activeRooms.size(); ++i) {
			if (roomComponent[i] == -1) { // まだコンポーネントに割り当てられていない部屋iからBFSを開始
				totalComponents++; // 新しい連結コンポーネントを発見
				std::queue<int> component_q; // BFS用のキュー
				component_q.push(i);         // 開始部屋をキューに追加
				roomComponent[i] = totalComponents; // 新しいコンポーネントIDを割り当て

				while (!component_q.empty()) {
					int u_active_idx = component_q.front(); // キューの先頭の部屋を取り出す
					component_q.pop();

					// 取り出した部屋uに隣接する全ての部屋vを調べる (roomAdjListに基づく)
					for (int v_active_idx : roomAdjList[u_active_idx]) {
						if (roomComponent[v_active_idx] == -1) { // 隣接部屋vがまだコンポーネントに割り当てられていなければ
							roomComponent[v_active_idx] = totalComponents; // 同じコンポーネントIDを割り当て
							component_q.push(v_active_idx);                // キューに追加して探索を続ける
						}
					}
				}
			}
		}

		// 複数の連結コンポーネントが存在する場合、それらを接続する
		if (totalComponents > 1) { // 連結コンポーネントが2つ以上あれば、マップが分断されている
			// Console << Format(U"複数の部屋コンポーネント ({}) が検出されました。分断されたコンポーネント間を接続します。", totalComponents);
			int mainComponentID = roomComponent[0]; // 最初の部屋のコンポーネントをメインとする

			for (int currentCompID = 1; currentCompID <= totalComponents; ++currentCompID) {
				if (currentCompID == mainComponentID) continue; // メインコンポーネント自体はスキップ

				double minDistance = Math::Inf; // 最短距離
				Optional<std::pair<int, int>> closestPairActiveIndices; // 最も近い部屋ペアのインデックス

				// mainComponentID に属する部屋と currentCompID に属する部屋の間で最も近いペアを探す
				for (int i = 0; i < activeRooms.size(); ++i) {
					if (roomComponent[i] == mainComponentID) {
						for (int j = 0; j < activeRooms.size(); ++j) {
							if (roomComponent[j] == currentCompID) {
								// 部屋の中心点間の距離を計算
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

				// 最も近いペアが見つかったら、それらを carvePath で接続
				if (closestPairActiveIndices.has_value()) {
					int roomIdx1_active = closestPairActiveIndices.value().first;
					int roomIdx2_active = closestPairActiveIndices.value().second;

					Point p1_center = activeRooms[roomIdx1_active].roomPtr->area.center().asPoint();
					Point p2_center = activeRooms[roomIdx2_active].roomPtr->area.center().asPoint();

					// carvePath に渡す前に座標をクランプ
					p1_center.x = Clamp(p1_center.x, 0, MAP_SIZE - 1); p1_center.y = Clamp(p1_center.y, 0, MAP_SIZE - 1);
					p2_center.x = Clamp(p2_center.x, 0, MAP_SIZE - 1); p2_center.y = Clamp(p2_center.y, 0, MAP_SIZE - 1);

					carvePath(map, p1_center, p2_center); // 強制的に通路を掘る
					// Console << Format(U"強制接続: 部屋 {} (コンポーネント {}) と 部屋 {} (コンポーネント {})", activeRooms[roomIdx1_active].miniMapPos, roomComponent[roomIdx1_active], activeRooms[roomIdx2_active].miniMapPos, roomComponent[roomIdx2_active]);
					// 注: ここではroomComponent配列を更新していない。物理的な経路が作られることを重視。
				}
			}
		}
	}
	// --- 全部屋の連結性確保ここまで ---

	// --- 4. スタート(S)とゴール(G)間の連結性確認と強制接続 ---
	Optional<Room> sRoomOpt, gRoomOpt; // スタート部屋とゴール部屋の情報を格納
	for (int y_mini = 0; y_mini < MINI_SIZE; ++y_mini) {
		for (int x_mini = 0; x_mini < MINI_SIZE; ++x_mini) {
			if (rooms[y_mini][x_mini].has_value()) {
				if (rooms[y_mini][x_mini].value().type == 'S') {
					sRoomOpt = rooms[y_mini][x_mini].value();
				} else if (rooms[y_mini][x_mini].value().type == 'G') {
					gRoomOpt = rooms[y_mini][x_mini].value();
				}
			}
		}
	}

	// スタート部屋またはゴール部屋が見つからない場合はエラーとして処理
	if (!sRoomOpt.has_value() || !gRoomOpt.has_value()) {
		Console << U"エラー: スタート('S')またはゴール('G')部屋がミニマップに見つかりません。";
		startTile_generated.reset(); // 念のためリセット
		goalTile_generated.reset();
		return map; // 現在のマップをそのまま返す
	}

	// スタートタイルとゴールタイルの位置をメンバ変数に保存 (部屋の左上のタイルとする)
	// 座標はマップ境界内にクランプする
	Point sPos = sRoomOpt.value().area.tl();
	sPos.x = Clamp(sPos.x, 0, MAP_SIZE - 1);
	sPos.y = Clamp(sPos.y, 0, MAP_SIZE - 1);
	startTile_generated = sPos;

	Point gPos = gRoomOpt.value().area.tl();
	gPos.x = Clamp(gPos.x, 0, MAP_SIZE - 1);
	gPos.y = Clamp(gPos.y, 0, MAP_SIZE - 1);
	goalTile_generated = gPos;

	// BFSでスタート部屋からゴール部屋への到達可能性をチェック
	const Room& sRoom = sRoomOpt.value();
	const Room& gRoom = gRoomOpt.value();
	bool s_g_connected = false; // S-G間が接続されているか

	Grid<bool> visited(MAP_SIZE, MAP_SIZE, false); // BFS用の訪問済みグリッド
	std::queue<Point> q_bfs;                       // BFS用のキュー

	// BFSの開始地点をスタート部屋内から探す (通行可能なタイル)
	Optional<Point> bfsStartPointOpt;
	Point sRoomCenter = sRoom.area.center().asPoint();
	sRoomCenter.x = Clamp(sRoomCenter.x, 0, MAP_SIZE - 1); // 座標をクランプ
	sRoomCenter.y = Clamp(sRoomCenter.y, 0, MAP_SIZE - 1);

	if (map[sRoomCenter.y][sRoomCenter.x] == 1) { // 部屋の中心が床ならそこから開始
		bfsStartPointOpt = sRoomCenter;
	} else { // 中心が床でない場合、部屋内の他の床タイルを探す
		for (int ry = sRoom.area.y; ry < sRoom.area.y + sRoom.area.h; ++ry) {
			for (int rx = sRoom.area.x; rx < sRoom.area.x + sRoom.area.w; ++rx) {
				Point currentTile(Clamp(rx, 0, MAP_SIZE - 1), Clamp(ry, 0, MAP_SIZE - 1));
				if (map[currentTile.y][currentTile.x] == 1) {
					bfsStartPointOpt = currentTile;
					goto found_passable_start_tile_for_sg_bfs; // 見つかったらループを抜ける
				}
			}
		}
	found_passable_start_tile_for_sg_bfs:; // ループ脱出用のラベル
	}

	if (!bfsStartPointOpt.has_value()) { // スタート部屋に通行可能なタイルが見つからない場合
		Console << U"エラー: スタート('S')部屋に通行可能なタイルが見つかりません。BFSを実行できません。";
		// この場合、S-Gは非連結とみなし、強制接続へ進む
	} else {
		Point bfsStartTile = bfsStartPointOpt.value();
		q_bfs.push(bfsStartTile);
		visited[bfsStartTile.y][bfsStartTile.x] = true;

		Point DIRS[] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} }; // 上下左右の移動方向

		while (!q_bfs.empty()) {
			Point current = q_bfs.front();
			q_bfs.pop();

			if (gRoom.area.contains(current)) { // ゴール部屋のタイルに到達したら
				s_g_connected = true; // 接続フラグを立てる
				break;                // BFS終了
			}

			for (const auto& dir : DIRS) { // 上下左右の隣接タイルをチェック
				Point next = current + dir;
				// 隣接タイルがマップ範囲内で、床(1)であり、未訪問ならキューに追加
				if (InRange(next.x, 0, MAP_SIZE - 1) && InRange(next.y, 0, MAP_SIZE - 1) &&
					map[next.y][next.x] == 1 && !visited[next.y][next.x]) {
					visited[next.y][next.x] = true;
					q_bfs.push(next);
				}
			}
		}
	}

	// S-G間が連結されていない場合、強制的に通路を掘る
	if (!s_g_connected) {
		Console << U"SとGが連結されていません。強制的に接続します。";
		Point sCenter_final = sRoom.area.center().asPoint();
		Point gCenter_final = gRoom.area.center().asPoint();

		// carvePathに渡す前に座標をクランプ
		sCenter_final.x = Clamp(sCenter_final.x, 0, MAP_SIZE - 1);
		sCenter_final.y = Clamp(sCenter_final.y, 0, MAP_SIZE - 1);
		gCenter_final.x = Clamp(gCenter_final.x, 0, MAP_SIZE - 1);
		gCenter_final.y = Clamp(gCenter_final.y, 0, MAP_SIZE - 1);

		carvePath(map, sCenter_final, gCenter_final);
	}

	return map; // 生成されたフルマップを返す
}
