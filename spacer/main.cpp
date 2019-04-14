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

	//	ずらすフレーム数.
	int frame = 0;

	//	起動引数の解読.
	{
		int err = args(argc, argv, inputs, options, { "-np" }, { "-o", "-f" });
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
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".spacer.exo";
			}
		}

		{
			auto it = options.find("-f");
			if (options.end() != it)
			{
				frame = ::atoi(it->second.c_str());
			}
			else
			{
				printf("スペースをフレーム数で指定してください。\n>> ");
				scanf_s("%d", &frame);
				(void)getchar();
				frame = std::max<int>(0, frame);
			}
		}
	}

	ExoFile exoSource;
	if (LoadExoFile(inputs[0], exoSource))
	{
		fprintf(stderr, "error: 引数に渡されたexoファイルが開けません。\n");
		return 2;
	}

	printf("> %d frame ずつスペースをあけます。\n", frame);

	SortExoObjectsByStart(exoSource.objects);

	int nowstart = 0;
	int shift = -1;
	for (auto &it : exoSource.objects)
	{
		int start = ::atoi(it.header.get("start")->c_str());
		int end = ::atoi(it.header.get("end")->c_str());

		if (nowstart != start)
		{
			nowstart = start;
			++shift;
		}

		it.header.set("start", std::to_string(start + (shift * frame)));
		it.header.set("end", std::to_string(end + (shift * frame)));
	}

	SortExoObjects(exoSource.objects);

	if (SaveExoFile(filepathExoDest, exoSource))
	{
		fprintf(stderr, "error: \"%s\"が生成できませんでした。\n", filepathExoDest.c_str());
		return 3;
	}

	printf("正常に終了しました。\n");
	return 0;
}
