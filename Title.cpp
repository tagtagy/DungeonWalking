# include "Title.hpp"

Title::Title(const InitData& init)
	: IScene{ init }
{
	save = new Save;
	LoadText.resize(SaveDetaNum);
	SaveDeta.resize(SaveDetaNum);
	//セーブデータのロード
	for (int i = 0; i < LoadText.size(); i++) {
		//セーブデータの名前
		TextReader reader{ U"example/Save" + Format(i) + U".txt" };
		//セーブデータの読み込み
		String text = reader.readAll();

		//セーブデータがない時
		if (text == U"")LoadText[i] = U"セーブデータがありません";
		else
		{	//セーブデータがあるとき
			//デーブデータの復号
			SaveDeta[i] = save->InverseAesEncryption(text, U"bbbbbbbbbbbbbbbb");
			
			//セーブ情報の表示
			LoadText[i] = U"Lv:" + SaveDeta[i];
		}
	}
	/*
	// セーブ用のテキストファイルをオープンする
	TextWriter writer{ U"example/Save0.txt" };

	// ファイルをオープンできなかったら
	if (not writer)
	{
		// 例外を投げて終了する
		throw Error{ U"Failed to open `test.txt`" };
	}
	writer.writeln(save->AesEncryption(U"13", U"bbbbbbbbbbbbbbbb"));*/
}

void Title::update()
{
	if (!IsLoad) {
		// ボタンの更新
		{
			m_startTransition.update(m_startButton.mouseOver());
			m_continueTransition.update(m_continueButton.mouseOver());
			m_exitTransition.update(m_exitButton.mouseOver());

			if (m_startButton.mouseOver() || m_continueButton.mouseOver() || m_exitButton.mouseOver())
			{
				Cursor::RequestStyle(CursorStyle::Hand);
			}
		}

		// ボタンのクリック処理
		if (m_startButton.leftClicked()) // ゲームへ
		{
			changeScene(State::Game);
		}
		else if (m_continueButton.leftClicked()) // ランキングへ
		{
			IsLoad = true;
		}
		else if (m_exitButton.leftClicked()) // 終了
		{
			System::Exit();
		}
	}
	else if (IsLoad) {

		m_save1Transition.update(m_save1Button.mouseOver());
		m_save2Transition.update(m_save2Button.mouseOver());
		m_save3Transition.update(m_save3Button.mouseOver());

		//マウスの変換
		if (m_save1Button.mouseOver() || m_save2Button.mouseOver() || m_save3Button.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);
		}

		if (!BaseWindow.mouseOver() && MouseL.down())IsLoad = false;
		Rect SaveButton{ Arg::center(650, 100), 50, 50 };
		if(SaveButton.leftClicked())IsLoad = false;

		//ロードボタン
		if (m_save1Button.leftClicked()) {
			//データの入力
			getData().Lv = atoi(SaveDeta[0].narrow().c_str());
			changeScene(State::Game);
		}
		if (m_save2Button.leftClicked()) {
			//データの入力
			getData().Lv = atoi(SaveDeta[1].narrow().c_str());
			changeScene(State::Game);
		}
		if (m_save3Button.leftClicked()) {
			//データの入力
			getData().Lv = atoi(SaveDeta[2].narrow().c_str());
			changeScene(State::Game);
		}
	}
}

void Title::draw() const
{
	Scene::SetBackground(ColorF{ 0 });

	// タイトル描画
	FontAsset(U"TitleFont")(U"DungeonWalk")
		.draw(TextStyle::OutlineShadow(0.2, ColorF{ 0.2 }, Vec2{ 3, 3 }, ColorF{ 0.5 }), 100, Vec2{ 25, 0 });

	//キャラ描画
	texture[0].resized(800).drawAt(500, 500);

	if (IsLoad) {

		BaseWindow.draw(Arg::top = Palette::Black, Arg::bottom = Palette::Dimgray).drawFrame(2, Palette::White);

		//ロード戻しボタン
		Rect SaveButton{ Arg::center(650, 100), 50, 50 };
		SaveButton.draw(Palette::Black).drawFrame(2, Palette::White);
		icon1.drawAt(SaveButton.pos + Vec2(25, 25));

		m_save1Button.draw(ColorF{ m_save1Transition.value() }).drawFrame(2);
		m_save2Button.draw(ColorF{ m_save2Transition.value() }).drawFrame(2);
		m_save3Button.draw(ColorF{ m_save3Transition.value() }).drawFrame(2);

		const Font& boldFont = FontAsset(U"Bold");
		boldFont(LoadText[0]).drawAt(30, m_save1Button.center(), ColorF{1 - m_save1Transition.value()});
		boldFont(LoadText[1]).drawAt(30, m_save2Button.center(), ColorF{1 - m_save2Transition.value()});
		boldFont(LoadText[2]).drawAt(30, m_save3Button.center(), ColorF{1 - m_save3Transition.value()});
	}
	else {
		// ボタン描画

		m_startButton.draw(ColorF{ m_startTransition.value() }).drawFrame(2);
		m_continueButton.draw(ColorF{ m_continueTransition.value() }).drawFrame(2);
		m_exitButton.draw(ColorF{  m_exitTransition.value() }).drawFrame(2);

		const Font& boldFont = FontAsset(U"Bold");
		boldFont(U"PLAY").drawAt(36, m_startButton.center(), ColorF{ 1 - m_startTransition.value() });
		boldFont(U"CONTINUE").drawAt(36, m_continueButton.center(), ColorF{ 1 - m_continueTransition.value() });
		boldFont(U"EXIT").drawAt(36, m_exitButton.center(), ColorF{ 1 - m_exitTransition.value() });

	}
}
