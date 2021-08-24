#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <cstdint>
#include "../common/exo.h"
#include "../common/args.h"

int main(int argc, char **argv)
{
	const std::string dirnameSetting = GetSettingFileDir();
	std::string filepathExoDest;

	std::vector<std::string> inputs;
	std::unordered_map<std::string, std::string> options;

	int minStart = 0x7FFFFFFF;
	int maxEnd = 0;

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
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".ExtendObject.exo";
			}
		}
	}

	ExoFile exoSource;
	if (LoadExoFile(inputs[0], exoSource))
	{
		fprintf(stderr, "error: 引数に渡されたexoファイルが開けません。\n");
		return 2;
	}

	//	startの最小値, endの最大値を探す.
	{
		for (auto it : exoSource.objects)
		{
			const int start = ::atoi(it.header.get("start")->c_str());
			const int end = ::atoi(it.header.get("end")->c_str());
			minStart = std::min<int>(start, minStart);
			maxEnd = std::max<int>(end, maxEnd);
		}
	}

	SortExoObjects(exoSource.objects);

	{
		auto it = exoSource.objects.begin();
		while (exoSource.objects.end() != it)
		{
			int layer = ::atoi(it->header.get("layer")->c_str());
			auto itNext = it + 1;
			if (exoSource.objects.end() == itNext || layer != ::atoi(itNext->header.get("layer")->c_str()))
			{
				//	このitは、レイヤーにおいて終末のオブジェクトである.
				it->header.set("end", std::to_string(maxEnd));
			}
			else
			{
				int n = ::atoi(itNext->header.get("start")->c_str());
				it->header.set("end", std::to_string(n - 1));
			}
			++it;
		}
	}

	if (SaveExoFile(filepathExoDest, exoSource))
	{
		fprintf(stderr, "error: \"%s\"が生成できませんでした。\n", filepathExoDest.c_str());
		return 6;
	}

	printf("正常に終了しました。\n");
	return 0;
}
