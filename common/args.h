#pragma once
/*
起動引数を扱いやすくするための関数群
*/

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

/**
*.exe infile [-o outfile][-np]

*/
int args(int argc, char **argv, std::vector<std::string> &inputs, std::unordered_map<std::string, std::string> &options, std::unordered_set<std::string> singles, std::unordered_set<std::string> doubles)
{
	if (argc <= 1)
	{
		AutoPause::init();
		return 0;
	}

	int err = 0;
	for (int i = 1; i < argc; ++i)
	{
		if ('-' != argv[i][0])
		{
			inputs.push_back(argv[i]);
			continue;
		}

		if (singles.end() != singles.find(argv[i]))
		{
			options.insert(std::pair< std::string, std::string>(argv[i], "true"));
		}
		else if (doubles.end() != doubles.find(argv[i]) && (i + 1) < argc)
		{
			options.insert(std::pair< std::string, std::string>(argv[i], argv[i + 1]));
			++i;
		}
		else
		{
			++err;
		}
	}

	if (options.end() == options.find("-np"))
	{
		//	NoPauseオプションがついていない場合、AutoPauseを使って入力待ちにする.
		//	これはユーザーがエラーメッセージを確認できるようにするための機能.
		AutoPause::init();
	}
	return err;
}
