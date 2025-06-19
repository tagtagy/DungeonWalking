#include "stdafx.h"
#include "BasePlayer.hpp"

BasePlayer::BasePlayer() {

}

Point BasePlayer::Move(int _x, int _y, Grid<int32>& mapData) {
	//壁に当たっていない時
	if (Player.x + _x >= 0 && mapData.width() > Player.x + _x &&
		Player.y + _y >= 0 && mapData.height() > Player.y + _y) {
		//進める所だった時
		if (mapData[Player.y + _y][Player.x + _x] == 0) {

			mapData[Player.y][Player.x] = 0;
			Player += Point{ _x,  _y };
			mapData[Player.y][Player.x] = 2;

			return Point{ -1,-1 };
		}
		else if (mapData[Player.y + _y][Player.x + _x] == 3) {
			//敵が居た時
			return Point{ Player.x + _x ,Player.y + _y };
		}
		else if (mapData[Player.y + _y][Player.x + _x] == 4) {
			//ステージを進むマスだった時
			mapData[Player.y][Player.x] = 0;
			Player += Point{ _x,  _y };
			mapData[Player.y][Player.x] = 2;

			return Point{ -2,-2 };
		}
	}

	return Point{ -1,-1 };
}
