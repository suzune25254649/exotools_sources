#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <cstdint>
#include <set>
#include "../common/exo.h"
#include "../common/args.h"

int main(int argc, char **argv)
{
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
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".PackLayer.exo";
			}
		}
	}

	ExoFile exoSource;
	if (LoadExoFile(inputs[0], exoSource))
	{
		fprintf(stderr, "error: 引数に渡されたexoファイルが開けません。\n");
		return 2;
	}

	std::unordered_map<int, int> converter;
	{
		std::set<int> listup;
		for (auto &it : exoSource.objects)
		{
			int layer = ::atoi(it.header.get("layer")->c_str());
			listup.insert(layer);
		}
		{
			int i = 1;
			for (auto it : listup)
			{
				converter.insert(std::pair<int, int>(it, i));
				++i;
			}
		}
	}

	for (auto &it : exoSource.objects)
	{
		int layer = ::atoi(it.header.get("layer")->c_str());
		it.header.set("layer", std::to_string(converter.find(layer)->second));
	}

	if (SaveExoFile(filepathExoDest, exoSource))
	{
		fprintf(stderr, "error: \"%s\"が生成できませんでした。\n", filepathExoDest.c_str());
		return 3;
	}

	printf("正常に終了しました。\n");
	return 0;
}
