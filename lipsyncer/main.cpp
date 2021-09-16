#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <cstdint>
#include "../common/exo.h"
#include "../common/args.h"

int main(int argc, char **argv)
{
	const std::string filenameSetting = "lipsyncer.setting.exo";
	const std::string dirnameSetting = GetSettingFileDir();
	std::string filepathExoDest;

	std::vector<std::string> inputs;
	std::unordered_map<std::string, std::string> options;

	//	�N�������̉��.
	{
		int err = args(argc, argv, inputs, options, { "-np" }, { "-o" });
		if (0 != err || inputs.empty())
		{
			fprintf(stderr, "error: �N���p�����[�^���ُ�ł��B\n");
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
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".lipsyncer.exo";
			}
		}
	}

	ExoFile exoSource;
	if (LoadExoFile(inputs[0], exoSource))
	{
		fprintf(stderr, "error: �����ɓn���ꂽexo�t�@�C�����J���܂���B\n");
		return 2;
	}
	ExoFile exoSetting;
	if (LoadExoFile(dirnameSetting + "\\" + filenameSetting, exoSetting))
	{
		fprintf(stderr, "error: EXE�Ɠ����t�H���_��\"%s\"������܂���B\n", filenameSetting.c_str());
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
		if (it.params[0].equals("_name", "�����t�@�C��") && nullptr != it.params[0].get("file"))
		{
			exoSetting.objects[0].header.set("start", *it.header.get("start"));
			exoSetting.objects[0].header.set("end", *it.header.get("end"));
			exoSetting.objects[0].header.set("layer", *it.header.get("layer"));
			exoSetting.objects[0].params[0].set("param", "file=\"" + replaceAll(*it.params[0].get("file"), "\\", "\\\\") + "\"");

			exoDest.objects.push_back(exoSetting.objects[0]);
		}
	}

	if (SaveExoFile(filepathExoDest, exoDest))
	{
		fprintf(stderr, "error: \"%s\"�������ł��܂���ł����B\n", filepathExoDest.c_str());
		return 4;
	}

	printf("����ɏI�����܂����B\n");
	return 0;
}
