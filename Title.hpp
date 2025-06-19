# pragma once
# include "Common.hpp"
#include "Save.hpp"
// タイトルシーン
class Title : public App::Scene
{
public:

	Title(const InitData& init);

	void update() override;

	void draw() const override;

private:

	RoundRect m_startButton{ Arg::center(150, 300), 200, 60, 8 };
	RoundRect m_continueButton{ Arg::center(150, 400), 200, 60, 8 };
	RoundRect m_exitButton{ Arg::center(150, 500), 200, 60, 8 };

	Transition m_startTransition{ 0.4s, 0.2s };
	Transition m_continueTransition{ 0.4s, 0.2s };
	Transition m_exitTransition{ 0.4s, 0.2s };

	//ロード画面
	Rect BaseWindow{ Arg::center(400, 300),500,400 };
	Texture icon1{ 0xF0158_icon, 50 };
	bool IsLoad = false;

	RoundRect m_save1Button{ Arg::center(400, 180), 400, 100, 8 };
	RoundRect m_save2Button{ Arg::center(400, 300), 400, 100, 8 };
	RoundRect m_save3Button{ Arg::center(400, 420), 400, 100, 8 };

	Transition m_save1Transition{ 0.4s, 0.2s };
	Transition m_save2Transition{ 0.4s, 0.2s };
	Transition m_save3Transition{ 0.4s, 0.2s };

	const short SaveDetaNum = 3;
	Array<String> LoadText;

	//キャラクターの画像
	const Texture texture[8]{
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_通常.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_笑顔.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_怒り.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_呆れ.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_苦笑い.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_驚き.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_口開き.png")).thresholded_Otsu()},
		Texture{Image(Resource(U"example/トゥマレ/トゥマレ_目閉じ.png")).thresholded_Otsu()}};

	//セーブデータ
	Array<String> SaveDeta;
	Save* save = nullptr;
};
