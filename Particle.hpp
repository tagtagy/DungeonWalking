#pragma once
# include <Siv3D.hpp>

struct HitEffect : IEffect
{
	Vec2 m_pos;

	ColorF m_color;

	// このコンストラクタ引数が .add<RingEffect>() の引数になる
	explicit HitEffect(const Vec2& pos)
		: m_pos{ pos }
		, m_color{ RandomColorF() } {
	}

	bool update(double t) override
	{
		// 時間に応じて大きくなる輪を描く
		Circle{ m_pos, (t * 100) }.drawFrame(4, m_color);

		// 1 秒未満なら継続する
		return (t < 1.0);
	}
};
