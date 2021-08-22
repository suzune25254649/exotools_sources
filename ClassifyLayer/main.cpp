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
				filepathExoDest = dirnameSetting + "\\exo_" + GetTimeString() + ".ClassifyLayer.exo";
			}
		}
	}

	const std::string filepathExoSource = inputs[0];

	//	���C���[�����ݒ�̃e�L�X�g�𕪐͂���.
	//	pair.first == ���O.
	//	pair.second == ���C���[�ԍ�.
	//	���O�͐擪��v�̂��߁Avector�̒��g���u���O���������v�ŕ��ёւ���.
	std::vector< std::pair<std::wstring, int> > names;
	{
		std::wstring text;
		if (LoadText(dirnameSetting + "\\" + filenameSetting, text))
		{
			fprintf(stderr, "error: EXE�Ɠ����t�H���_��\"%s\"������܂���B\n", filenameSetting.c_str());
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
		fprintf(stderr, "error: \"%s\"���ǂݍ��߂܂���B\n", filepathExoSource.c_str());
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
			if (!it.params[0].equals("_name", "�����t�@�C��"))
			{
				continue;
			}

			std::string filepath = *it.params[0].get("file");
			filepath = filepath.substr(0, filepath.size() - 3) + "txt";

			std::wstring text;
			if (LoadText(filepath, text))
			{
				fprintf(stderr, "error: \"%s\"���ǂݍ��߂܂���B\n", filepath.c_str());
				return 4;
			}

			//	1�ڂ̏������ɁATextType���ǂꂩ�����肷��.
			//	�Ȍ�͂����Ō��肵��TextType�Ƃ��ĉ�͂��邽�߁A���݂͕s��.
			if (TT_UNKNOWN == type)
			{
				type = GetTextType(filepath, names);
				switch (type)
				{
				case TT_UNKNOWN:
					fprintf(stderr, "error: \"%s\"�̐ݒ�ƍ��v���܂���ł����B�Z�b�e�B���O�t�@�C�����C�����Ă��������B\n", filenameSetting.c_str());
					return 5;
				case TT_VOICEROID:
					fprintf(stdout, "> VOICEROID2�`�����Ɣ��f���܂����B\n");
					break;
				case TT_CEVIO:
					fprintf(stdout, "> CeVIO�`�����Ɣ��f���܂����B\n");
					break;
				}
			}

			//	TextType�`���ɏ]���ĉ�͂��A�b�҂����肷��.
			if (TT_VOICEROID == type)
			{
				auto pos = text.find(L"��");
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
					fprintf(stderr, "error: \"%s\"�̃t�@�C������CeVIO�`���ł͂���܂���B\n", filepath.c_str());
					return 6;
				}
			}

			//	�b�҂𖼑O���X�g����T���A���C���[�ԍ�������������.
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

	//	���C���[�ԍ������������Ă���̂ŁA�\�[�g���Ȃ������Ƃɂ���.
	SortExoObjects(exo.objects);

	if (SaveExoFile(filepathExoDest, exo))
	{
		fprintf(stderr, "error: \"%s\"�������ł��܂���ł����B\n", filepathExoDest.c_str());
		return 7;
	}

	printf("����ɏI�����܂����B\n");
	return 0;
}


TextType GetTextType(std::string filepath, const std::vector< std::pair<std::wstring, int> > &names)
{
	std::wstring text;
	if (LoadText(filepath, text))
	{
		return TT_UNKNOWN;
	}


	//	�{�C���`�����m�F. �ŏ��̃e�L�X�g��"��"�����݂��A���v���Z�b�g�������O���X�g�Ɛ擪��v����΃{�C���`��.
	{
		auto pos = text.find(L"��");
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
