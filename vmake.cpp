#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std::literals;

bool stringInVector(const std::string& str, const std::vector<std::string>& v)
{
	for (const auto& i : v)
	{
		if (i == str) return true;
	}
	return false;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif
	try
	{
		int arg = 1;
		auto advanceArg = [&arg, &argc]() -> void {
			if (++arg >= argc)
			{
				std::puts("Ogiltig användning av argument");
				std::exit(EXIT_FAILURE);
			}
		};

		bool extensionsChanged = false;
		std::vector<std::string> extensions = {".cpp"};
		std::string output;
		std::string objectExt = ".o";
		std::string objs;
		std::uintmax_t bells = 0;
		unsigned long long ms = 50;
		std::vector<std::string> skipPaths;
		std::string srcArgs;
		std::string lnkArgs;

		for (; arg < argc; arg++)
		{
			if (argv[arg][0] != '-') break;
			switch (argv[arg][1])
			{
				case 'h':
				case '?':
				{
					std::printf(
						"Syntax:\n"
						"    vmake [flaggor] kompilator [kompilatorflaggor]\n"
						"Flaggor:\n"
						"    -h, -?       Visa hjälp.\n"
						"    -o FILNAMN   Ange programmets namn. Default är\n"
						"                 kompilatorns default.\n"
						"    -e ÄNDELSE   Ange en filändelse som ska sökas\n"
						"                 efter. Default är \".cpp\".\n"
						"    -O ÄNDELSE   Ange objektfiländelsen. Default\n"
						"                 är \".o\".\n"
						"    -x FILNAMN   Lägg till en extra objektfil som\n"
						"                 också ska länkas in i programmet.\n"
						"    -s FILNAMN   Skippa en källkodsfil.\n"
						"    -S ARGUMENT  Ange ett argument som endast\n"
						"                 ska användas när källkodsfiler\n"
						"                 ska kompileras till\n"
						"                 objektsfiler.\n"
						"    -L ARGUMENT  Ange ett argument som endast\n"
						"                 ska användas när objektsfiler\n"
						"                 ska länkas till det slutgiltiga\n"
						"                 programmet.\n"
						"    -b           Spela ett plingljud när\n"
						"                 kompileringen är klar.\n"
						"    -i MS        Ange intervallet för plingningar\n"
						"                 från -b.\n"
					);
				}
				return EXIT_SUCCESS;
				case 'e':
				{
					advanceArg();
					if (!extensionsChanged)
					{
						extensionsChanged = true;
						extensions.clear();
					}
					extensions.emplace_back(argv[arg]);
				}
				break;
				case 'o':
				{
					advanceArg();
					output = argv[arg];
				}
				break;
				case 'O':
				{
					advanceArg();
					objectExt = argv[arg];
				}
				break;
				case 'x':
				{
					advanceArg();
					objs += ' ';
					objs += argv[arg];
				}
				break;
				case 'b':
				{
					bells++;
				}
				break;
				case 'i':
				{
					advanceArg();
					ms = std::stoull(argv[arg]);
				}
				break;
				case 's':
				{
					advanceArg();
					skipPaths.emplace_back(std::filesystem::canonical(std::filesystem::path(argv[arg])).string());
				}
				break;
				case 'S':
				{
					advanceArg();
					srcArgs += ' ';
					srcArgs += argv[arg];
				}
				break;
				case 'L':
				{
					advanceArg();
					lnkArgs += ' ';
					lnkArgs += argv[arg];
				}
				break;
				default:
				{
					std::printf("Okänt argument: %s\n", argv[arg]);
				}
				return EXIT_FAILURE;
			}
		}

		if (arg >= argc)
		{
			std::puts("Ingen kompilator specificerades.");
			return EXIT_FAILURE;
		}
		const std::string compiler = argv[arg++];

		std::string compArgs;
		for (; arg < argc; arg++)
		{
			compArgs += ' ';
			compArgs += argv[arg];
		}

		std::vector<std::filesystem::directory_entry> files;
		for (const auto& i : std::filesystem::recursive_directory_iterator("."))
		{
			const std::string ext = i.path().extension().string();
			if (ext.size() > 1)
			{
				if (stringInVector(ext, extensions) &&
					!stringInVector(std::filesystem::canonical(i.path()).string(), skipPaths))
				{
					files.emplace_back(i.path());
				}
			}
		}

		for (size_t i = 0; i < files.size(); i++)
		{
			const std::string obji = files[i].path().stem().string() + objectExt;
			objs += ' ';
			objs += obji;

			std::printf("(%zu/%zu) %s", i + 1, files.size(), files[i].path().string().c_str());
			if (!std::filesystem::exists(obji) || files[i].last_write_time() > std::filesystem::last_write_time(obji))
			{
				std::putchar('\n');
				std::system((
					compiler +
					" "s +
					files[i].path().string() +
					" -c -o "s +
					obji +
					compArgs +
					srcArgs
				).c_str());
			}
			else
			{
				std::puts(" (Skippar)");
			}
		}

		std::puts("Länkar");

		std::system((
			compiler +
			objs +
			(output.empty() ? ""s : " -o "s + output) +
			compArgs +
			lnkArgs
		).c_str());

		for (unsigned long long i = 0; i < bells; i++)
		{
			if (i > 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(ms));
			}
#ifdef _WIN32
			MessageBeep(MB_ICONINFORMATION);
#else
			std::putchar('\a');
#endif
		}
	}
	catch (const std::exception& e)
	{
		std::printf("Fel: %s\n", e.what());
		return EXIT_FAILURE;
	}
	catch (...)
	{
		std::puts("Okänt fel");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}