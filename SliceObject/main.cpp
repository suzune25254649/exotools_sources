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

	int layerTarget = 0;
	int layerRef = 0;

	//	起動引数の解読.
	{
		int err = args(argc, argv, inputs, options, { "-np" }, { "-o", "-t", "-r" });
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
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".SliceObject.exo";
			}
		}

		{
			auto it = options.find("-t");
			if (options.end() != it)
			{
				layerTarget = ::atoi(it->second.c_str());
			}
			else
			{
				printf("「分割したいオブジェクト」があるレイヤーの番号を指定してください。\n>> ");
				scanf_s("%d", &layerTarget);
				(void)getchar();
			}
		}

		{
			auto it = options.find("-r");
			if (options.end() != it)
			{
				layerRef = ::atoi(it->second.c_str());
			}
			else
			{
				printf("「分割する箇所を指定するオブジェクト」があるレイヤーの番号を指定してください。\n>> ");
				scanf_s("%d", &layerRef);
				(void)getchar();
			}
		}
	}

	ExoFile exoSource;
	if (LoadExoFile(inputs[0], exoSource))
	{
		fprintf(stderr, "error: 引数に渡されたexoファイルが開けません。\n");
		return 2;
	}

	//	レイヤー番号指定が正しいかをチェック.
	{
		if (layerTarget == layerRef)
		{
			fprintf(stderr, "error: レイヤー番号に同じ値が指定されています。\n");
			return 3;
		}

		int countTarget = 0;
		int countRef = 0;
		for (auto it : exoSource.objects)
		{
			const int layer = ::atoi(it.header.get("layer")->c_str());
			if (layer == layerTarget)
			{
				++countTarget;
			}
			if (layer == layerRef)
			{
				++countRef;
			}
		}

		if (0 == countTarget)
		{
			fprintf(stderr, "error: レイヤー(%d)にオブジェクトがありません。\n", layerTarget);
			return 4;
		}
		if (0 == countRef)
		{
			fprintf(stderr, "error: レイヤー(%d)にオブジェクトがありません。\n", layerRef);
			return 5;
		}
	}


	ExoFile exoDest;
	exoDest.exedit = exoSource.exedit;

	SortExoObjects(exoSource.objects);

	{
		auto it = exoSource.objects.begin();
		while (exoSource.objects.end() != it)
		{
			int layer = ::atoi(it->header.get("layer")->c_str());
			if (layer == layerTarget)
			{
				break;
			}
			exoDest.objects.push_back(*it);
			++it;
		}

		for (auto &itRef : exoSource.objects)
		{
			int layer = ::atoi(itRef.header.get("layer")->c_str());
			if (layer != layerRef)
			{
				continue;
			}

			int startTarget = ::atoi(it->header.get("start")->c_str());
			int endTarget = ::atoi(it->header.get("end")->c_str());
			int startRef = ::atoi(itRef.header.get("start")->c_str());

			//	分割ターゲットのendより、参照のstartの方が後ろにあるので、分割ターゲット(it)を進める.
			if (endTarget < startRef)
			{
				exoDest.objects.push_back(*it);
				++it;
				if (exoSource.objects.cend() == it)
				{
					break;
				}
				int layer = ::atoi(it->header.get("layer")->c_str());
				if (layer != layerTarget)
				{
					break;
				}
			}
			//	分割ターゲットのstartより、参照のstartの方が前にあるので、参照(itRef)を進める.
			//	startが一致している場合も分割不要なので、参照(itRef)を進める.
			if (startRef <= startTarget)
			{
				continue;
			}

			//	分割し、前半をリストに加える.
			ExoObject obj = *it;
			obj.header.set("end", std::to_string(startRef - 1));
			exoDest.objects.push_back(obj);

			//	分割した後半のstartを書き換え、処理を進める. (再分割されるかもしれないので、++itしてはならない)
			it->header.set("start", std::to_string(startRef));
		}

		while (exoSource.objects.end() != it)
		{
			exoDest.objects.push_back(*it);
			++it;
		}
	}

	if (SaveExoFile(filepathExoDest, exoDest))
	{
		fprintf(stderr, "error: \"%s\"が生成できませんでした。\n", filepathExoDest.c_str());
		return 6;
	}

	printf("正常に終了しました。\n");
	return 0;
}
