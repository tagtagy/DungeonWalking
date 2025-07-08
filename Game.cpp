# include "Game.hpp"

// 静的メンバー変数 s_currentStage の定義と初期化
// 現在のステージ番号を保持する。ゲーム開始時は0。
short Game::s_currentStage = 0;

// 新しいマップを生成し、ゲームの初期設定を行うメソッド
void Game::GenerateAndSetupNewMap() {
	// 1. MapGenerator を使用してミニマップとフルマップのレイアウトを生成する
	auto miniMap = generator.generateMiniMap(); // ミニマップを生成 (部屋の配置の概要)
	auto generatedLayout = generator.generateFullMap(miniMap); // ミニマップに基づいてフルマップを生成 (Grid<int>形式)

	// 2. 現在のゲームマップ (currentMapGrid) を初期化する
	// MapGenerator で定義されたマップサイズでリサイズする
	currentMapGrid.resize(MapGenerator::MAP_SIZE, MapGenerator::MAP_SIZE);

	// 3. 生成されたマップレイアウトを currentMapGrid に適用し、スタート(S)とゴール(G)のタイルを特定する
	// MapGenerator がスタート地点またはゴール地点を正常に設定できなかった場合のフォールバック処理
	if (!generator.startTile_generated.has_value() || !generator.goalTile_generated.has_value()) {
		Console << U"エラー: MapGenerator がスタートまたはゴールタイルを設定しませんでした。";
		// フォールバックとして、非常にシンプルなマップを作成する
		currentMapGrid.assign(MapGenerator::MAP_SIZE, MapGenerator::MAP_SIZE, 1); // 全て壁 (タイルID:1)
		currentMapGrid[1][1] = 2; // プレイヤーの開始位置 (タイルID:2)
		currentMapGrid[1][2] = 4; // ゴール位置 (タイルID:4)
		Player->SetPlayerPos(Point{ 1,1 }); // プレイヤーの初期位置を設定
		// 既存の敵がいれば全て削除 (リトライシナリオなどを想定)
		for (auto* enemy : Enemys) { delete enemy; }
		Enemys.clear();
		return; // マップ生成処理を終了
	}

	// MapGenerator から取得したプレイヤーの開始位置とゴール位置
	Point playerStartPos = generator.startTile_generated.value();
	Point goalPos = generator.goalTile_generated.value();

	// マップ全体を走査し、各タイルの種類を設定
	for (int y = 0; y < MapGenerator::MAP_SIZE; ++y) {
		for (int x = 0; x < MapGenerator::MAP_SIZE; ++x) {
			if (Point(x, y) == playerStartPos) {
				currentMapGrid[y][x] = 2; // プレイヤースタート地点 (タイルID:2)
			}
			else if (Point(x, y) == goalPos) {
				currentMapGrid[y][x] = 4; // ゴール地点 (通行可能、黄色で描画、タイルID:4)
			}
			else if (generatedLayout[y][x] == 0) { // MapGenerator が生成した壁
				currentMapGrid[y][x] = 0; // ゲーム内の壁 (通行不可、描画なし、タイルID:0)
			}
			else if (generatedLayout[y][x] == 1) { // MapGenerator が生成した床/道
				currentMapGrid[y][x] = 1; // ゲーム内の床 (通行可能、PieceColorで描画、タイルID:1)
			}
			else { // 想定外のタイルIDの場合 (発生すべきではない)
				currentMapGrid[y][x] = 0; // デフォルトでゲーム内の壁として扱う
			}
		}
	}

	// 4. プレイヤーの初期位置を設定する
	Player->SetPlayerPos(playerStartPos);
	// プレイヤーの開始タイルがデバッグ用の描画で上書きされないように、再度プレイヤースタートとしてマークする
	currentMapGrid[playerStartPos.y][playerStartPos.x] = 2;


	// --- デバッグ用: 生成された部屋エリアを視覚化するためにマークする ---
	const int DEBUG_ROOM_TILE_ID = 5; // 通行可能、マゼンタ色で描画されるデバッグ用タイルID
	if (not generator.generatedRoomAreas.isEmpty()) {
		for (const auto& roomAreaRect : generator.generatedRoomAreas) {
			for (int y_room = roomAreaRect.y; y_room < roomAreaRect.y + roomAreaRect.h; ++y_room) {
				for (int x_room = roomAreaRect.x; x_room < roomAreaRect.x + roomAreaRect.w; ++x_room) {
					// 座標がマップ範囲内にあるか確認
					if (InRange(x_room, 0, MapGenerator::MAP_SIZE - 1) && InRange(y_room, 0, MapGenerator::MAP_SIZE - 1)) {
						// MapGenerator によって定義されたエリアを部屋としてマークする。
						// 本来はゲーム内の床(1)であるべきだが、デバッグ目的で5を使用。
						// スタート(2)またはゴール(4)タイルを上書きしないようにする。
						if (currentMapGrid[y_room][x_room] != 2 && currentMapGrid[y_room][x_room] != 4) {
							currentMapGrid[y_room][x_room] = DEBUG_ROOM_TILE_ID;
						}
					}
				}
			}
		}
	}
	// --- デバッグ用処理ここまで ---


	// マップの境界線を壁にする処理は削除 (ユーザーの要望により壁をなくしたため)
	// int gridWidth = currentMapGrid.width();
	// int gridHeight = currentMapGrid.height();
	// if (gridWidth > 0 && gridHeight > 0) { ... } // 境界壁コードは削除済み

	// 5. 敵をスポーンする
	//    - 敵の配列 (Enemys) が空であることを前提とする (新しいゲームインスタンス作成前にデストラクタで処理される想定)
	//    - スタート(S)とゴール(G)のタイル位置を取得し、それらの部屋には敵をスポーンしないようにする。
	Optional<Point> startTileOpt = generator.startTile_generated; // MapGeneratorが設定したスタートタイル
	Optional<Point> goalTileOpt = generator.goalTile_generated;   // MapGeneratorが設定したゴールタイル

	for (const auto& roomAreaRect : generator.generatedRoomAreas) {
		// 現在の部屋 (roomAreaRect) がスタート部屋またはゴール部屋かどうかを判定
		bool isStartRoom = false;
		if (startTileOpt.has_value() && roomAreaRect.contains(startTileOpt.value())) {
			isStartRoom = true;
		}

		bool isGoalRoom = false;
		if (goalTileOpt.has_value() && roomAreaRect.contains(goalTileOpt.value())) {
			isGoalRoom = true;
		}

		// スタート部屋またはゴール部屋の場合、敵のスポーンをスキップ
		if (isStartRoom || isGoalRoom) {
			continue;
		}

		// 部屋の面積 (部屋の矩形の幅 × 高さ) に基づいてスポーンする敵の数を決定
		int roomTileArea = roomAreaRect.w * roomAreaRect.h;
		// スポーンルール例: エリアの25タイルごとに1体の敵、1部屋あたり最大3体まで。最小0体。
		int numEnemiesToSpawn = Clamp(roomTileArea / 25, 0, 3);

		for (int i = 0; i < numEnemiesToSpawn; ++i) {
			// 部屋内で有効なスポーンポイントを見つけるために、限られた回数試行する
			for (int attempt = 0; attempt < 10; ++attempt) {
				// 部屋の範囲内でランダムな座標を生成
				int spawnX = Random(roomAreaRect.x, roomAreaRect.x + roomAreaRect.w - 1);
				int spawnY = Random(roomAreaRect.y, roomAreaRect.y + roomAreaRect.h - 1);
				Point spawnPos(spawnX, spawnY);

				// ランダムに選択された位置がマップグリッドの範囲内であり、
				// かつ、ゲーム内の床タイル(1)またはデバッグ用部屋タイル(5)であるかを確認
				if (s3d::InRange(spawnPos.x, 0, static_cast<int>(currentMapGrid.width() - 1)) &&
					s3d::InRange(spawnPos.y, 0, static_cast<int>(currentMapGrid.height() - 1))) {

					int tileAtSpawn = currentMapGrid[spawnPos.y][spawnPos.x];
					if (tileAtSpawn == 1 || tileAtSpawn == 5) { // ゲーム内の床(1) または デバッグ用部屋エリア(5)
						Enemys << new BaseEnemy(spawnPos, 0); // 新しい敵 (タイプ0) を作成して Enemys 配列に追加
						// 注意: currentMapGrid[spawnPos.y][spawnPos.x] を敵タイル(3など)に変更しない。
						// 敵の位置は Enemys 配列で追跡される。
						break; // 敵1体のスポーンに成功。次の敵のスポーン処理へ (またはループ終了)。
					}
				}
			}
		}
	}
}

