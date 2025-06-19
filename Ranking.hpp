# pragma once
# include "Common.hpp"

// ランキングシーン
class Ranking : public App::Scene
{
public:

	Ranking(const InitData& init);

	void update() override;

	void draw() const override;

private:

};
