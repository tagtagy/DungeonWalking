# include "Game.hpp"

// Define and Initialize Static Member
short Game::s_currentStage = 0;

void Game::GenerateAndSetupNewMap() {
    // 1. Generate Map Layout from MapGenerator
    auto miniMap = generator.generateMiniMap();
    auto generatedLayout = generator.generateFullMap(miniMap); // Grid<int>

    // 2. Initialize currentMapGrid
    currentMapGrid.resize(MapGenerator::MAP_SIZE, MapGenerator::MAP_SIZE);

    // 3. Adapt generatedLayout to currentMapGrid and identify S and G tiles
    if (!generator.startTile_generated.has_value() || !generator.goalTile_generated.has_value()) {
        Console << U"Error: MapGenerator did not set start or goal tile.";
        // Consider a fallback or error state
        // For now, try to generate a default small map or exit
        // This example will just create a very simple fallback map if generation fails critically
        currentMapGrid.assign(MapGenerator::MAP_SIZE, MapGenerator::MAP_SIZE, 1); // All walls
        currentMapGrid[1][1] = 2; // Player start
        currentMapGrid[1][2] = 4; // Goal
        Player->SetPlayerPos(Point{1,1});
        // Optionally clear enemies if any were added before this point in a retry scenario
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
            } else if (Point(x, y) == goalPos) {
                currentMapGrid[y][x] = 4; // Goal
            } else if (generatedLayout[y][x] == 0) { // MapGenerator Wall (assuming 0 is wall from generator)
                currentMapGrid[y][x] = 1; // Game Wall
            } else if (generatedLayout[y][x] == 1) { // MapGenerator Floor (assuming 1 is floor from generator)
                currentMapGrid[y][x] = 0; // Game Floor
            } else {
                currentMapGrid[y][x] = 1; // Default to wall if unknown tile from generator
            }
        }
    }

    // 4. Set Player Position
    Player->SetPlayerPos(playerStartPos);

    // Ensure map borders are walls, unless it's a Start or Goal tile
    int gridWidth = currentMapGrid.width();
    int gridHeight = currentMapGrid.height();

    if (gridWidth > 0 && gridHeight > 0) { // Proceed only if grid is not empty
        for (int i = 0; i < gridWidth; ++i) {
            // Top border
            if (currentMapGrid[0][i] != 2 && currentMapGrid[0][i] != 4) {
                currentMapGrid[0][i] = 1;
            }
            // Bottom border
            if (currentMapGrid[gridHeight - 1][i] != 2 && currentMapGrid[gridHeight - 1][i] != 4) {
                currentMapGrid[gridHeight - 1][i] = 1;
            }
        }
        for (int i = 0; i < gridHeight; ++i) {
            // Left border
            if (currentMapGrid[i][0] != 2 && currentMapGrid[i][0] != 4) {
                currentMapGrid[i][0] = 1;
            }
            // Right border
            if (currentMapGrid[i][gridWidth - 1] != 2 && currentMapGrid[i][gridWidth - 1] != 4) {
                currentMapGrid[i][gridWidth - 1] = 1;
            }
        }
    }

    // 5. Spawn Enemies (Ensure Enemys array is empty - handled by destructor before new Game instance)
    // Get S and G tile locations to identify these rooms and avoid spawning enemies in them.
    Optional<Point> startTileOpt = generator.startTile_generated;
    Optional<Point> goalTileOpt = generator.goalTile_generated;

    for (const auto& roomAreaRect : generator.generatedRoomAreas) {
        // Determine if the current roomAreaRect corresponds to the Start or Goal room.
        bool isStartRoom = false;
        if (startTileOpt.has_value() && roomAreaRect.contains(startTileOpt.value())) {
            isStartRoom = true;
        }

        bool isGoalRoom = false;
        if (goalTileOpt.has_value() && roomAreaRect.contains(goalTileOpt.value())) {
            isGoalRoom = true;
        }

        if (isStartRoom || isGoalRoom) {
            continue; // Skip enemy spawning in Start or Goal rooms
        }

        // Determine the number of enemies based on room area (width * height of the room's Rect).
        int roomTileArea = roomAreaRect.w * roomAreaRect.h;
        // Example spawning rule: 1 enemy per 25 tiles of area, max 3 enemies per room. Min 0.
        int numEnemiesToSpawn = Clamp(roomTileArea / 25, 0, 3);

        for (int i = 0; i < numEnemiesToSpawn; ++i) {
            // Attempt to find a valid spawn point within the room for a limited number of tries.
            for (int attempt = 0; attempt < 10; ++attempt) {
                int spawnX = Random(roomAreaRect.x, roomAreaRect.x + roomAreaRect.w - 1);
                int spawnY = Random(roomAreaRect.y, roomAreaRect.y + roomAreaRect.h - 1);
                Point spawnPos(spawnX, spawnY);

                // Check if the randomly chosen position is within the map grid bounds
                // AND is a floor tile (type 0) in currentMapGrid.
                if (s3d::InRange(spawnPos.x, 0, static_cast<int>(currentMapGrid.width() - 1)) &&
                    s3d::InRange(spawnPos.y, 0, static_cast<int>(currentMapGrid.height() - 1)) &&
                    currentMapGrid[spawnPos.y][spawnPos.x] == 0) {

                    Enemys << new BaseEnemy(spawnPos, 0); // Create and add new enemy (type 0)
                    // Note: Do not change currentMapGrid[spawnPos.y][spawnPos.x] to tile type 3 (enemy).
                    // Enemy locations are tracked via the Enemys array.
                    break; // Successfully spawned one enemy, move to the next one (if any)
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
	if (KeyNum1.down()) InputMove(-1,  1);
	if (KeyNum2.down()) InputMove( 0,  1);
	if (KeyNum3.down()) InputMove( 1,  1);
	if (KeyNum4.down()) InputMove(-1,  0);
	if (KeyNum5.down()) InputMove( 0,  0);
	if (KeyNum6.down()) InputMove( 1,  0);
	if (KeyNum7.down()) InputMove(-1, -1);
	if (KeyNum8.down()) InputMove( 0, -1);
	if (KeyNum9.down()) InputMove( 1, -1);

	if (KeyM.down()) {
		showFullMap = !showFullMap;
	}

	if (IsMove) {

		if (Move == MoveMode::Walk) {

		}
		if (Move == MoveMode::Run) {

		}
		if (Move == MoveMode::Hit) {

		}
	}

}

void Game::InputMove(int _x, int _y) {

	IsMove = true;

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
		} else {
			changeScene(State::Game); // Reload the game scene for the next stage
		}
		return; // Important: Stop further processing in InputMove after a scene change
	}
	//カメラ更新
	camera->MoveCamera(PieceSize, WallThickness, Player->GetPlayerPos());

	//ダメージ
	if (enemyHitPos != Point{ -1,-1 }) { // If player hit an enemy
		for (int i = 0; i < Enemys.size(); i++) {
			if (Enemys[i]->GetEnemyPos() == enemyHitPos) {
				Enemys[i]->Damage(Player->Attack());
				// No need to change tile on map for enemy damage/death, handled by enemy removal
			}
		}
	}

	//敵の生存確認 & remove dead enemies
	// When an enemy dies, its tile should revert to floor (0)
	// This part is tricky: BaseEnemy doesn't store its original tile type.
	// For now, we assume if an enemy was on a tile, it becomes floor (0).
	// A better way would be to have a separate grid for dynamic entities or store original tile.
	for (int i = static_cast<int>(Enemys.size()) - 1; i >= 0; --i) {
		if (Enemys[i]->GetDeath()) {
			// Optional: currentMapGrid[Enemys[i]->GetEnemyPos().y][Enemys[i]->GetEnemyPos().x] = 0; // Set tile to floor
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
			if (tileType == 0) getPaddle(x, y).rounded(3).draw(PieceColor); // Floor
			else if (tileType == 1) { // Wall
                bool shouldDrawWall = [&]() {
                    const Point DIRS[] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}}; // Cardinal directions
                    for (const auto& dir : DIRS) {
                        int nx = x + dir.x;
                        int ny = y + dir.y;

                        // Check if neighbor is within bounds
                        if (nx >= 0 && nx < currentMapGrid.width() && ny >= 0 && ny < currentMapGrid.height()) {
                            int neighborTileType = currentMapGrid[ny][nx];
                            // Check if neighbor is a passable tile type
                            if (neighborTileType == 0 || neighborTileType == 2 || neighborTileType == 4) {
                                return true; // Found a passable cardinal neighbor, so this wall should be drawn
                            }
                        }
                        // If neighbor is out of bounds, this wall is on the edge of the map and should be drawn.
                        // The new border logic ensures map edges are walls, so this case might not be
                        // strictly necessary if map edges are always walls and this culling is for internal walls.
                        // However, if a passable tile (e.g. S or G) is on the border, this wall next to it should draw.
                        // And if the map could have non-wall edges (not current case), it'd be important.
                        // For now, the logic is: draw if a cardinal neighbor is passable.
                        // If a cardinal neighbor is out-of-bounds, it's not passable, so we don't draw based on that.
                        // This means walls on the very edge of the map that don't border a passable tile internally
                        // won't be drawn by this logic. The border setting logic in GenerateAndSetupNewMap
                        // ensures the border is wall.
                        // This culling is primarily for *internal* walls that are not adjacent to passable space.
                        // A wall on the border of the grid (e.g. x=0) that has no passable neighbor at (x=1)
                        // would not be drawn by this. This is likely the desired effect: only draw walls "facing" open areas.
                    }
                    return false; // No passable cardinal neighbor found
                }(); // Immediately invoke the lambda

                if (shouldDrawWall) {
                    getPaddle(x, y).rounded(3).draw(Palette::Dimgray);
                }
            }
			else if (tileType == 2) getPaddle(x, y).rounded(3).draw(Palette::Green);    // Player Start (Changed to Green)
			// else if (tileType == 3) getPaddle(x, y).rounded(3).draw(Palette::Red); // Enemy - Enemies drawn separately
			else if (tileType == 4) getPaddle(x, y).rounded(3).draw(Palette::Yellow);  // Goal
			else { // Default for floor if enemy was there or other unknown
				getPaddle(x,y).rounded(3).draw(PieceColor);
			}
		}
	}

	// Player is drawn by its own class or needs to be drawn here
	// Player->draw(...); // Assuming BasePlayer has a draw method

	//敵の移動経路 (Enemies themselves)
	for (auto& enemy : Enemys) {
		enemy->draw(PieceSize, WallThickness, camera->GetCamera());
	}

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

		for (int y = 0; y < MapGenerator::MAP_SIZE; ++y) {
			for (int x = 0; x < MapGenerator::MAP_SIZE; ++x) {
				RectF tileRect(fullMapOffset.x + (x * FullMapTileRenderSize),
					fullMapOffset.y + (y * FullMapTileRenderSize),
					FullMapTileRenderSize, FullMapTileRenderSize);

				if (y < currentMapGrid.height() && x < currentMapGrid.width()) { // Check bounds for currentMapGrid
					int tileType = currentMapGrid[y][x];
					if (tileType == 1) { // Wall
						tileRect.draw(Palette::Dimgray);
					}
					else if (tileType == 0) { // Floor
						tileRect.draw(PieceColor);
					}
					else if (tileType == 2) { // Original Start Position tile
						tileRect.draw(Palette::Green);
					}
					else if (tileType == 4) { // Goal Position tile
						tileRect.draw(Palette::Yellow);
					}
					// Tile type 3 (enemy position) is not explicitly drawn as a tile, enemies are drawn on top
					else { // Default for any other tile type (e.g. if enemy was 3)
						tileRect.draw(PieceColor); // Default to floor color
					}
				}
				else { // If MapGenerator::MAP_SIZE is larger than currentMapGrid, draw as void/black
					tileRect.draw(Palette::Black);
				}
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

Rect Game::getPaddle(int _x, int _y)const
{
	return{ (PieceSize * _x) + (WallThickness * (_x + 1)) - camera->GetCamera().x,
			(PieceSize * _y) + (WallThickness * (_y + 1)) - camera->GetCamera().y,
			 PieceSize };
}