// Gameクラスのコンストラクタ
Game::Game(const InitData& init)
	: IScene{ init } // 親クラス IScene のコンストラクタを呼び出す
{
	Player = new BasePlayer; // プレイヤーオブジェクトを生成
	GenerateAndSetupNewMap(); // 最初のマップを生成・設定

	// カメラの初期位置を設定
	// Player->GetPlayerPos() は GenerateAndSetupNewMap() によって設定済み
	// プレイヤーのグリッド座標をピクセル座標に変換
	Point playerPixelPos = { (PieceSize * Player->GetPlayerPos().x) + (WallThickness * (Player->GetPlayerPos().x + 1)),
							 (PieceSize * Player->GetPlayerPos().y) + (WallThickness * (Player->GetPlayerPos().y + 1)) };
	// カメラの初期位置を計算 (プレイヤーが画面中央やや左上に来るように調整)
	camera = new Camera(playerPixelPos - Point((800 - 150) / 2, (600 - 150) / 2));

	// ヒットエフェクトの初期化
	m_hitEffects.setSpeed(2.0);       // パーティクルの速度
	m_hitEffects.setMaxLifeTime(0.3s); // パーティクルの最大生存時間
}

// Gameクラスのデストラクタ
Game::~Game() {
	// 動的に確保したメモリを解放
	delete Player;
	Player = nullptr;

	delete camera;
	camera = nullptr;

	for (auto* enemy : Enemys) {
		delete enemy;
	}
	// Enemys.clear(); // s3d::Array は自動的に自身をクリアするので不要
}

