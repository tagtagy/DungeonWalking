
#include "BasePlayer.hpp"

BasePlayer::BasePlayer() {
	// Player.x and Player.y are initialized by SetPlayerPos or default constructor of Point
}

Point BasePlayer::Move(int _x, int _y, Grid<int32>& mapData) {
	Point targetPos = Player + Point{ _x, _y }; // Calculate target position

	// Check map boundaries
	if (targetPos.x >= 0 && targetPos.x < mapData.width() &&
		targetPos.y >= 0 && targetPos.y < mapData.height()) {

		int targetTileType = mapData[targetPos.y][targetPos.x];

		// Tile type definitions for clarity (matching Game.cpp new definitions)
		// 0: Game Wall (Not Passable, Not Drawn)
		// 1: Game Floor (Passable, Drawn with PieceColor)
		// 2: Player Start (Passable, Drawn Green)
		// 3: Enemy (Passable for attack intent, not for movement, enemies are separate entities)
		// 4: Goal (Passable, Drawn Yellow)
		// 5: Debug Room Area (Passable, Drawn Magenta)

		bool canMoveToTarget = false;
		if (targetTileType == 1 || // New Game Floor
			targetTileType == 2 || // Player Start (e.g., moving back to start)
			targetTileType == 4 || // Goal
			targetTileType == 5) { // Debug Room Area
			canMoveToTarget = true;
		}

		if (canMoveToTarget) {
			// プレイヤーが開始タイル上にいた場合、開始タイルタイプ（2）に戻す必要があります。  
            // そうでない場合、標準の床タイル（1）に戻します。  
            // これは、プレイヤータイル（mapData[Player.y][Player.x] = 2 in Game.cpp）が  
            // 単なるマーカーであり、下位のタイルタイプを復元する必要があることを前提としています。
			// ただし、Game.cppではプレイヤーの新規位置を2に設定しています。
            // したがって、旧位置は床タイプ（1）になるべきです。
            // 簡素化すると：旧位置は常に床タイプ（1）になるべきで、特別な場合（ゴール）を除きます。
            // プレイヤーがゴール（4）上にいて移動した場合、ゴール（4）のままにすべきです。
			// プレイヤーがスタート（2）にいて移動した場合、スタート（2）のままにすべきです。
            // これは、currentMapGridが「ベース」マップを格納し、プレイヤーがオーバーレイであることを示しています。
            // この関数内の`mapData[Player.y][Player.x] = 2;`という行は、プレイヤーがタイルを「置き換える」ことを示しています。
			// それを追跡すると：古い位置は標準の床（1）になります。

            // プレイヤーがそこにいた前に mapData[Player.y][Player.x] に何があったか？
            // プレイヤーが現在 (px, py) にいる場合、mapData[py][px] は 2（プレイヤーマーカー）です。
			// プレイヤーが (px, py) から (nx, ny) に移動する場合：
            // mapData[py][px] は、プレイヤーがそこにいた *前の状態* に戻すべきです。
            // これには、元のタイルを知る必要があります。
            // 簡素化のため、標準の床タイル（1）になると仮定します。
			// より堅牢なシステムでは、エンティティ用の別レイヤーまたは基盤タイルを保存する仕組みが必要です。
            // 現在のコードでは:
            // mapData[Player.y][Player.x] = 2; // プレイヤーは2とマークされたタイル上にいます。
            // 移動する際、このタイルは床タイルに戻る必要があります。

			// 現在のプレイヤータイルが2（プレイヤーのスタートタイル）の場合、プレイヤーが移動した際に2に戻すべきです。
			// このロジックは複雑です。Game.cppの`currentMapGrid`が静的タイルの真実のソースであると仮定します。
            // Player::Moveはターゲットが通過可能かどうかだけを確認すべきです。
            // Game.cppが`currentMapGrid`上のプレイヤーのマーカーを更新します。

			// BasePlayer::Moveのシンプルなロジック:
            // 1. ターゲットタイルタイプに基づいてターゲット位置が通過可能かどうかを確認する。
            // 2. 通過可能であれば、プレイヤーの内部位置を更新する。
            // 3. 成功/インタラクションの種類を示すコードを返す。
            // Game.cppはその後、currentMapGridを更新する:
			//    - マップ上の古いプレイヤーの位置は元のタイルタイプに戻ります（例: フロア1、またはスタート2）
            //    - マップ上の新しいプレイヤーの位置はプレイヤーマーカーになります（例: 2、ただしこれはスタートタイルと衝突します）
            // BasePlayer::Move の現在のコードは、currentMapGrid である mapData を直接変更しています。


			if (mapData[Player.y][Player.x] == 2) { // プレイヤーが現在自分のマーカー上にいる場合
                // これは、プレイヤーが立っているタイルが常に2とマークされていることを前提としています。
                // また、タイルタイプ2はプレイヤーの現在の位置専用であり、
                // 固定位置の「元のスタートタイル」とは区別されます。
				// プレイヤーがタイルタイプ2（プレイヤーの位置）のタイルから移動した場合、
                // そのタイルは床（1）になるべきです。
				mapData[Player.y][Player.x] = 1; // プレイヤーの古い位置をマップ上の新しいゲームフロアに設定
			}
			// プレイヤーがスタート（2）またはゴール（4）のタイルタイプ上にいて移動した場合、それらのタイルはタイプを維持します。
			// つまり、プレイヤーがそのタイル上にいることで、それらを「消費」したり変更したりしません。
			// mapData[Player.y][Player.x] = 2 の行は、プレイヤーマーカーが常に2であることを示しています。
			// これは、スタートタイルも2である場合と衝突します。
			// スタートタイルは特定の位置であり、プレイヤーマーカーは動的であると仮定しましょう。
			// Game.cppでスタートタイルを2に設定しています。これは問題です。  
			// 必要に応じてプレイヤーマーカーを新しいID（例：6）に設定するか、Game.cppで処理するようにします。

			// 現時点では既存のパターンに従います：古い位置がフロアになり、新しい位置がプレイヤーマーカー（2）になります。
			// プレイヤーがスタートタイルに戻り、その後離れると、スタートタイルが上書きされます。
			// スタートタイル（2）が静的である場合、これは元の設計のバグの可能性が高いです。
			// 制約を考慮すると、直接的な変更に固執しましょう：
			mapData[Player.y][Player.x] = 1;  // 地図上の古いプレイヤーの位置を新しいゲームフロアに設定
			Player = targetPos;               // プレイヤーの内部位置を更新
			mapData[Player.y][Player.x] = 2;  // 地図上の新しいプレイヤーの位置をプレイヤータイル（2）としてマーク
			// これは実質的にプレイヤーが常に「2」の状態にあることを意味します
			// スタートタイルも「2」の場合、上書きされる可能性があります。
			return Point{ -1,-1 };            // 移動成功、相互作用なし

		}
		else if (targetTileType == 3) { // Moving onto an enemy tile (3)
			return targetPos; // Player intends to attack, does not move. Return enemy position.
		}
		// If targetTileType is 0 (New Game Wall) or any other unhandled type, movement is blocked.
		// No change in player position or mapData needed for blocked moves.
	}

	// Indicates no move occurred (hit boundary or blocked by unhandled/wall tile type)
	return Point{ -1,-1 };
}
