#pragma once
// Siv3Dライブラリの基本的なヘッダーファイルをインクルード
#include <Siv3D.hpp>

// マップ生成を担当するクラス
class MapGenerator
{
public:
	// --- 定数定義 ---
	// ミニマップの1辺のサイズ (例: 5x5のミニマップ)
	static constexpr int MINI_SIZE = 5;
	// フルマップの1辺のタイル数 (例: 50x50タイルのフルマップ)
	static constexpr int MAP_SIZE = 50;
	// ミニマップの1マスがフルマップ上で占めるおおよそのユニットサイズ
	// (例: ミニマップの1マスはフルマップの10x10タイルの領域に対応)
	static constexpr int ROOM_UNIT = 10;

	// --- publicメソッド ---
	// ミニマップを生成する関数
	// 戻り値: 文字の2次元配列で表現されたミニマップ (例: 'S'スタート, 'G'ゴール, 'R'部屋, 'O'空き)
	Array<Array<char>> generateMiniMap();

	// フルマップ（実際のゲームマップ）を生成する関数
	// 引数:
	//   miniMap: generateMiniMap() で生成されたミニマップのレイアウト
	// 戻り値: 整数の2次元グリッドで表現されたフルマップ (例: 0:壁, 1:床)
	Grid<int> generateFullMap(const Array<Array<char>>& miniMap);

	// --- publicメンバ変数 (生成結果を格納) ---
	// 生成されたスタートタイルの座標 (Optional: 生成されなかった場合は値なし)
	Optional<Point> startTile_generated;
	// 生成されたゴールタイルの座標 (Optional: 生成されなかった場合は値なし)
	Optional<Point> goalTile_generated;
	// 生成された各部屋の矩形領域を格納する配列
	Array<Rect> generatedRoomAreas;

private:
	// --- private構造体定義 ---
	// 各部屋の情報を格納するための内部構造体
	struct Room {
		Rect area;   // 部屋の矩形領域 (フルマップ上のタイル座標とサイズ)
		char type;   // 部屋の種類（'S':スタート, 'G':ゴール, 'R':通常の部屋）
	};
};