// 毎フレームの更新処理
void Game::update()
{
	// --- 連続移動ロジック ---
	Point moveInput(0, 0); // 現在のフレームでの移動入力方向
	bool newKeyPressedThisFrame = false; // このフレームで新しい移動キーが押されたか

	// テンキーによる移動入力の検出
	if (KeyNum1.pressed()) moveInput.set(-1, 1); // 左下
	else if (KeyNum2.pressed()) moveInput.set(0, 1);  // 下
	else if (KeyNum3.pressed()) moveInput.set(1, 1);  // 右下
	else if (KeyNum4.pressed()) moveInput.set(-1, 0); // 左
	else if (KeyNum6.pressed()) moveInput.set(1, 0);  // 右
	else if (KeyNum7.pressed()) moveInput.set(-1, -1);// 左上
	else if (KeyNum8.pressed()) moveInput.set(0, -1); // 上
	else if (KeyNum9.pressed()) moveInput.set(1, -1); // 右上

	// 移動入力があった場合
	if (moveInput != Point(0, 0)) {
		// 前回ホールドされていた方向と異なるか、または初めてキーが押された場合
		if (!m_heldMoveDirection.has_value() || m_heldMoveDirection.value() != moveInput) {
			newKeyPressedThisFrame = true; // 新しいキー入力としてマーク
			m_isAttackIntent = true;      // 攻撃の意図ありと見なす (初回移動は攻撃の可能性があるため)
			InputMove(moveInput.x, moveInput.y); // 移動処理を実行
			m_heldMoveDirection = moveInput;     // ホールドされている方向を更新
			m_isWaitingForInitialRepeat = true; // 初回リピート待ち状態に設定
			m_initialMoveDelayTimer.restart();  // 初回リピート遅延タイマーを開始
		}
		else { // 同じ方向のキーがホールドされ続けている場合
			m_isAttackIntent = false; // リピート移動は攻撃の意図なしと見なす
			// 初回リピート待ち状態の場合
			if (m_isWaitingForInitialRepeat) {
				if (m_initialMoveDelayTimer.reachedZero()) { // 遅延タイマーが0になったら
					m_isWaitingForInitialRepeat = false;    // 初回リピート待ち状態を解除
					m_moveRepeatTimer.restart();            // リピートタイマーを開始
					InputMove(moveInput.x, moveInput.y);    // 移動処理を実行
				}
			}
			// 通常のリピート状態の場合
			else if (m_moveRepeatTimer.reachedZero()) { // リピートタイマーが0になったら
				m_moveRepeatTimer.restart();            // リピートタイマーを再開
				InputMove(moveInput.x, moveInput.y);    // 移動処理を実行
			}
		}
	}
	else { // 移動入力がない場合
		if (m_heldMoveDirection.has_value()) { // 直前まで移動していた場合
			m_isAttackIntent = false; // 攻撃の意図をリセット
		}
		m_heldMoveDirection.reset();        // ホールド方向をリセット
		m_initialMoveDelayTimer.pause();    // 遅延タイマーを一時停止
		m_moveRepeatTimer.pause();          // リピートタイマーを一時停止
		m_isWaitingForInitialRepeat = false;// 初回リピート待ち状態を解除
		// m_isAttackIntent はキーが押されていないか離された場合、リセット/falseになる
	}

	// テンキー5が押された場合 (待機コマンド、単発トリガー)
	if (KeyNum5.down()) {
		m_isAttackIntent = true;  // 攻撃の意図あり (待機も行動の一種)
		InputMove(0, 0);         // 移動量0で InputMove を呼び出し、ターンを進める
		m_isAttackIntent = false; // 単発アクションなので意図をリセット
	}

	// このフレームで新しいキーによる移動が開始されなかった場合、
	// 後続のロジックのために攻撃の意図をfalseにしておく
	if (!newKeyPressedThisFrame && moveInput == Point(0, 0) && !KeyNum5.down()) {
		m_isAttackIntent = false;
	}

	// Mキーが押されたらフルマップ表示を切り替える
	if (KeyM.down()) {
		showFullMap = !showFullMap;
	}

	// --- カメラシェイク処理 ---
	if (m_cameraShakeTimer.isRunning()) { // シェイクタイマーが作動中の場合
		if (m_cameraShakeTimer.reachedZero()) { // タイマーが0になったら
			m_cameraShakeOffset.reset(); // シェイクオフセットをリセット
		}
		else {
			m_cameraShakeOffset = s3d::RandomVec2() * 3.0; // ランダムな方向に3.0の強さでオフセットを生成
		}
	}
	else {
		m_cameraShakeOffset.reset(); // タイマーが作動していなければオフセットをクリア
	}

	m_hitEffects.update(); // パーティクルエフェクトを更新

	// プレイヤーのランジ（突進）アニメーションの視覚的オフセット計算 (draw()で使うための準備)
	Vec2 lungeVisualOffset = Vec2::Zero(); // このローカル変数はここでは直接使用されない。描画時のオフセットはdraw()内で計算される。
	if (m_playerLungeDirection.has_value() && m_playerLungeTimer.isRunning()) {
		if (m_playerLungeTimer.reachedZero()) {
			m_playerLungeDirection.reset(); // タイマー終了でランジ方向をリセット
		}
		else {
			// ランジの進行度 (0.0 -> 1.0)
			double progress = m_playerLungeTimer.progress0_1();
			// Sinカーブで滑らかな動き (0 -> 1 -> 0) を作る
			double lungeAmplitude = Sin(progress * Math::Pi);
			// ランジの距離 (1マスの30%)
			double lungeDistance = static_cast<double>(PieceSize) * 0.3;
			// 視覚的なオフセットを計算
			lungeVisualOffset = m_playerLungeDirection.value() * lungeAmplitude * lungeDistance;
		}
	}
	else if (m_playerLungeDirection.has_value()) { // タイマーが停止したが方向が残っている場合 (フレーム間のクリーンアップ)
		m_playerLungeDirection.reset();
	}
}

