# include "Game.hpp"

// 静的メンバーの定義と初期化
short Game::s_currentStage = 0;

void Game::GenerateAndSetupNewMap() {
	// 1. MapGeneratorから地図レイアウトを生成する
	auto miniMap = generator.generateMiniMap();
	auto generatedLayout = generator.generateFullMap(miniMap); // Grid<int>

	// 2. currentMapGrid を初期化します。
	currentMapGrid.resize(MapGenerator::MAP_SIZE, MapGenerator::MAP_SIZE);

	// 3. 生成されたレイアウトを現在のマップグリッドに適用し、SタイルとGタイルを特定する。
	if (!generator.startTile_generated.has_value() || !generator.goalTile_generated.has_value()) {
		Console << U"Error: MapGenerator did not set start or goal tile.";
		// フォールバックまたはエラー状態を考慮する
		// 現在は、デフォルトの小さなマップを生成するか、終了する
		// この例では、生成が重大なエラーで失敗した場合、非常にシンプルなフォールバックマップを作成する
		currentMapGrid.assign(MapGenerator::MAP_SIZE, MapGenerator::MAP_SIZE, 1); // All walls
		currentMapGrid[1][1] = 2; // プレイヤー開始
		currentMapGrid[1][2] = 4; // ゴール
		Player->SetPlayerPos(Point{ 1,1 });
		// リトライシナリオにおいて、この時点以前に追加された敵がいる場合、任意でそれらを消去する。
		for (auto* enemy : Enemys) { delete enemy; }
		Enemys.clear();
		return;
	}

	Point playerStartPos = generator.startTile_generated.value();
	Point goalPos = generator.goalTile_generated.value();

	for (int y = 0; y < MapGenerator::MAP_SIZE; ++y) {
		for (int x = 0; x < MapGenerator::MAP_SIZE; ++x) {
			if (Point(x, y) == playerStartPos) {
				currentMapGrid[y][x] = 2; // Player Start
			}
			else if (Point(x, y) == goalPos) {
				currentMapGrid[y][x] = 4; // Goal (Passable, Yellow)
			}
			else if (generatedLayout[y][x] == 0) { // MapGenerator Wall
				currentMapGrid[y][x] = 0; // Game Wall (Passable: No, Draw: No)
			}
			else if (generatedLayout[y][x] == 1) { // MapGenerator Floor/Path
				currentMapGrid[y][x] = 1; // Game Floor (Passable: Yes, Draw: PieceColor)
			}
			else { // Should not happen
				currentMapGrid[y][x] = 0; // Default to Game Wall
			}
		}
	}

	// 4. プレイヤーの位置を設定する
	Player->SetPlayerPos(playerStartPos);
	// Ensure player's starting tile is marked as player start, not overwritten by debug
	currentMapGrid[playerStartPos.y][playerStartPos.x] = 2;


	// --- BEGIN DEBUG: Mark generated room areas for visualization ---
	const int DEBUG_ROOM_TILE_ID = 5; // Passable: Yes, Draw: Magenta
	if (not generator.generatedRoomAreas.isEmpty()) {
		for (const auto& roomAreaRect : generator.generatedRoomAreas) {
			for (int y_room = roomAreaRect.y; y_room < roomAreaRect.y + roomAreaRect.h; ++y_room) {
				for (int x_room = roomAreaRect.x; x_room < roomAreaRect.x + roomAreaRect.w; ++x_room) {
					if (InRange(x_room, 0, MapGenerator::MAP_SIZE - 1) && InRange(y_room, 0, MapGenerator::MAP_SIZE - 1)) {
						// Mark the area defined by MapGenerator as a room.
						// This should ideally be Game Floor (1) but for debug it's 5.
						// Avoid overwriting Start (2) or Goal (4) tiles.
						if (currentMapGrid[y_room][x_room] != 2 && currentMapGrid[y_room][x_room] != 4) {
							// If it was MG Floor (now Game Floor 1) or MG Wall (now Game Wall 0), mark as debug room area.
							currentMapGrid[y_room][x_room] = DEBUG_ROOM_TILE_ID;
						}
					}
				}
			}
		}
	}
	// --- END DEBUG ---


	// マップの境界線を壁にする処理を削除 (ユーザー要望により壁をなくす)
	// int gridWidth = currentMapGrid.width();
	// int gridHeight = currentMapGrid.height();
	// if (gridWidth > 0 && gridHeight > 0) { ... } // Boundary wall code removed

	// 5. 敵をスポーンする（敵の配列が空であることを確認 - 新しいゲームインスタンスが作成される前にデストラクタで処理される）
	// SとGのタイル位置を取得し、これらの部屋を特定して、それらに敵をスポーンしないようにする。
	Optional<Point> startTileOpt = generator.startTile_generated;
	Optional<Point> goalTileOpt = generator.goalTile_generated;

	for (const auto& roomAreaRect : generator.generatedRoomAreas) {
		// 現在のroomAreaRectがStartまたはGoalの部屋に対応しているかどうかを判定します。
		bool isStartRoom = false;
		if (startTileOpt.has_value() && roomAreaRect.contains(startTileOpt.value())) {
			isStartRoom = true;
		}

		bool isGoalRoom = false;
		if (goalTileOpt.has_value() && roomAreaRect.contains(goalTileOpt.value())) {
			isGoalRoom = true;
		}

		if (isStartRoom || isGoalRoom) {
			continue; // スタートまたはゴール部屋での敵の出現をスキップする
		}

		// 部屋の面積（部屋の矩形の幅 × 高さ）に基づいて敵の数を決定します。
		int roomTileArea = roomAreaRect.w * roomAreaRect.h;
		// 例：敵の生成ルール：エリアの25タイルごとに1体の敵を生成し、1部屋あたり最大3体まで。最小0体。
		int numEnemiesToSpawn = Clamp(roomTileArea / 25, 0, 3);

		for (int i = 0; i < numEnemiesToSpawn; ++i) {
			// 部屋内で有効なスポーンポイントを探索する試みを、限られた回数で行う。
			for (int attempt = 0; attempt < 10; ++attempt) {
				int spawnX = Random(roomAreaRect.x, roomAreaRect.x + roomAreaRect.w - 1);
				int spawnY = Random(roomAreaRect.y, roomAreaRect.y + roomAreaRect.h - 1);
				Point spawnPos(spawnX, spawnY);

				// Check if the randomly chosen position is within the map grid bounds
				// AND is a Game Floor tile (type 1) or a Debug Room Tile (type 5) in currentMapGrid.
				if (s3d::InRange(spawnPos.x, 0, static_cast<int>(currentMapGrid.width() - 1)) &&
					s3d::InRange(spawnPos.y, 0, static_cast<int>(currentMapGrid.height() - 1))) {

					int tileAtSpawn = currentMapGrid[spawnPos.y][spawnPos.x];
					if (tileAtSpawn == 1 || tileAtSpawn == 5) { // Game Floor (1) or Debug Room Area (5)
						Enemys << new BaseEnemy(spawnPos, 0); // 新しい敵（タイプ0）を作成して追加する
						// 注意：currentMapGrid[spawnPos.y][spawnPos.x]をタイルタイプ3（敵）に変更しないでください。
					// 敵の位置はEnemys配列で追跡されます。
						break; // 敵を1体生成に成功しました。次に存在する敵がいる場合、その敵に移動します。
					}
				}
			}
		}
	}

}
Game::Game(const InitData& init)
	: IScene{ init }
{
	Player = new BasePlayer;
	GenerateAndSetupNewMap(); // Generate the first map

	//カメラの初期位置
	// Player->GetPlayerPos() is now set by GenerateAndSetupNewMap()
	Point playerPixelPos = { (PieceSize * Player->GetPlayerPos().x) + (WallThickness * (Player->GetPlayerPos().x + 1)),
							 (PieceSize * Player->GetPlayerPos().y) + (WallThickness * (Player->GetPlayerPos().y + 1)) };
	camera = new Camera(playerPixelPos - Point((800 - 150) / 2, (600 - 150) / 2));

	// Initialize hit effects
	m_hitEffects.setSpeed(2.0);
	m_hitEffects.setMaxLifeTime(0.3s);
}

