#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std::literals;

bool stringInVector(const std::string& str, const std::vector<std::string>& v)
{
	return std::find(v.begin(), v.end(), str) != v.end();
}

bool isModified(std::filesystem::file_time_type objt, const std::filesystem::path& file)
{
	auto loop = [](
		auto loop, 
		std::filesystem::file_time_type objt,
		const std::filesystem::path& file,
		std::vector<std::filesystem::path>& cache
	) -> bool {
		if (std::find(cache.begin(), cache.end(), file) != cache.end())
			return false;
		cache.push_back(file);
		if (objt < std::filesystem::last_write_time(file))
			return true;

		std::vector<char> fc;
		size_t size = 0;
		{
			std::ifstream fs(file, std::ios::binary);
			fs.seekg(0, std::ios::end);
			size = fs.tellg();
			fc.resize(size);
			fs.seekg(0, std::ios::beg);
			fs.read(&fc[0], size);
		}

		std::vector<std::filesystem::path> deps;
		size_t pos = 0;
		auto skipRest = [&]{
			while (pos < size)
			{
				if (fc[pos] == '\n')
				{
					pos++;
					break;
				}
				else if (fc[pos] == '\r')
				{
					pos++;
					if (pos < size && fc[pos] == '\n') pos++;
					break;
				}
				else
				{
					pos++;
				}
			}
		};
		auto skipSeq = [&](const char* seq) -> bool {
			while (pos < size)
			{
				if (*seq == '\0') return true;
				if (*seq != fc[pos]) return false;
				seq++;
				pos++;
			}
			return false;
		};
		auto skipWS = [&]{
			while (pos < size && (
				fc[pos] == ' ' ||
				fc[pos] == '\t'
			)) pos++;
		};
		if (!skipSeq("\xEF\xBB\xBF")) pos = 0;
		while (pos < size)
		{
			if (fc[pos++] == '#')
			{
				skipWS();
				if (skipSeq("include"))
				{
					skipWS();
					if (skipSeq("\""))
					{
						std::string fn;
						while (pos < size && fc[pos] != '\"')
						{
							fn += fc[pos++];
						}
						if (pos < size)
						{
							deps.emplace_back(std::filesystem::path(fn));
						}
					}
				}
			}
			skipRest();
		}

		for (const auto& i : deps)
		{
			if (loop(loop, objt, file.parent_path() / i, cache))
				return true;
		}

		return false;
	};
	std::vector<std::filesystem::path> cache;
	return loop(loop, objt, file, cache);
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif

	bool verbose = true;

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
						"    -q           Skriv inte ut något i konsolen.\n"
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
					objs += " \"";
					objs += argv[arg];
					objs += '\"';
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
					srcArgs += " \"";
					srcArgs += argv[arg];
					srcArgs += '\"';
				}
				break;
				case 'L':
				{
					advanceArg();
					lnkArgs += " \"";
					lnkArgs += argv[arg];
					lnkArgs += '\"';
				}
				break;
				case 'q':
				{
					verbose = false;
				}
				break;
				default:
				{
					if (verbose) std::printf("Okänt argument: %s\n", argv[arg]);
				}
				return EXIT_FAILURE;
			}
		}

		if (arg >= argc)
		{
			if (verbose) std::puts("Ingen kompilator angavs.");
			return EXIT_FAILURE;
		}
		const std::string compiler = argv[arg++];

		std::string compArgs;
		for (; arg < argc; arg++)
		{
			compArgs += ' ';
			compArgs += argv[arg];
		}

		size_t totFiles = 0;
		std::vector<std::filesystem::directory_entry> files;
		for (const auto& i : std::filesystem::recursive_directory_iterator("."))
		{
			const std::string ext = i.path().extension().string();
			if (ext.size() > 1)
			{
				if (stringInVector(ext, extensions) &&
					!stringInVector(std::filesystem::canonical(i.path()).string(), skipPaths))
				{
					const std::string obji = (i.path().parent_path() / i.path().stem()).string() + objectExt;
					objs += " \"";
					objs += obji;
					objs += '\"';

					totFiles++;
					if (!std::filesystem::exists(obji) || isModified(std::filesystem::last_write_time(obji), i))
					{
						files.emplace_back(i.path());
					}
				}
			}
		}

		std::printf("%zu av %zu filer måste kompileras.\n", files.size(), totFiles);

		for (size_t i = 0; i < files.size(); i++)
		{
			const std::string obji = (files[i].path().parent_path() / files[i].path().stem()).string() + objectExt;

			if (verbose) std::printf("(%zu/%zu) %s\n", i + 1, files.size(), files[i].path().string().c_str());
			std::system((
				compiler +
				" \""s +
				files[i].path().string() +
				"\" -c -o \""s +
				obji +
				"\""s +
				compArgs +
				srcArgs
			).c_str());
		}

		if (verbose) std::puts("Länkar");

		std::system((
			compiler +
			objs +
			(output.empty() ? ""s : " -o \""s + output + "\""s) +
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
		if (verbose) std::printf("Fel: %s\n", e.what());
		return EXIT_FAILURE;
	}
	catch (...)
	{
		if (verbose) std::puts("Okänt fel");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}