#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <cstdint>
#include "../common/exo.h"
#include "../common/args.h"

enum TextType
{
	TT_UNKNOWN = 0,
	TT_VOICEROID,
	TT_CEVIO,
};

extern TextType GetTextType(std::string filepath, const std::vector< std::pair<std::wstring, int> > &names);

int main(int argc, char **argv)
{
	const std::string filenameSetting = "ClassifyLayer.setting.txt";
	const std::string dirnameSetting = GetSettingFileDir();
	std::string filepathExoDest;

	std::vector<std::string> inputs;
	std::unordered_map<std::string, std::string> options;

	//	起動引数の解読.
	{
		int err = args(argc, argv, inputs, options, { "-np" }, { "-o" });
		if (0 != err || inputs.empty())
		{
			fprintf(stderr, "error: 起動パラメータが異常です。\n");
			return 1;
		}

		{
			auto it = options.find("-o");
			if (options.end() != it)
			{
				filepathExoDest = it->second;
			}
			else
			{
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".ClassifyLayer.exo";
			}
		}
	}

	const std::string filepathExoSource = inputs[0];

	//	レイヤー分け設定のテキストを分析する.
	//	pair.first == 名前.
	//	pair.second == レイヤー番号.
	//	名前は先頭一致のため、vectorの中身を「名前が長い順」で並び替える.
	std::vector< std::pair<std::wstring, int> > names;
	{
		std::wstring text;
		if (LoadText(dirnameSetting + "\\" + filenameSetting, text))
		{
			fprintf(stderr, "error: EXEと同じフォルダに\"%s\"がありません。\n", filenameSetting.c_str());
			return 2;
		}
		int i = 0;
		auto lines = split(text, L"\n");
		for (auto &line : lines)
		{
			auto s = trim(line);
			if (!s.empty())
			{
				names.push_back(std::pair<std::wstring, int>(s, i));
				++i;
			}
		}
		std::sort(names.begin(), names.end(),
			[](const std::pair<std::wstring, int> &lh, const std::pair<std::wstring, int> &rh)
		{
			return rh.first.length() < lh.first.length();
		});
	}

	ExoFile exo;
	if (LoadExoFile(filepathExoSource, exo))
	{
		fprintf(stderr, "error: \"%s\"が読み込めません。\n", filepathExoSource.c_str());
		return 3;
	}

	{
		TextType type = TT_UNKNOWN;

		std::wstring talker;
		size_t index = 0;
		for (auto &it : exo.objects)
		{
			if (it.params.empty())
			{
				continue;
			}
			if (!it.params[0].equals("_name", "音声ファイル"))
			{
				continue;
			}

			std::string filepath = *it.params[0].get("file");
			filepath = filepath.substr(0, filepath.size() - 3) + "txt";

			std::wstring text;
			if (LoadText(filepath, text))
			{
				fprintf(stderr, "error: \"%s\"が読み込めません。\n", filepath.c_str());
				return 4;
			}

			//	1つ目の処理時に、TextTypeがどれかを決定する.
			//	以後はここで決定したTextTypeとして解析するため、混在は不可.
			if (TT_UNKNOWN == type)
			{
				type = GetTextType(filepath, names);
				switch (type)
				{
				case TT_UNKNOWN:
					fprintf(stderr, "error: \"%s\"の設定と合致しませんでした。セッティングファイルを修正してください。\n", filenameSetting.c_str());
					return 5;
				case TT_VOICEROID:
					fprintf(stdout, "> VOICEROID2形式だと判断しました。\n");
					break;
				case TT_CEVIO:
					fprintf(stdout, "> CeVIO形式だと判断しました。\n");
					break;
				}
			}

			//	TextType形式に従って解析し、話者を決定する.
			if (TT_VOICEROID == type)
			{
				auto pos = text.find(L"＞");
				if (std::string::npos != pos)
				{
					talker = trim(text.substr(0, pos));
				}
			}
			else if (TT_CEVIO == type)
			{
				std::smatch match;
				if (std::regex_search(filepath, match, std::regex(R"((\\|/)\d+_(.+)_(.+)\.txt$)")))
				{
					talker = StringToWString(match[2]);
				}
				else
				{
					fprintf(stderr, "error: \"%s\"のファイル名がCeVIO形式ではありません。\n", filepath.c_str());
					return 6;
				}
			}

			//	話者を名前リストから探し、レイヤー番号を書き換える.
			size_t layer = names.size();
			for (auto it = names.cbegin(); names.cend() != it; ++it)
			{
				if (!it->first.empty() && 0 == talker.find(it->first))
				{
					layer = it->second;
					break;
				}
			}
			it.header.set("layer", std::to_string(layer + 1));
		}
	}

	//	レイヤー番号を書き換えているので、ソートしなおすことにする.
	SortExoObjects(exo.objects);

	if (SaveExoFile(filepathExoDest, exo))
	{
		fprintf(stderr, "error: \"%s\"が生成できませんでした。\n", filepathExoDest.c_str());
		return 7;
	}

	printf("正常に終了しました。\n");
	return 0;
}


TextType GetTextType(std::string filepath, const std::vector< std::pair<std::wstring, int> > &names)
{
	std::wstring text;
	if (LoadText(filepath, text))
	{
		return TT_UNKNOWN;
	}


	//	ボイロ形式か確認. 最初のテキストに"＞"が存在し、かつプリセット名が名前リストと先頭一致すればボイロ形式.
	{
		auto pos = text.find(L"＞");
		if (std::string::npos != pos)
		{
			std::wstring wstr = trim(text.substr(0, pos));
			for (auto it = names.cbegin(); names.cend() != it; ++it)
			{
				if (!it->first.empty() && 0 == wstr.find(it->first))
				{
					return TT_VOICEROID;
					break;
				}
			}
		}
	}

	std::smatch match;
	if (std::regex_search(filepath, match, std::regex(R"((\\|/)\d+_(.+)_(.+)\.txt$)")))
	{
		std::wstring wstr = StringToWString(match[2]);
		for (size_t i = 0; i < names.size(); ++i)
		{
			if (!names[i].first.empty() && 0 == wstr.find(names[i].first))
			{
				return TT_CEVIO;
			}
		}
	}
	return TT_UNKNOWN;
}