Game::~Game() {
	delete Player;
	Player = nullptr;

	delete camera;
	camera = nullptr;

	for (auto* enemy : Enemys) {
		delete enemy;
	}
	// Enemys.clear(); // s3d::Array clears itself
}

void Game::update()
{
	// Continuous Movement Logic
	Point moveInput(0, 0);
	bool newKeyPressedThisFrame = false;

	if (KeyNum1.pressed()) moveInput.set(-1, 1);
	else if (KeyNum2.pressed()) moveInput.set(0, 1);
	else if (KeyNum3.pressed()) moveInput.set(1, 1);
	else if (KeyNum4.pressed()) moveInput.set(-1, 0);
	else if (KeyNum6.pressed()) moveInput.set(1, 0);
	else if (KeyNum7.pressed()) moveInput.set(-1, -1);
	else if (KeyNum8.pressed()) moveInput.set(0, -1);
	else if (KeyNum9.pressed()) moveInput.set(1, -1);

	if (moveInput != Point(0, 0)) {
		if (!m_heldMoveDirection.has_value() || m_heldMoveDirection.value() != moveInput) {
			// New direction or first press for this direction sequence
			newKeyPressedThisFrame = true;
			m_isAttackIntent = true;
			InputMove(moveInput.x, moveInput.y);
			m_heldMoveDirection = moveInput;
			m_isWaitingForInitialRepeat = true;
			m_initialMoveDelayTimer.restart();
		}
		else { // Key is still held for the same direction
			m_isAttackIntent = false; // Default to no attack intent for repeats
			if (m_isWaitingForInitialRepeat) {
				if (m_initialMoveDelayTimer.reachedZero()) {
					m_isWaitingForInitialRepeat = false;
					m_moveRepeatTimer.restart();
					InputMove(moveInput.x, moveInput.y);
				}
			}
			else if (m_moveRepeatTimer.reachedZero()) {
				m_moveRepeatTimer.restart();
				InputMove(moveInput.x, moveInput.y);
			}
		}
	}
	else {
		// No move key is pressed
		if (m_heldMoveDirection.has_value()) { // Was moving, now stopped
			m_isAttackIntent = false;
		}
		m_heldMoveDirection.reset();
		m_initialMoveDelayTimer.pause();
		m_moveRepeatTimer.pause();
		m_isWaitingForInitialRepeat = false;
		// m_isAttackIntent is reset/false if no keys pressed or just released
	}

	if (KeyNum5.down()) { // Wait command, single trigger
		m_isAttackIntent = true;
		InputMove(0, 0);
		m_isAttackIntent = false; // Reset after single action
	}

	// If no new key initiated a move this frame, ensure attack intent is false for any potential subsequent logic.
	if (!newKeyPressedThisFrame && moveInput == Point(0, 0) && !KeyNum5.down()) {
		m_isAttackIntent = false;
	}

	if (KeyM.down()) {
		showFullMap = !showFullMap;
	}

	// Camera Shake Logic
	if (m_cameraShakeTimer.isRunning()) {
		if (m_cameraShakeTimer.reachedZero()) {
			m_cameraShakeOffset.reset();
		}
		else {
			m_cameraShakeOffset = s3d::RandomVec2() * 3.0; // Shake magnitude
		}
	}
	else {
		m_cameraShakeOffset.reset(); // Ensure offset is cleared
	}

	m_hitEffects.update(); // Update particle effects

	Vec2 lungeVisualOffset = Vec2::Zero(); // This local variable is not used here, lunge offset for drawing is calculated in draw()
	if (m_playerLungeDirection.has_value() && m_playerLungeTimer.isRunning()) {
		if (m_playerLungeTimer.reachedZero()) {
			m_playerLungeDirection.reset();
		}
		else {
			double progress = m_playerLungeTimer.progress0_1();
			double lungeAmplitude = Sin(progress * Math::Pi); // Smooth 0 -> 1 -> 0 curve
			double lungeDistance = static_cast<double>(PieceSize) * 0.3;
			lungeVisualOffset = m_playerLungeDirection.value() * lungeAmplitude * lungeDistance;
		}
	}
	else if (m_playerLungeDirection.has_value()) {
		m_playerLungeDirection.reset(); // Cleanup if timer stopped abruptly or finished last frame
	}
}

