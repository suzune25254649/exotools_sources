#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <cstdint>
#include "../common/exo.h"
#include "../common/args.h"

int main(int argc, char **argv)
{
	const std::string filenameSetting = "MultiSliderMaker.setting.exo";
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
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".MultiSliderMaker.exo";
			}
		}
	}

	ExoFile exoSource;
	if (LoadExoFile(inputs[0], exoSource))
	{
		fprintf(stderr, "error: 引数に渡されたexoファイルが開けません。\n");
		return 2;
	}
	ExoFile exoSetting;
	if (LoadExoFile(dirnameSetting + "\\" + filenameSetting, exoSetting))
	{
		fprintf(stderr, "error: EXEと同じフォルダに\"%s\"がありません。\n", filenameSetting.c_str());
		return 3;
	}

	ExoFile exoDest;
	exoDest.exedit = exoSource.exedit;

	for (auto &it : exoSource.objects)
	{
		if (it.params.empty())
		{
			continue;
		}
		if (it.params[0].equals("_name", "音声ファイル"))
		{

			//	*.patam.txtから、内容を取得する.
			std::vector<int> params;
			std::string filepath;
			{
				std::wstring text;
				auto value = it.params[0].get("file");
				if (nullptr == value)
				{
					continue;
				}
				filepath = value->substr(0, value->size() - 3) + "param.txt";

				if (LoadText(filepath, text))
				{
					fprintf(stderr, "warning: \"%s\"が読み込めません。\n", filepath.c_str());
					continue;
				}
				text = trim(text);
				auto tokens = split(text, L" ");
				for (auto &it : tokens)
				{
					it = trim(it);
					if (L"" != it)
					{
						params.push_back(::_wtoi(it.c_str()));
					}
				}
			}

			ExoFile temp = exoSetting;
			temp.objects[0].header.set("start", *it.header.get("start"));
			temp.objects[0].header.set("end", *it.header.get("end"));
			temp.objects[0].header.set("layer", *it.header.get("layer"));

			size_t index = 0;
			for (auto &it : temp.objects[0].params)
			{
				if (params.size() <= index)
				{
					break;
				}

				{
					auto *str = it.get("name");
					if (nullptr == str || "多目的スライダー@PSDToolKit" != *str)
					{
						continue;
					}
				}

				//	1つの項目につき、トラックは4つまで.
				for (int i = 0; i < 4 && index < params.size(); ++i, ++index)
				{
					std::string value = std::to_string(params[index]);
					auto *str = it.get("track" + std::to_string(i));
					if (nullptr != str)
					{
						auto tokens = split(*str, ",");
						if (3 <= tokens.size())
						{
							it.set("track" + std::to_string(i), value + "," + value + "," + tokens[2]);
						}
						else
						{
							it.set("track" + std::to_string(i), value + "," + value + ",0");
						}
					}
					else
					{
						it.set("track" + std::to_string(i), value + "," + value + ",0");
					}
				}
			}
			if (index < params.size())
			{
				fprintf(stderr, "warning: \"%s\"でパラメータが%d個指定されていますが、元となるexoファイルの多目的スライダーに充分なスライダーがありません。元となるexoファイルを拡張してください。\n", filepath.c_str(), params.size());
				continue;
			}
			exoDest.objects.push_back(temp.objects[0]);
		}
	}

	if (SaveExoFile(filepathExoDest, exoDest))
	{
		fprintf(stderr, "error: \"%s\"が生成できませんでした。\n", filepathExoDest.c_str());
		return 4;
	}

	printf("正常に終了しました。\n");
	return 0;
}