// プレイヤーの移動入力とそれに伴う処理を行うメソッド
void Game::InputMove(int _x, int _y) {

	// 現在の敵の位置リスト (BasePlayer::Move で使用される可能性があるため事前に収集)
	Array<Point>enemyPoss;
	for (auto& enemy : Enemys) {
		enemyPoss << enemy->GetEnemyPos();
	}

	// --- プレイヤーの移動処理 ---
	// Player->Move を呼び出し、移動先が敵だった場合はその敵の座標を返す
	Point enemyHitPos = Player->Move(_x, _y, currentMapGrid);

	// --- ゴール判定 ---
	// プレイヤーがゴールタイル (ID:4) に到達したか確認
	if (currentMapGrid[Player->GetPlayerPos().y][Player->GetPlayerPos().x] == 4) {
		Game::s_currentStage++; // 現在のステージ番号をインクリメント
		const int MAX_STAGES = 10; // 最大ステージ数を定義

		if (Game::s_currentStage >= MAX_STAGES) { // 最大ステージ数に達したら
			Game::s_currentStage = 0; // ステージ番号をリセット
			changeScene(State::Title); // タイトルシーンへ遷移
		}
		else {
			changeScene(State::Game); // 次のステージのゲームシーンを再読み込み
		}
		return; // シーン変更後は InputMove 内の以降の処理を中断する
	}

	// --- カメラ更新 ---
	camera->MoveCamera(PieceSize, WallThickness, Player->GetPlayerPos());

	// --- プレイヤーの攻撃処理 (移動先に敵がいた場合) ---
	if (enemyHitPos != Point{ -1,-1 }) { // Player->Move が敵の座標を返した場合（つまり、移動しようとしたマスに敵がいた場合）
		// 攻撃の意図の有無に関わらず、連続移動は停止させる
		m_heldMoveDirection.reset();
		m_initialMoveDelayTimer.pause();
		m_moveRepeatTimer.pause();
		m_isWaitingForInitialRepeat = false;

		// bool attackOccurredThisTurn = false; // この変数は現在使用されていないためコメントアウトまたは削除を検討

		if (m_isAttackIntent) { // 攻撃の意図があった場合のみ、攻撃を実行
			for (int i = 0; i < Enemys.size(); i++) {
				if (Enemys[i] && Enemys[i]->GetEnemyPos() == enemyHitPos) { // 衝突した敵オブジェクトを特定
					Enemys[i]->Damage(Player->Attack()); // プレイヤーの攻撃力で敵にダメージを与える
					m_cameraShakeTimer.restart();       // カメラシェイクエフェクトを開始/再開
					// attackOccurredThisTurn = true; // attackOccurredThisTurn を使用する場合はここでtrueに設定

					// ヒットエフェクトやプレイヤーのランジ（突進）アニメーションを設定
					// Point enemyGridPos = Enemys[i]->GetEnemyPos(); // 必要であれば使用
					// Vec2 enemyCenterPixelPos = Vec2{...}; // 必要であれば使用

					Vec2 lungeDir = (enemyHitPos - Player->GetPlayerPos()); // プレイヤーから敵への方向ベクトルを計算
					if (lungeDir.lengthSq() > 0) { // ベクトルの長さが0より大きければ正規化
						lungeDir.normalize();
					} else { // まれに同じタイルにいる場合などは、デフォルト方向（右）を設定
						lungeDir = Vec2{ 1,0 };
					}
					m_playerLungeDirection = lungeDir; // プレイヤーのランジ方向を設定
					m_playerLungeTimer.restart();      // ランジアニメーション用のタイマーを開始
					// 敵にダメージを与えたり倒したりしても、マップタイル自体を変更する必要はない（敵の削除処理で対応）
					break; // 攻撃対象の敵への処理が完了したので、敵ループを抜ける
				}
			}
		}

		// 攻撃の意図があった場合（実際に攻撃がヒットしたか、空振りしたかに関わらず）、
		// この行動（移動または攻撃の試み）の後に攻撃意図フラグをリセットする
		if (m_isAttackIntent) {
			m_isAttackIntent = false;
		}
	} // 敵への攻撃処理ここまで

	// --- 敵の生存確認と死亡処理 ---
	// 敵が死んだとき、そのタイルは床(1)に戻るべき。
	// BaseEnemy は元のタイルの種類を保存していないため、ここでは敵がいたタイルは床(1)になると仮定。
	// (より良い方法は、動的エンティティ用に別のグリッドを持つか、元のタイルを保存すること)
	for (int i = static_cast<int>(Enemys.size()) - 1; i >= 0; --i) { // 配列の末尾から削除するため逆順でループ
		if (Enemys[i]->GetDeath()) { // 敵が死亡している場合
			Point deadEnemyPos = Enemys[i]->GetEnemyPos(); // 死亡した敵の座標
			// グリッド範囲内であることを確認してからタイルを変更
			if (s3d::InRange(deadEnemyPos.x, 0, static_cast<int>(currentMapGrid.width() - 1)) &&
				s3d::InRange(deadEnemyPos.y, 0, static_cast<int>(currentMapGrid.height() - 1))) {
				currentMapGrid[deadEnemyPos.y][deadEnemyPos.x] = 1; // タイルを新しいゲーム内の床ID(1)に設定
			}
			delete Enemys[i];        // 敵オブジェクトを削除
			Enemys.remove_at(i); // 配列から削除
		}
	}
	// Enemys.remove_if([](BaseEnemy* x) {bool isDead = x->GetDeath(); if(isDead) delete x; return isDead; }); // ラムダ式を使った別解

	// --- 敵の移動と攻撃処理 ---
	for (auto& currentEnemy : Enemys) { // 生き残っている各敵について処理
		// 敵の移動処理 (プレイヤーの位置をターゲットとし、currentMapGrid を参照)
		// 移動先にプレイヤーがいた場合、プレイヤーにダメージを与える
		Player->Damage(currentEnemy->Move(Player->GetPlayerPos(), currentMapGrid));
	}
}

