# include "Game.hpp"

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

    // 5. Spawn Enemies (Ensure Enemys array is empty - handled by caller during stage change)
    for (int y = 0; y < MapGenerator::MAP_SIZE; ++y) {
        for (int x = 0; x < MapGenerator::MAP_SIZE; ++x) {
            if (currentMapGrid[y][x] == 0) { // Game Floor
                if (Random(0.05)) { // 5% chance to spawn an enemy
                    if (Enemys.size() < 100) { // Max 100 enemies, sanity check
                        Enemys << new BaseEnemy(Point{ x,y }, 0); // Type 0 enemy
                        // currentMapGrid[y][x] = 3; // Mark tile as enemy - NO, keep floor as 0, draw enemy on top
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
		NowStage++;

		if (NowStage >= 10) { // Example: Game ends after 10 stages
			changeScene(State::Title);
			return;
		}

		// Clear existing enemies (already handled by bug fix: pointers deleted, array cleared)
		for (auto* enemyPtr : Enemys) { // This loop should be here as per previous fix
			delete enemyPtr;
		}
		Enemys.clear();

		GenerateAndSetupNewMap(); // Generate and set up the new map

		camera->MoveCamera(PieceSize, WallThickness, Player->GetPlayerPos());
		return;
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
			else if (tileType == 1) getPaddle(x, y).rounded(3).draw(Palette::Dimgray); // Wall
			else if (tileType == 2) getPaddle(x, y).rounded(3).draw(Palette::Blue);    // Player Start
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
}

Rect Game::getPaddle(int _x, int _y)const
{
	return{ (PieceSize * _x) + (WallThickness * (_x + 1)) - camera->GetCamera().x,
			(PieceSize * _y) + (WallThickness * (_y + 1)) - camera->GetCamera().y,
			 PieceSize };
}

