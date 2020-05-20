#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <cstdint>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "../common/exo.h"
#include "../common/args.h"

extern uint32_t CalcWavLength(const char *filename, const uint32_t rate);

int main(int argc, char **argv)
{
	const std::string filenameSetting = "wav2exo.setting.exo";
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
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".wav2exo.exo";
			}
		}
	}
	
	ExoFile exoSetting;
	if (LoadExoFile(dirnameSetting + "\\" + filenameSetting, exoSetting))
	{
		fprintf(stderr, "error: EXE�Ɠ����t�H���_��\"%s\"������܂���B\n", filenameSetting.c_str());
		return 2;
	}

	const uint32_t rate = ::atoi(exoSetting.exedit.get("rate")->c_str());

	ExoFile exoDest;
	exoDest.exedit = exoSetting.exedit;

	uint32_t start = 1;
	for(auto it : inputs)
	{
		std::vector<std::string> sources;

		if (::PathIsDirectoryA(it.c_str()))
		{
			HANDLE hFind;
			WIN32_FIND_DATAA win32fd;
			hFind = ::FindFirstFileA((it + "\\*.wav").c_str(), &win32fd);
			if (hFind == INVALID_HANDLE_VALUE)
			{
				fprintf(stderr, "error: �t�H���_\"%s\"�̒��ɃA�N�Z�X�ł��܂���B\n", it.c_str());
				return 3;
			}
			do
			{
				sources.push_back(it + "\\" + win32fd.cFileName);
			}
			while (::FindNextFileA(hFind, &win32fd));
			::FindClose(hFind);

			std::sort(sources.begin(), sources.end(), [](const std::string& left, const std::string& right) {
				return (0 > ::StrCmpLogicalW(StringToWString(left).c_str(), StringToWString(right).c_str()));
			});
		}
		else
		{
			sources.push_back(it);
		}

		for (auto itSrc : sources)
		{
			uint32_t length = CalcWavLength(itSrc.c_str(), rate);

			char path[32768];
			GetFullPathNameA(itSrc.c_str(), sizeof(path), path, nullptr);

			auto obj = exoSetting.objects[0];
			obj.header.set("start", std::to_string(start));
			obj.header.set("end", std::to_string(start + length - 1));
			obj.params[0].set("file", path);
			exoDest.objects.push_back(obj);

			start += length;
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



/**
�����t�@�C��(*.wav)��ǂݍ��݁A���̍Đ����Ԃ𓮉�ł̃t���[�����ɕϊ�����.
@param	filename	�����t�@�C����.
@param	rate		����̕b�ԃt���[�����[�g.
@return				�t���[����.
*/
uint32_t CalcWavLength(const char *filename, const uint32_t rate)
{
	FILE *fr = nullptr;
	if (fopen_s(&fr, filename, "rb"))
	{
		return 0;
	}
	uint32_t sizeHeader;
	fseek(fr, 16, SEEK_SET);
	fread(&sizeHeader, 1, sizeof(sizeHeader), fr);

	PCMWAVEFORMAT header;
	fread(&header, 1, sizeof(header), fr);

	uint32_t sizeData;
	fseek(fr, 20 + sizeHeader + 4, SEEK_SET);
	fread(&sizeData, 1, sizeof(sizeData), fr);
	fclose(fr);

	return (rate * sizeData + (header.wf.nAvgBytesPerSec - 1)) / header.wf.nAvgBytesPerSec;
}
