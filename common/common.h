#pragma once
/*
便利関数などを集めた.
*/

#include <string>
#include <vector>

std::vector<std::string> split(std::string text, std::string separator)
{
	std::vector<std::string> result;

	auto offset = std::string::size_type(0);
	while (true)
	{
		auto pos = text.find(separator, offset);
		if (std::string::npos == pos)
		{
			result.push_back(text.substr(offset));
			break;
		}
		result.push_back(text.substr(offset, pos - offset));
		offset = pos + separator.length();
	}
	return result;
}

std::string trim(const std::string &string, const char *trimCharacterList = " \t\f\v\r\n")
{
	std::string result;
	std::string::size_type left = string.find_first_not_of(trimCharacterList);

	if (std::string::npos != left)
	{
		std::string::size_type right = string.find_last_not_of(trimCharacterList);
		return string.substr(left, right - left + 1);
	}
	return result;
}

std::string replaceAll(std::string text, std::string from, std::string to){

	size_t pos = 0;
	while (std::string::npos != (pos = text.find(from, pos)))
	{
		text.replace(pos, from.length(), to);
		pos += to.length();
	}
	return text;
}
std::wstring replaceAll(std::wstring text, std::wstring from, std::wstring to) {

	size_t pos = 0;
	while (std::wstring::npos != (pos = text.find(from, pos)))
	{
		text.replace(pos, from.length(), to);
		pos += to.length();
	}
	return text;
}

errno_t LoadText(std::string filename, std::string &output)
{
	FILE *fr = nullptr;
	if (fopen_s(&fr, filename.c_str(), "r"))
	{
		return 1;
	}
	std::string text;
	char buffer[1024];
	while (nullptr != fgets(buffer, sizeof(buffer), fr))
	{
		text += buffer;
	}
	fclose(fr);
	output = text;
	return 0;
}

errno_t SaveText(const std::string &filename, const std::string &text)
{
	FILE *fw = nullptr;
	if (fopen_s(&fw, filename.c_str(), "w"))
	{
		return 1;
	}
	fwrite(text.c_str(), text.size(), 1, fw);
	fclose(fw);
	return 0;
}

/**
setting系ファイルのあるフォルダパスを取得する.
EXEファイルの存在するディレクトリがそれにあたる.
ただしデバッグ中はカレントディレクトリにする. デバッグをしやすくための処置.

@return フォルダパス.
*/
std::string GetSettingFileDir()
{
#ifdef _DEBUG
	char path[32768];
	::GetCurrentDirectoryA(sizeof(path), path);
	return path;
#else
	char path[32768];
	::GetModuleFileNameA(nullptr, path, sizeof(path));
	std::string exe = path;
	return exe.substr(0, exe.find_last_of('\\'));
#endif
}

std::string GetTimeString()
{
	char path[32768];
	SYSTEMTIME now;
	GetLocalTime(&now);
	sprintf_s(path, "%04d-%02d-%02d_%02d-%02d-%02d", now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
	return path;
}

/*
デストラクタで入力待ちを行う.
initを呼び出しておくと、アプリケーション終了時に入力待ちを発生させることができる.
*/
struct AutoPause
{
	static void init()
	{
		static std::unique_ptr<AutoPause> pause;
		pause = std::unique_ptr<AutoPause>(new AutoPause);
	}
	~AutoPause()
	{
		::getc(stdin);
	}
};

std::wstring StringToWString(std::string text)
{
	std::wstring result;
	int n = ::MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, NULL, 0);
	if (0 == n)
	{
		return result;
	}

	auto p = std::unique_ptr<wchar_t[]>(new wchar_t[n]);
	n = ::MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, p.get(), n);
	if (0 == n)
	{
		return result;
	}
	result = p.get();
	return result;
}

std::string WStringToString(std::wstring text)
{
	std::string result;
	int n = ::WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, NULL, 0, NULL, NULL);
	if (0 == n)
	{
		return result;
	}

	auto p = std::unique_ptr<char[]>(new char[n]);
	n = ::WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, p.get(), n, NULL, NULL);
	if (0 == n)
	{
		return result;
	}
	result = p.get();
	return result;
}