void Game::InputMove(int _x, int _y) {


	Array<Point>enemyPoss;
	for (auto& enemy : Enemys) {
		enemyPoss << enemy->GetEnemyPos();
	}

	//プレイヤー移動
	Point enemyHitPos = Player->Move(_x, _y, currentMapGrid); // Use currentMapGrid

	//プレイヤーがマップを進めるマスにいるか (Goal tile is 4)
	if (currentMapGrid[Player->GetPlayerPos().y][Player->GetPlayerPos().x] == 4) {
		Game::s_currentStage++;
		const int MAX_STAGES = 10; // Define max stages

		if (Game::s_currentStage >= MAX_STAGES) {
			Game::s_currentStage = 0; // Reset for the next full game playthrough
			changeScene(State::Title);
		}
		else {
			changeScene(State::Game); // Reload the game scene for the next stage
		}
		return; // Important: Stop further processing in InputMove after a scene change
	}
	//カメラ更新
	camera->MoveCamera(PieceSize, WallThickness, Player->GetPlayerPos());

	//ダメージ
	if (enemyHitPos != Point{ -1,-1 }) { // If player's intended move was onto an enemy
		// Stop continuous movement regardless of attack intent
		m_heldMoveDirection.reset();
		m_initialMoveDelayTimer.pause();
		m_moveRepeatTimer.pause();
		m_isWaitingForInitialRepeat = false;

		if (m_isAttackIntent) { // Only attack if it was an initial, intentional action
			for (int i = 0; i < Enemys.size(); i++) {
				if (Enemys[i] && Enemys[i]->GetEnemyPos() == enemyHitPos) {
					Enemys[i]->Damage(Player->Attack());
					m_cameraShakeTimer.restart(); // Start/Restart camera shake

					// Add particles for hit effect
					Point enemyGridPos = Enemys[i]->GetEnemyPos();
					Vec2 enemyCenterPixelPos = Vec2{
						(PieceSize * enemyGridPos.x) + (WallThickness * (enemyGridPos.x + 1)) + (PieceSize / 2.0),
						(PieceSize * enemyGridPos.y) + (WallThickness * (enemyGridPos.y + 1)) + (PieceSize / 2.0)
					};
					Vec2 lungeDir = (enemyHitPos - Player->GetPlayerPos());
					if (lungeDir.lengthSq() > 0) {
						lungeDir.normalize();
					}
					else {
						lungeDir = Vec2{ 1,0 }; // Default if somehow on same tile
					}
					m_playerLungeDirection = lungeDir;
					m_playerLungeTimer.restart();
					// No need to change tile on map for enemy damage/death, handled by enemy removal
				}
			}
		}
		// After processing potential attack, reset intent flag if it was true,
		// so it's not accidentally reused by other logic before next update cycle.
		// This is important if InputMove could be called multiple times with stale m_isAttackIntent.
		// However, the update loop's structure should handle setting it false for repeats.
		// For safety, if an attack was intended and processed, reset the flag.
		// This is done after the loop and effects if an attack occurred.
		bool attackOccurred = false;

		if (m_isAttackIntent) { // Only attack if it was an initial, intentional action
			for (int i = 0; i < Enemys.size(); i++) { // Iterate through enemies to find the one at enemyHitPos
				if (Enemys[i] && Enemys[i]->GetEnemyPos() == enemyHitPos) {
					Enemys[i]->Damage(Player->Attack());
					m_cameraShakeTimer.restart(); // Start/Restart camera shake
					attackOccurred = true;

					// Add particles for hit effect (This code was previously misplaced)
					Point particleOriginEnemyGridPos = Enemys[i]->GetEnemyPos(); // Use 'i' from this loop
					// Vec2 particleOriginPixelPos = ... ; // Calculate pixel position for particles if needed by m_hitEffects.add
					// For now, m_hitEffects.add seems to not be used, but lunge is set.

					Vec2 lungeDir = (enemyHitPos - Player->GetPlayerPos());
					if (lungeDir.lengthSq() > 0) {
						lungeDir.normalize();
					}
					else {
						lungeDir = Vec2{ 1,0 }; // Default if somehow on same tile
					}
					m_playerLungeDirection = lungeDir;
					m_playerLungeTimer.restart();
					// No need to change tile on map for enemy damage/death, handled by enemy removal
					break; // Found and processed the enemy
				}
			}
		}

		if (m_isAttackIntent) { // If an action was intended (even if no specific enemy was hit, e.g. attacking empty space)
			m_isAttackIntent = false; // Reset intent after the action (or attempted action)
		}
	} // End of if (enemyHitPos != Point{ -1,-1 })

	//敵の生存確認 & remove dead enemies
	// When an enemy dies, its tile should revert to floor (0)
	// This part is tricky: BaseEnemy doesn't store its original tile type.
	// For now, we assume if an enemy was on a tile, it becomes floor (0).
	// A better way would be to have a separate grid for dynamic entities or store original tile.
	for (int i = static_cast<int>(Enemys.size()) - 1; i >= 0; --i) {
		if (Enemys[i]->GetDeath()) {
			Point deadEnemyPos = Enemys[i]->GetEnemyPos();
			// Ensure position is valid before writing to grid
			if (s3d::InRange(deadEnemyPos.x, 0, static_cast<int>(currentMapGrid.width() - 1)) &&
				s3d::InRange(deadEnemyPos.y, 0, static_cast<int>(currentMapGrid.height() - 1))) {
				currentMapGrid[deadEnemyPos.y][deadEnemyPos.x] = 1; // Set tile to new Game Floor ID (1)
			}
			delete Enemys[i];
			Enemys.remove_at(i);
		}
	}
	// Enemys.remove_if([](BaseEnemy* x) {bool isDead = x->GetDeath(); if(isDead) delete x; return isDead; }); // Alternative with delete

	//エネミー移動と攻撃
	for (auto& currentEnemy : Enemys) { // Renamed to avoid conflict
		Player->Damage(currentEnemy->Move(Player->GetPlayerPos(), currentMapGrid)); // Use currentMapGrid
	}
}

	// void Game::Map() { // Removed as per instruction
	// }

