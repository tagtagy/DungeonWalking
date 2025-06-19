#pragma once
# include "Common.hpp"



class EnemyDataBase {
public:
	EnemyDataBase() {

		EnemyData <<
			StertsBase{ U"A",10,3,Palette::Red },
			StertsBase{ U"B",10,3,Palette::Red };

	}

	Array<StertsBase> EnemyData;
};
