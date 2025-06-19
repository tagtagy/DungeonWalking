#include "stdafx.h"
#include "Save.hpp"

Save::Save() {
	
}

Array<uint8> Save::AesEncryption(const String text, const String keyText) {
	// UTF-8変換（std::string）
	std::string utf8str = text.toUTF8();
	// std::string → Array<uint8>
	m_block.assign(utf8str.begin(), utf8str.end());
	
	// 16バイトに調整（パディング or 切り詰め）
	if (m_block.size() < text.size())
	{
		m_block.resize(text.size()); // 0埋め
	}
	else if (m_block.size() > text.size())
	{
		m_block.resize(text.size()); // 最初の16バイトだけ使う
	}
	//Print << m_block;
	AddRoundKey(keyText);
	//Print << m_block;
	SubBytes();
	//Print << m_block;
	//ShiftRows();
	//Print << m_block;
	//MixColumns();
	//Print << m_block;
	AddRoundKey(keyText);
	//Print << m_block;
	// セーブ用のテキストファイルをオープンする
	TextWriter writer{ U"test.txt" };

	// ファイルをオープンできなかったら
	if (not writer)
	{
		// 例外を投げて終了する
		throw Error{ U"Failed to open `test.txt`" };
	}
	writer.writeln(toBlock());

	return toBlock();
}
String Save::InverseAesEncryption(const String _text, const String keyText) {

	// テキストファイルの内容をすべて読み込む
	String text = _text;

	text.replace(U'{', U' ').replace(U'}', U' ');
	Array <String> tests = text.split(U',');

	Array<uint8> result;

	for (const auto& token : tests) {
		if (token.trimmed().isEmpty()) continue;
		result << Parse<uint8>(token.trimmed());
	}

	m_block = result;

	//Print << U"aaaaaaaaaaaaaaaaaaaa";
	//Print << m_block;
	AddRoundKey(keyText);
	//Print << m_block;
	//InverseMixColumns();
	//Print << m_block;
	//InverseShiftRows();
	//Print << m_block;
	InverseSubBytes();
	//Print << m_block;
	AddRoundKey(keyText);
	//Print << m_block;
	TextWriter writer{ U"test1.txt" };

	// ファイルをオープンできなかったら
	if (not writer)
	{
		// 例外を投げて終了する
		throw Error{ U"Failed to open `test.txt`" };
	}

	// 復号化後のテキストを表示
	auto decryptedBlock = m_block;
	// 復号化後の文字列を表示（元のテキスト）
	String resultText(decryptedBlock.begin(), decryptedBlock.end());
	writer.writeln(resultText);

	return resultText;
}

// 鍵を文字列から取得するメソッド
Array<uint8> Save::generateKeyFromString(const String password, size_t blockSize)
{
	std::string utf8 = password.toUTF8();
	Array<uint8> key(blockSize);

	for (size_t i = 0; i < blockSize; ++i)
	{
		key[i] = (i < utf8.size()) ? static_cast<uint8>(utf8[i]) : 0;
	}

	return key;
}

void Save::SubBytes()
{
	for (size_t i = 0; i < m_block.size(); ++i)
	{
		m_block[i] = sbox[m_block[i]];  // S-Boxによる置換
	}
}
void Save::InverseSubBytes()
{
	for (size_t i = 0; i < m_block.size(); ++i)
	{
		m_block[i] = inv_sbox[m_block[i]];  // 逆S-Boxによる置換
	}
}

// ShiftRows：行のシフト処理
void Save::ShiftRows()
{
	Array<uint8> tmp = m_block; // 変換後の一時保存用

	// 1行目：そのまま
	// 2行目：1バイト左シフト
	m_block[1] = tmp[5];
	m_block[5] = tmp[9];
	m_block[9] = tmp[13];
	m_block[13] = tmp[1];

	// 3行目：2バイト左シフト
	m_block[2] = tmp[10];
	m_block[6] = tmp[14];
	m_block[10] = tmp[2];
	m_block[14] = tmp[6];

	// 4行目：3バイト左シフト（＝1バイト右シフト）
	m_block[3] = tmp[15];
	m_block[7] = tmp[3];
	m_block[11] = tmp[7];
	m_block[15] = tmp[11];
}
void Save::InverseShiftRows()
{
	Array<uint8> tmp = m_block; // 一時保存

	// 1行目：そのまま
	// 2行目：1バイト右シフト
	m_block[1] = tmp[13];
	m_block[5] = tmp[1];
	m_block[9] = tmp[5];
	m_block[13] = tmp[9];

	// 3行目：2バイト右シフト
	m_block[2] = tmp[10];
	m_block[6] = tmp[14];
	m_block[10] = tmp[2];
	m_block[14] = tmp[6];

	// 4行目：3バイト右シフト
	m_block[3] = tmp[15];
	m_block[7] = tmp[3];
	m_block[11] = tmp[7];
	m_block[15] = tmp[11];
}

// MixColumns の演算
uint8 Save::gf_multiply(uint8 a, uint8 b)
{
	uint8 p = 0;
	uint8 hbs = 0x80;  // 高位ビット
	for (int i = 0; i < 8; i++)
	{
		if (b & 0x01) {
			p ^= a;  // bの最下位ビットが1なら、pにaをXOR
		}
		a <<= 1;  // aを左シフト
		if (a & hbs) {
			a ^= 0x11B;  // 最上位ビットが1の場合、0x11B（GF(2^8)の生成多項式）をXOR
		}
		b >>= 1;  // bを右シフト
	}
	return p;
}

void Save::MixColumns()
{
	Array<uint8> tmp = m_block;  // 変換後の一時保存用

	for (int i = 0; i < 4; i++)  // 4列
	{
		// 各列に対して行列を適用
		uint8 col[4];
		for (int j = 0; j < 4; j++)
		{
			col[j] = tmp[j * 4 + i];  // 列ごとのデータを取得
		}

		// 行列乗算の結果を格納
		for (int j = 0; j < 4; j++)
		{
			m_block[j * 4 + i] = 0;
			for (int k = 0; k < 4; k++)
			{
				m_block[j * 4 + i] ^= gf_multiply(mixMatrix[j][k], col[k]); // 行列との乗算
			}
		}
	}
}
void Save::InverseMixColumns()
{
	Array<uint8> tmp = m_block;

	for (int i = 0; i < 4; i++)  // 4列
	{
		uint8 col[4];
		for (int j = 0; j < 4; j++)
		{
			col[j] = tmp[j * 4 + i];
		}

		// 逆行列との乗算
		for (int j = 0; j < 4; j++)
		{
			m_block[j * 4 + i] = 0;
			for (int k = 0; k < 4; k++)
			{
				m_block[j * 4 + i] ^= gf_multiply(invMixMatrix[j][k], col[k]);
			}
		}
	}
}

void Save::AddRoundKey(const String keyText)
{
	Array<uint8> key = generateKeyFromString(keyText, m_block.size());

	if (m_block.size() != key.size())
	{
		throw Error{ U"AddRoundKey: ブロックと鍵のサイズが一致しません。" };
	}

	for (size_t i = 0; i < m_block.size(); ++i)
	{
		m_block[i] ^= key[i];  // XOR演算
	}
}
