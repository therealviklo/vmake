#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif

enum struct LanguageMode
{
	c,
	cpp
};

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
	using namespace std::string_literals;
	for (const auto& f : std::filesystem::recursive_directory_iterator(dir))
	{
		if (!f.is_directory())
		{
			const auto& path = f.path();
			const auto pathString = path.string();
			if (pathString != "."s && pathString != ".."s)
			{
				if (endsWith(pathString, mode == LanguageMode::c ? ".c" : ".cpp"))
				{
					ss << " \"" << (dir / path) << "\"";
				}
			}
		}
	}
}

int main(int argc, char* argv[])
{
	try
	{
#ifdef _WIN32
		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);
#endif
		
		LanguageMode mode = LanguageMode::cpp;
		std::string token = "NAMN";
		int bells = 0;
		int bellInterval = 900;

		int currArgument = 1;

		// Hantera flaggor
		for (; currArgument < argc && (argv[currArgument][0] == '/' || argv[currArgument][0] == '-'); currArgument++)
		{
			if (std::strlen(argv[currArgument]) != 2)
			{
				std::cerr << "Ogiltigt argument: " << argv[currArgument] << '\n';
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
					<< '\n';
					return 0;
				}
				break;
				case 'c':
				case 'C':
				{
					mode = LanguageMode::c;
				}
				break;
				case 't':
				case 'T':
				{
					currArgument++;
					if (currArgument >= argc)
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << '\n';
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
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << '\n';
						return 1;
					}

					std::size_t charsRead = 0;
					try
					{
						bellInterval = std::stoi(argv[currArgument], &charsRead);
					}
					catch (std::invalid_argument)
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << '\n';
						std::cerr << "(Antalet millisekunder är inte ett giltigt heltal)" << '\n';
						return 1;
					}
					catch (std::out_of_range)
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << '\n';
						std::cerr << "(Antalet millisekunder får inte plats i en int)" << '\n';
						return 1;
					}

					// Ser till att det inte finns text efter talet
					if (charsRead != std::strlen(argv[currArgument]))
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << '\n';
						std::cerr << "(Antalet millisekunder är inte ett giltigt heltal)" << '\n';
						return 1;
					}

					if (bellInterval < 0)
					{
						std::cerr << "Felaktig användning av argument: " << argv[currArgument - 1] << '\n';
						std::cerr << "(Antalet millisekunder är mindre än noll)" << '\n';
						return 1;
					}
				}
				break;
				default:
				{
					std::cerr << "Ogiltigt argument: " << argv[currArgument] << '\n';
					return 1;
				}
			}
		}

		std::stringstream ss;
		addFilenames(ss, std::filesystem::current_path().string(), mode);

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
			if (i > 0) std::this_thread::sleep_for(std::chrono::milliseconds(bellInterval));
#ifdef _WIN32
			MessageBeep(MB_OK);
#else
			std::cout << '\a';
#endif
		}

		return returnValue;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fel: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "Okänt fel" << '\n';
	}
	return 1;
}