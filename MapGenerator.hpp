#pragma once
#include <Siv3D.hpp>

class MapGenerator
{
public:
	static constexpr int MINI_SIZE = 5;     // ミニマップのサイズ（5x5）
	static constexpr int MAP_SIZE = 50;     // フルマップのサイズ（50x50）
	static constexpr int ROOM_UNIT = 10;    // ミニマップ1マスに対応する部屋サイズ（10x10）

	// ミニマップを生成する関数
	Array<Array<char>> generateMiniMap();

	// フルマップ（実マップ）を生成する関数
	Grid<int> generateFullMap(const Array<Array<char>>& miniMap);

	Optional<Point> startTile_generated;
	Optional<Point> goalTile_generated;
	Array<Rect> generatedRoomAreas;

private:
	// 各部屋の情報を格納する構造体
	struct Room {
		Rect area;   // 部屋の矩形領域
		char type;   // 部屋の種類（S:スタート, G:ゴール, R:通常）
	};
};

