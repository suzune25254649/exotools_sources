#pragma once
/*
exoファイルを加工しやすくするための関数群
*/

#include "common.h"
#include <regex>
#include <algorithm>

struct KeyStore
{
	std::string key;
	std::string value;
};

struct KeyStores
{
	std::vector<KeyStore> datas;

	void set(std::string key, std::string value)
	{
		auto n = get(key);
		if (n)
		{
			//	すでにある.
			*const_cast<std::string*>(n) = value;
			return;
		}
		KeyStore s;
		s.key = key;
		s.value = value;
		datas.push_back(s);
	}
	const std::string *get(std::string key)const
	{
		for (auto &it : datas)
		{
			if (key == it.key)
			{
				return &it.value;
			}
		}
		return nullptr;
	}
	bool equals(std::string key, std::string value)const
	{
		auto s = get(key);
		return (nullptr != s) && *s == value;
	}
};

struct ExoObject
{
	KeyStores header;
	std::vector< KeyStores > params;
};

struct ExoFile
{
	KeyStores exedit;
	std::vector<ExoObject> objects;
};

std::wstring TextToWString(std::string text)
{
	std::wstring result;
	for (auto it = text.cbegin(); text.cend() != it; ++it)
	{
		wchar_t ch = 0;
		ch += ((*it <= '9') ? *it - '0' : *it - 'a' + 10) << 4;
		++it;
		ch += ((*it <= '9') ? *it - '0' : *it - 'a' + 10) << 0;
		++it;
		ch += ((*it <= '9') ? *it - '0' : *it - 'a' + 10) << 12;
		++it;
		ch += ((*it <= '9') ? *it - '0' : *it - 'a' + 10) << 8;

		result += ch;
	}
	return result;
}

std::string WStringToText(std::wstring str)
{
	std::string result;
	for (auto it = str.cbegin(); str.cend() != it; ++it)
	{
		int n = 0;
		n = (*it >> 4) & 0x0f;
		if (n <= 9)	result += '0' + n;
		else		result += 'a' + n - 10;
		n = (*it >> 0) & 0x0f;
		if (n <= 9)	result += '0' + n;
		else		result += 'a' + n - 10;
		n = (*it >> 12) & 0x0f;
		if (n <= 9)	result += '0' + n;
		else		result += 'a' + n - 10;
		n = (*it >> 8) & 0x0f;
		if (n <= 9)	result += '0' + n;
		else		result += 'a' + n - 10;
	}

	result.resize(4096, '0');
	return result;
}

errno_t LoadExoFile(std::string filename, ExoFile &output)
{
	std::string text;
	auto err = LoadTextA(filename, text);
	if (0 != err)
	{
		return err;
	}

	ExoFile exo;
	KeyStores *stores = &exo.exedit;

	std::smatch match;

	auto lines = split(text, "\n");
	for (auto &it : lines)
	{
		it = trim(it);
		if (!it.empty() && '[' == it.c_str()[0])
		{
			if (std::regex_match(it, match, std::regex(R"(\[(\d+)\])")))
			{
				//	"[0]"系の、数字が1つのパターン.
				exo.objects.push_back(ExoObject());
				stores = &exo.objects.back().header;
			}
			else if (std::regex_match(it, match, std::regex(R"(\[(\d+)\.(\d+)\])")))
			{
				//	"[0.0]"系の、数字が2つのパターン.
				exo.objects.back().params.push_back(KeyStores());
				stores = &exo.objects.back().params.back();
			}
		}
		else if (!it.empty())
		{
			auto pos = it.find('=');
			if (std::string::npos == pos)
			{
				continue;
			}

			std::string key = it.substr(0, pos);
			std::string value = it.substr(pos + 1);
			stores->set(key, value);
		}
	}

	output = std::move(exo);
	return 0;
}

errno_t SaveExoFile(std::string filename, const ExoFile &input)
{
	FILE *fw = nullptr;
	if (fopen_s(&fw, filename.c_str(), "w"))
	{
		return 1;
	}
	::fputs("[exedit]\n", fw);
	for (auto it : input.exedit.datas)
	{
		::fprintf(fw, "%s=%s\n", it.key.c_str(), it.value.c_str());
	}

	int noObj = 0;
	for (auto itObj : input.objects)
	{
		::fprintf(fw, "[%d]\n", noObj);
		for (auto it : itObj.header.datas)
		{
			::fprintf(fw, "%s=%s\n", it.key.c_str(), it.value.c_str());
		}
		int noParam = 0;
		for (auto itParam : itObj.params)
		{
			::fprintf(fw, "[%d.%d]\n", noObj, noParam);
			for (auto it : itParam.datas)
			{
				::fprintf(fw, "%s=%s\n", it.key.c_str(), it.value.c_str());
			}
			++noParam;
		}
		++noObj;
	}
	fclose(fw);
	return 0;
}

void SortExoObjects(std::vector<ExoObject> &objects)
{
	struct Compare
	{
		static bool comp(const ExoObject& left, const ExoObject& right)
		{
			int layerLeft = ::atoi(left.header.get("layer")->c_str());
			int layerRight = ::atoi(right.header.get("layer")->c_str());
			if (layerLeft < layerRight)
			{
				return true;
			}
			else if (layerLeft == layerRight)
			{
				return (::atoi(left.header.get("start")->c_str()) < ::atoi(right.header.get("start")->c_str()));
			}
			return false;
		}
	};
	std::sort(objects.begin(), objects.end(), Compare::comp);
}


void SortExoObjectsByStart(std::vector<ExoObject> &objects)
{
	struct Compare
	{
		static bool comp(const ExoObject& left, const ExoObject& right)
		{
			int startLeft = ::atoi(left.header.get("start")->c_str());
			int startRight = ::atoi(right.header.get("start")->c_str());
			if (startLeft < startRight)
			{
				return true;
			}
			else if (startLeft == startRight)
			{
				return (::atoi(left.header.get("layer")->c_str()) < ::atoi(right.header.get("layer")->c_str()));
			}
			return false;
		}
	};
	std::sort(objects.begin(), objects.end(), Compare::comp);
}