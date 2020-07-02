#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <windows.h>

enum LanguageMode
{
	M_C,
	M_CPP
};

std::string getCurrentDirectory()
{
	char pathname[MAX_PATH] = {};
	GetCurrentDirectoryA(MAX_PATH, pathname);
	return pathname;
}

bool endsWith(const std::string& str, const std::string& ending)
{
	auto c = str.rbegin();
	for (auto ec = ending.rbegin(); ec != ending.rend(); ec++)
	{
		if (c == str.rend()) return false;

		if (*c != *ec) return false;

		c++;
	}
	return true;
}

void addFilenames(std::stringstream& ss, const std::string& dir, LanguageMode mode)
{
	WIN32_FIND_DATAA findData;
	HANDLE findHandle = FindFirstFileA((dir + "\\*").c_str(), &findData);
	if (findHandle == INVALID_HANDLE_VALUE) return;

	do
	{
		if (findData.cFileName != std::string(".") && findData.cFileName != std::string(".."))
		{
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				addFilenames(ss, dir + "\\" + findData.cFileName, mode);
			}
			else if (endsWith(findData.cFileName, mode == M_C ? ".c" : ".cpp"))
			{
				ss << " \"" << dir << "\\" << findData.cFileName << "\"";
			}
		}
	} while (FindNextFileA(findHandle, &findData));

	FindClose(findHandle);
}

int main(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "/h") == 0 ||
			strcmp(argv[i], "-h") == 0 ||
			strcmp(argv[i], "/H") == 0 ||
			strcmp(argv[i], "-H") == 0 ||
			strcmp(argv[i], "/?") == 0 ||
			strcmp(argv[i], "-?") == 0)
		{
			std::cout
			<< "Syntax:\n"
			<< "    vmake [flags] command_before_files [command_after_files] [flags]\n"
			<< "Flags:\n"
			<< "    -h, -?  Show help\n"
			<< "    -c      Search for .c files instead of .cpp files"
			<< std::endl;
			return 0;
		}
	}

	LanguageMode mode;
	int commandPos;
	bool endString;
	if (argc == 4)
	{
		endString = true;
		if (strcmp(argv[1], "/c") == 0 || strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "/C") == 0 || strcmp(argv[1], "-C") == 0)
		{
			commandPos = 2;
			mode = M_C;
		}
		else if (strcmp(argv[3], "/c") == 0 || strcmp(argv[3], "-c") == 0 || strcmp(argv[3], "/C") == 0 || strcmp(argv[3], "-C") == 0)
		{
			commandPos = 1;
			mode = M_C;
		}
		else
		{
			std::cerr << "Invalid syntax" << std::endl;
			return 1;
		}
	}
	else if (argc == 3)
	{
		if (strcmp(argv[1], "/c") == 0 || strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "/C") == 0 || strcmp(argv[1], "-C") == 0)
		{
			endString = false;
			commandPos = 2;
			mode = M_C;
		}
		else if (strcmp(argv[2], "/c") == 0 || strcmp(argv[2], "-c") == 0 || strcmp(argv[2], "/C") == 0 || strcmp(argv[2], "-C") == 0)
		{
			endString = false;
			commandPos = 1;
			mode = M_C;
		}
		else
		{
			endString = true;
			commandPos = 1;
			mode = M_CPP;
		}
	}
	else if (argc == 2)
	{
		endString = false;
		commandPos = 1;
		mode = M_CPP;
	}
	else
	{
		std::cerr << "Invalid number of arguments" << std::endl;
		return 1;
	}

	std::stringstream ss;
	addFilenames(ss, getCurrentDirectory(), mode);

	if (endString)
		system((argv[commandPos] + ss.str() + " " + argv[commandPos + 1]).c_str());
	else
		system((argv[commandPos] + ss.str()).c_str());

	return 0;
}