// void Game::Map() { // 指示により削除された古いマップ処理メソッド
// }

// 描画処理
void Game::draw() const
{
	Scene::SetBackground(ColorF{ 0.2 }); // 背景色を設定

	if (currentMapGrid.isEmpty()) return; // マップが空の場合は描画しない

	// --- ステージプレーン (マップタイル) の描画 ---
	for (int y = 0; y < currentMapGrid.height(); y++) {
		for (int x = 0; x < currentMapGrid.width(); x++) {
			int tileType = currentMapGrid[y][x]; // タイルの種類を取得
			if (tileType == 0) { // 新しいゲーム内の壁 (タイルID:0) - 描画しない
				// 描画処理なし
			}
			else if (tileType == 1) { // 新しいゲーム内の床 (タイルID:1)
				getPaddle(x, y).rounded(3).draw(PieceColor); // PieceColorで角丸矩形を描画
			}
			else if (tileType == 2) { // プレイヤースタートタイル (タイルID:2)
				getPaddle(x, y).rounded(3).draw(Palette::Green); // 緑色で描画
			}
			else if (tileType == 4) { // ゴールタイル (タイルID:4)
				getPaddle(x, y).rounded(3).draw(Palette::Yellow); // 黄色で描画
			}
			else if (tileType == 5) { // デバッグ用部屋タイル (タイルID:5)
				getPaddle(x, y).rounded(3).draw(Palette::Magenta); // マゼンタ色で描画
			}
			// 想定外のタイルIDの場合は描画されない
		}
	}

	// --- プレイヤーの描画 ---
	if (Player) { // プレイヤーオブジェクトが存在する場合
		Point playerGridPos = Player->GetPlayerPos(); // プレイヤーのグリッド座標
		RectF playerBodyBase = getPaddle(playerGridPos.x, playerGridPos.y); // 基本となる描画矩形

		// ランジ（突進）アニメーションの視覚的オフセットを計算
		Vec2 lungeVisualOffsetDraw = Vec2::Zero();
		if (m_playerLungeDirection.has_value() && m_playerLungeTimer.isRunning()) {
			double progress = m_playerLungeTimer.progress0_1();
			double lungeAmplitude = Sin(progress * Math::Pi); // 0 -> 1 -> 0 の滑らかなカーブ
			double lungeDistance = static_cast<double>(PieceSize) * 0.3; // ランジ距離
			lungeVisualOffsetDraw = m_playerLungeDirection.value() * lungeAmplitude * lungeDistance;
		}

		// オフセットを適用したプレイヤーの描画矩形
		RectF playerVisualRect = playerBodyBase.movedBy(lungeVisualOffsetDraw);
		playerVisualRect.draw(Player->GetSterts().color); // プレイヤーの色で描画
	}

	// --- 敵の描画 ---
	// カメラシェイクを考慮したカメラ位置を計算
	s3d::Vec2 currentShakeVec = m_cameraShakeOffset.value_or(s3d::Vec2::Zero());
	s3d::Point effectiveCameraPointForEnemies = camera->GetCamera() - currentShakeVec.asPoint();

	for (auto& enemy : Enemys) {
		if (enemy) { // 敵オブジェクトが有効な場合
			// 敵自身の描画メソッドを呼び出す (ピースサイズ、壁の厚さ、有効カメラ位置を渡す)
			enemy->draw(PieceSize, WallThickness, effectiveCameraPointForEnemies);
		}
	}

	// m_hitEffects.update() は Game::update() 内で既に呼ばれているため、ここでは不要。
	// Siv3D の Effect システムは通常、.update() 呼び出し後に自動で描画を処理する。

	// --- UIの描画 ---
	{
		// メッセージウィンドウ
		MessageWindow.draw(Palette::Black).drawFrame(2, Palette::White);
		// ミニメッセージウィンドウ
		MiniMessageWindow.draw(Palette::Black).drawFrame(2, Palette::White);
		// キャラクターウィンドウとキャラクターテクスチャ
		CharaWindow.draw(Palette::Black).drawFrame(2, Palette::White);
		texture[0](250, 0, 300, 400).fitted(CharaWindow.size).drawAt(CharaWindow.center().x, 350);
	}

	// --- フルマップの描画 (showFullMap が true の場合) ---
	if (showFullMap) {
		const Point fullMapOffset(10, 10); // 画面端からのオフセット

		// オプション: フルマップパネルの半透明背景を描画
		Rect((fullMapOffset.x - 2), (fullMapOffset.y - 2),
			(MapGenerator::MAP_SIZE * FullMapTileRenderSize) + 4,
			(MapGenerator::MAP_SIZE * FullMapTileRenderSize) + 4)
			.draw(ColorF(0.1, 0.1, 0.1, 0.8));

		// マップタイルを描画
		for (int y_map = 0; y_map < MapGenerator::MAP_SIZE; ++y_map) {
			for (int x_map = 0; x_map < MapGenerator::MAP_SIZE; ++x_map) {
				RectF tileRect(fullMapOffset.x + (x_map * FullMapTileRenderSize),
					fullMapOffset.y + (y_map * FullMapTileRenderSize),
					FullMapTileRenderSize, FullMapTileRenderSize);

				if (y_map < currentMapGrid.height() && x_map < currentMapGrid.width()) { // グリッド範囲内か確認
					int tileTypeOnGrid = currentMapGrid[y_map][x_map];
					if (tileTypeOnGrid == 0) { // 壁 (描画なし)
						// tileRect.draw(Palette::Black); // 必要なら黒などで「何もない」ことを示す
					}
					else if (tileTypeOnGrid == 1) { // 床
						tileRect.draw(PieceColor);
					}
					else if (tileTypeOnGrid == 2) { // スタート地点
						tileRect.draw(Palette::Green);
					}
					else if (tileTypeOnGrid == 4) { // ゴール地点
						tileRect.draw(Palette::Yellow);
					}
					else if (tileTypeOnGrid == 5) { // デバッグ用部屋
						tileRect.draw(Palette::Magenta);
					}
					// その他のタイルタイプはフルマップには描画しない
				}
				else {
					tileRect.draw(Palette::Black); // currentMapGrid の範囲外
				}
			}
		} // マップタイルの描画ここまで

		// フルマップ上にプレイヤーを描画
		if (Player) {
			RectF playerMapRect(fullMapOffset.x + (Player->GetPlayerPos().x * FullMapTileRenderSize),
				fullMapOffset.y + (Player->GetPlayerPos().y * FullMapTileRenderSize),
				FullMapTileRenderSize, FullMapTileRenderSize);
			playerMapRect.draw(Palette::Cyan); // シアン色でプレイヤーを表現
		}

		// フルマップ上に敵を描画
		for (const auto& enemy : Enemys) {
			if (enemy) {
				RectF enemyMapRect(fullMapOffset.x + (enemy->GetEnemyPos().x * FullMapTileRenderSize),
					fullMapOffset.y + (enemy->GetEnemyPos().y * FullMapTileRenderSize), // 修正: enemy->GetPlayerPos() -> enemy->GetEnemyPos()
					FullMapTileRenderSize, FullMapTileRenderSize);
				enemyMapRect.draw(Palette::Red); // 赤色で敵を表現
			}
		}
	} // フルマップ描画ここまで

	// 以前の draw() にあったプレイヤー/敵の重複描画ロジックは削除済み。
}

// 指定されたグリッド座標 (_x, _y) に対応する描画用の矩形領域 (RectF) を返すヘルパーメソッド
// カメラの位置とシェイクを考慮する
RectF Game::getPaddle(int _x, int _y) const {
	// 現在のカメラシェイクオフセットを取得 (なければ Vec2::Zero())
	s3d::Vec2 currentShake = m_cameraShakeOffset.value_or(s3d::Vec2::Zero());
	// 基本となるカメラ位置を取得 (camera->GetCamera() は Point を返す)
	s3d::Vec2 baseCameraPos = camera->GetCamera();
	// シェイクを適用した有効なカメラ位置
	s3d::Vec2 effectiveCameraPos = baseCameraPos - currentShake;

	// グリッド座標、ピースサイズ、壁の厚さ、カメラ位置から描画矩形を計算
	return RectF{ (PieceSize * _x) + (WallThickness * (_x + 1)) - effectiveCameraPos.x,
				  (PieceSize * _y) + (WallThickness * (_y + 1)) - effectiveCameraPos.y,
				  static_cast<double>(PieceSize) }; // PieceSize は int なので、RectF のために double にキャスト
}
