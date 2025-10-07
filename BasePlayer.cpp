
#include "BasePlayer.hpp"

BasePlayer::BasePlayer() {
	
}

Point BasePlayer::Move(int _x, int _y,const Grid<int32> mapData) {
	Point targetPos = Player + Point{ _x, _y }; // 目標位置を計算する

	// マップの外に出ないか確認
	if (targetPos.x >= 0 && targetPos.x < mapData.width() &&
		targetPos.y >= 0 && targetPos.y < mapData.height()) {

		Player = targetPos;// プレイヤーの内部位置を更新
		return Point(Player);
	}

	// 移動が発生しなかったことを示す（境界に到達したか、未処理タイル／壁タイルタイプによってブロックされた）
	return Point{ -1,-1 };
}
