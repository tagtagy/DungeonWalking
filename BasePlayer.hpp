#pragma once
# include "Common.hpp"

class BasePlayer
{
public:
	BasePlayer();

	Point Move(int _x, int _y,Grid<int32>& mapData);

	//攻撃
	int Attack() { return Sterts.atc; };
	//ダメージ
	void Damage(int _damage) { Sterts.HP -= _damage; }

	StertsBase GetSterts() { return Sterts; }
	void SetPlayerPos(Point _pos) { Player = _pos; }
	Point GetPlayerPos() { return Player; }

	void draw() const;
private:
	Point Player = { 10,5 };

	StertsBase Sterts = { U"",10,5,Palette::Blue };

};

