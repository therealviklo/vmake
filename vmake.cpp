#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <windows.h>
#include <stdexcept>

enum LanguageMode
{
	M_C,
	M_CPP
};

std::string getCurrentDirectory()
{
	char pathname[MAX_PATH] = {};
	if (GetCurrentDirectoryA(MAX_PATH, pathname) == 0) throw std::runtime_error("Unable to get current directory");
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
	try
	{
		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);
		
		LanguageMode mode = M_CPP;
		std::string token = "NAMN";
		int bells = 0;
		int bellInterval = 900;

		int currArgument = 1;

		// Hantera flaggor
		for (; currArgument < argc && (argv[currArgument][0] == '/' || argv[currArgument][0] == '-'); currArgument++)
		{
			if (strlen(argv[currArgument]) != 2)
			{
				std::cerr << "Ogiltigt argument: " << argv[currArgument] << std::endl;
				return 1;
			}

			switch (argv[currArgument][1])
			{
				case 'h':
				case 'H':
				case '?':
				{
					std::cout
					<< "Syntax:\n"
					<< "    vmake [flaggor] kommando\n"
					<< "Flaggor:\n"
					<< "    -h, -?   Visa hjälp.\n"
					<< "    -c       Leta efter .c-filer istället för .cpp-filer.\n"
					<< "    -t TOKEN Byt ut TOKEN mot listan på de filer som har hittats.\n"
					<< "             Om denna flagga utelämnas används \"NAMN\".\n"
					<< "    -b       Spela ett ljud när kommandot har utförts.\n"
					<< "             Om denna flagga anges flera gånger spelas ljudet\n"
					<< "             flera gånger.\n"
					<< "    -i MS    Om -b anges flera gånger vänter vmake MS\n"
					<< "             millisekunder mellan varje ljud. Default är\n"
					<< "             900 millisekunder."
					<< std::endl;
					return 0;
				}
				break;
				case 'c':
				case 'C':
				{
					mode = M_C;
				}
				break;
				case 't':
				case 'T':
				{
					currArgument++;
					if (currArgument >= argc)
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << std::endl;
						return 1;
					}
					token = argv[currArgument];
				}
				break;
				case 'b':
				case 'B':
				{
					bells++;
				}
				break;
				case 'i':
				case 'I':
				{
					currArgument++;
					if (currArgument >= argc)
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << std::endl;
						return 1;
					}

					std::size_t charsRead = 0;
					try
					{
						bellInterval = std::stoi(argv[currArgument], &charsRead);
					}
					catch (std::invalid_argument)
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << std::endl;
						std::cerr << "(Antalet millisekunder är inte ett giltigt heltal)" << std::endl;
						return 1;
					}
					catch (std::out_of_range)
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << std::endl;
						std::cerr << "(Antalet millisekunder får inte plats i en int)" << std::endl;
						return 1;
					}

					// Ser till att det inte finns text efter talet
					if (charsRead != strlen(argv[currArgument]))
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << std::endl;
						std::cerr << "(Antalet millisekunder är inte ett giltigt heltal)" << std::endl;
						return 1;
					}

					if (bellInterval < 0)
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << std::endl;
						std::cerr << "(Antalet millisekunder är mindre än noll)" << std::endl;
						return 1;
					}
				}
				break;
				default:
				{
					std::cerr << "Ogiltigt argument: " << argv[currArgument] << std::endl;
					return 1;
				}
			}
		}

		std::stringstream ss;
		addFilenames(ss, getCurrentDirectory(), mode);

		std::stringstream command;
		if (currArgument < argc)
		{
			command << argv[currArgument] << " ";
			currArgument++;
		}
		for (; currArgument < argc; currArgument++)
		{
			if (argv[currArgument] == token)
			{
				command << ss.str() << " ";
			}
			else
			{
				command << "\"" << argv[currArgument] << "\" ";
			}
		}

		int returnValue = system(command.str().c_str());

		for (int i = 0; i < bells; i++)
		{
			if (i > 0) Sleep(bellInterval);
			MessageBeep(MB_OK);
		}

		return returnValue;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fel: " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cerr << "Okänt fel" << std::endl;
	}
	return 1;
}