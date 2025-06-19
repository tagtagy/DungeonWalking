# include "Game.hpp"

Game::Game(const InitData& init)
	: IScene{ init }
{
	Player = new BasePlayer;

	//敵味方の初期位置
	for (int y = 0; y < RoomMap[NowStage].height(); y++) {
		for (int x = 0; x < RoomMap[NowStage].width(); x++) {
			if (RoomMap[NowStage][y][x] == 2) {
				Player->SetPlayerPos(Point{ x,y });
			}
			else if (RoomMap[NowStage][y][x] == 3)Enemys << new BaseEnemy(Point{ x,y }, 0);
		}
	}


	Point pos = { (PieceSize * Player->GetPlayerPos().x) + (WallThickness * (Player->GetPlayerPos().x + 1)),
				  (PieceSize * Player->GetPlayerPos().y) + (WallThickness * (Player->GetPlayerPos().y + 1))};

	//カメラの初期位置
	camera = new Camera(pos - Point((800 - 150) / 2, (600 - 150) / 2));



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
	Point enemy = Player->Move(_x, _y, RoomMap[NowStage]);

	//プレイヤーがマップを進めるマスにいるか
	if (enemy.x == -2) {

		if (NowStage + 1 >= RoomMap.size()) {
			changeScene(State::Title);
			return;
		}
		else {
			//ステージ更新
			NowStage++;
			//敵味方の初期位置
			for (int y = 0; y < RoomMap[NowStage].height(); y++) {
				for (int x = 0; x < RoomMap[NowStage].width(); x++) {
					if (RoomMap[NowStage][y][x] == 2)Player->SetPlayerPos(Point{ x,y });
					else if (RoomMap[NowStage][y][x] == 3)Enemys << new BaseEnemy(Point{ x,y }, 0);
				}
			}
			//カメラ更新
			camera->MoveCamera(PieceSize, WallThickness, Player->GetPlayerPos());
			return;
		}
	}
	//カメラ更新
	camera->MoveCamera(PieceSize, WallThickness, Player->GetPlayerPos());

	//ダメージ
	if (enemy != Point{ -1,-1 }) {
		for (int i = 0; i < Enemys.size(); i++) {
			if (Enemys[i]->GetEnemyPos() == enemy)Enemys[i]->Damage(Player->Attack());
		}
	}

	//敵の生存確認
	for (int i = 0; i < Enemys.size(); i++)
	{
		if (Enemys[i]->GetDeath())
		{
			RoomMap[NowStage][Enemys[i]->GetEnemyPos().y][Enemys[i]->GetEnemyPos().x] = 0;
		}
	}
	Enemys.remove_if([](BaseEnemy* x) {return x->GetDeath(); });

	//エネミー移動と攻撃
	for (auto& enemy : Enemys) {
		Player->Damage(enemy->Move(Player->GetPlayerPos(), RoomMap[NowStage]));
	}
}

void Game::Map() {
	auto miniMap = generator.generateMiniMap();           // ミニマップ生成
	auto fullMap = generator.generateFullMap(miniMap);    // 実マップ生成

	for (int y = 0; y < MapGenerator::MAP_SIZE; ++y)
		for (int x = 0; x < MapGenerator::MAP_SIZE; ++x)
			if (fullMap[y][x] == 1)
				Rect(x * 10, y * 10, 10, 10).draw(ColorF(0.3));
}

void Game::draw() const
{
	Scene::SetBackground(ColorF{ 0.2 });

	//ステージプレーン
	for (int x = 0; x < RoomMap[NowStage].width(); x++) {
		for (int y = 0; y < RoomMap[NowStage].height(); y++) {

			if (RoomMap[NowStage][y][x] == 0)getPaddle(x, y).rounded(3).draw(PieceColor);
			else if (RoomMap[NowStage][y][x] == 1)getPaddle(x, y).rounded(3).draw(Palette::Dimgray);
			else if (RoomMap[NowStage][y][x] == 2)getPaddle(x, y).rounded(3).draw(Palette::Blue);
			else if (RoomMap[NowStage][y][x] == 3)getPaddle(x, y).rounded(3).draw(Palette::Red);
			else if (RoomMap[NowStage][y][x] == 4)getPaddle(x, y).rounded(3).draw(Palette::Yellow);
		}
	}

	//敵の移動経路
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

