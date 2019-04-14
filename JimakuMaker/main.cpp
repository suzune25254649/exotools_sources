#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <cstdint>
#include "../common/exo.h"
#include "../common/args.h"

int main(int argc, char **argv)
{
	const std::string filenameSetting = "JimakuMaker.setting.exo";
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
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".JimakuMaker.exo";
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
			ExoObject obj = exoSetting.objects[0];

			//	字幕内容をテキストファイルから取得する.
			std::wstring jimaku;
			{
				std::string filepath = *it.params[0].get("file");
				filepath = filepath.substr(0, filepath.size() - 3) + "txt";
				std::string text;
				if (LoadText(filepath, text))
				{
					fprintf(stderr, "error: \"%s\"が読み込めません。\n", filepath.c_str());
					continue;
				}
				jimaku = StringToWString(trim(text));

				auto pos = jimaku.find(L"＞");
				if (std::wstring::npos != pos)
				{
					jimaku = jimaku.substr(pos + 1);
				}
			}

			//	セリフ部分を置換.
			jimaku = replaceAll(TextToWString(*obj.params[0].get("text")), L"セリフ", jimaku);

			obj.header.set("start", *it.header.get("start"));
			obj.header.set("end", *it.header.get("end"));
			obj.header.set("layer", *it.header.get("layer"));
			obj.params[0].set("text", WStringToText(jimaku));

			exoDest.objects.push_back(obj);
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