void Game::draw() const
{
	Scene::SetBackground(ColorF{ 0.2 });

	if (currentMapGrid.isEmpty()) return; // Guard against drawing empty map

	//ステージプレーン
	for (int y = 0; y < currentMapGrid.height(); y++) { // Use currentMapGrid
		for (int x = 0; x < currentMapGrid.width(); x++) { // Use currentMapGrid
			int tileType = currentMapGrid[y][x];
			if (tileType == 0) { // New Game Wall - Do not draw
				// No drawing operation
			}
			else if (tileType == 1) { // New Game Floor
				getPaddle(x, y).rounded(3).draw(PieceColor);
			}
			else if (tileType == 2) { // Player Start Tile
				getPaddle(x, y).rounded(3).draw(Palette::Green);
			}
			else if (tileType == 4) { // Goal
				getPaddle(x, y).rounded(3).draw(Palette::Yellow);
			}
			else if (tileType == 5) { // DEBUG_ROOM_TILE_ID
				getPaddle(x, y).rounded(3).draw(Palette::Magenta);
			}
			// Removed default else case, as all valid tile types should be handled.
			// If a tile is an unexpected value, it simply won't be drawn.
		}
	}

	// Player is drawn by its own class or needs to be drawn here
	if (Player) { // Ensure Player object exists
		Point playerGridPos = Player->GetPlayerPos();
		RectF playerBodyBase = getPaddle(playerGridPos.x, playerGridPos.y);

		Vec2 lungeVisualOffsetDraw = Vec2::Zero(); // Renamed to avoid conflict with update's local var
		if (m_playerLungeDirection.has_value() && m_playerLungeTimer.isRunning()) {
			double progress = m_playerLungeTimer.progress0_1();
			double lungeAmplitude = Sin(progress * Math::Pi); // Smooth 0 -> 1 -> 0 curve
			double lungeDistance = static_cast<double>(PieceSize) * 0.3;
			lungeVisualOffsetDraw = m_playerLungeDirection.value() * lungeAmplitude * lungeDistance;
		}

		RectF playerVisualRect = playerBodyBase.movedBy(lungeVisualOffsetDraw);
		playerVisualRect.draw(Player->GetSterts().color);
	}

	//敵の移動経路 (Enemies themselves)
	s3d::Vec2 currentShakeVec = m_cameraShakeOffset.value_or(s3d::Vec2::Zero());
	s3d::Point effectiveCameraPointForEnemies = camera->GetCamera() - currentShakeVec.asPoint();

	for (auto& enemy : Enemys) {
		if (enemy) { // Ensure enemy is not null
			enemy->draw(PieceSize, WallThickness, effectiveCameraPointForEnemies); // Pass shaken camera Point
		}
	}

	// m_hitEffects.update() was removed from here as it's already in Game::update()
	// Siv3D Effect system typically handles its own drawing after .update() is called.

	//UI
	{
		MessageWindow.draw(Palette::Black).drawFrame(2, Palette::White);
		MiniMessageWindow.draw(Palette::Black).drawFrame(2, Palette::White);

		CharaWindow.draw(Palette::Black).drawFrame(2, Palette::White);
		texture[0](250, 0, 300, 400).fitted(CharaWindow.size).drawAt(CharaWindow.center().x, 350);
	}

	if (showFullMap) {
		const Point fullMapOffset(10, 10); // Small offset from screen edge

		// Optional: Draw a semi-transparent background for the full map panel
		Rect((fullMapOffset.x - 2), (fullMapOffset.y - 2),
			(MapGenerator::MAP_SIZE * FullMapTileRenderSize) + 4,
			(MapGenerator::MAP_SIZE * FullMapTileRenderSize) + 4)
			.draw(ColorF(0.1, 0.1, 0.1, 0.8));

		for (int y_map = 0; y_map < MapGenerator::MAP_SIZE; ++y_map) { // Renamed loop var
			for (int x_map = 0; x_map < MapGenerator::MAP_SIZE; ++x_map) { // Renamed loop var
				RectF tileRect(fullMapOffset.x + (x_map * FullMapTileRenderSize),
					fullMapOffset.y + (y_map * FullMapTileRenderSize),
					FullMapTileRenderSize, FullMapTileRenderSize);

				if (y_map < currentMapGrid.height() && x_map < currentMapGrid.width()) { // Check bounds
					int tileTypeOnGrid = currentMapGrid[y_map][x_map]; // Renamed var
					if (tileTypeOnGrid == 0) { // New Game Wall - Do not draw on full map either
						// tileRect.draw(Palette::Black); // Or some indicator for "void" if desired
					}
					else if (tileTypeOnGrid == 1) { // New Game Floor
						tileRect.draw(PieceColor);
					}
					else if (tileTypeOnGrid == 2) { // Original Start Position tile
						tileRect.draw(Palette::Green);
					}
					else if (tileTypeOnGrid == 4) { // Goal Position tile
						tileRect.draw(Palette::Yellow);
					}
					else if (tileTypeOnGrid == 5) { // DEBUG_ROOM_TILE_ID
						tileRect.draw(Palette::Magenta);
					}
					// else: other tile types are not drawn on full map
				}
				else {
					tileRect.draw(Palette::Black); // Out of currentMapGrid bounds
				}
			}
			// Draw Player on the full map
			if (Player) { // Check if Player exists
				RectF playerMapRect(fullMapOffset.x + (Player->GetPlayerPos().x * FullMapTileRenderSize),
					fullMapOffset.y + (Player->GetPlayerPos().y * FullMapTileRenderSize),
					FullMapTileRenderSize, FullMapTileRenderSize);
				playerMapRect.draw(Palette::Cyan);
			}

			// Draw Enemies on the full map
			for (const auto& enemy : Enemys) {
				if (enemy) { // Ensure enemy pointer is valid
					RectF enemyMapRect(fullMapOffset.x + (enemy->GetEnemyPos().x * FullMapTileRenderSize),
						fullMapOffset.y + (enemy->GetEnemyPos().y * FullMapTileRenderSize),
						FullMapTileRenderSize, FullMapTileRenderSize);
					enemyMapRect.draw(Palette::Red);
				}
			}
		}
	}

	// The following block is a duplicate of player/enemy drawing logic that appears earlier in draw().
	// It is being removed.
	/*
	// Player is drawn by its own class or needs to be drawn here
	// Player->draw(...); // Assuming BasePlayer has a draw method

	// Explicitly draw the Player
	if (Player) { // Ensure Player object exists
		Point playerGridPos = Player->GetPlayerPos();
		RectF playerBodyBase = getPaddle(playerGridPos.x, playerGridPos.y);

		Vec2 lungeVisualOffset = Vec2::Zero();

		RectF playerVisualRect = playerBodyBase.movedBy(lungeVisualOffset);
		playerVisualRect.draw(Player->GetSterts().color);
	}

	//敵の移動経路 (Enemies themselves)
	s3d::Vec2 currentShakeVec = m_cameraShakeOffset.value_or(s3d::Vec2::Zero());
	s3d::Point effectiveCameraPointForEnemies = camera->GetCamera() - currentShakeVec.asPoint();

	for (auto& enemy : Enemys) {
		if (enemy) { // Ensure enemy is not null
			enemy->draw(PieceSize, WallThickness, effectiveCameraPointForEnemies); // Pass shaken camera Point
		}
	}

	m_hitEffects.update(); // Draw particle effects // This was the duplicate call.
	*/



}

RectF Game::getPaddle(int _x, int _y) const { // Changed return type to RectF
	s3d::Vec2 currentShake = m_cameraShakeOffset.value_or(s3d::Vec2::Zero());
	s3d::Vec2 baseCameraPos = camera->GetCamera(); // camera->GetCamera() is Point
	s3d::Vec2 effectiveCameraPos = baseCameraPos - currentShake;

	return RectF{ (PieceSize * _x) + (WallThickness * (_x + 1)) - effectiveCameraPos.x,
				  (PieceSize * _y) + (WallThickness * (_y + 1)) - effectiveCameraPos.y,
				  static_cast<double>(PieceSize) }; // PieceSize is int, ensure double for RectF
}
