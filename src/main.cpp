#include <iostream>
#include "Build.h"

#define VERSION   "jupiter version 0.0.5 dev"
#define COPYRIGHT "copyright (C) 2015 iitalics"

static int Main (std::vector<std::string> args);

int main (int argc, char** argv)
{
	std::vector<std::string> args;
	args.reserve(argc);

	for (int i = 0; i < argc; i++)
		args.push_back(argv[i]);

	return Main(std::move(args));
}
static int Main (std::vector<std::string> args)
{
	if (args.size() > 1 && args[1] == "-keymaps")
	{
		Lexer::generateKeyMaps();
		return 0;
	}

	if (args.size() <= 1)
	{
		std::cout << "Usage: jup <entry> [-B <build path>] <paths>..." << std::endl
		          << std::endl
			  << VERSION << std::endl
			  << COPYRIGHT << std::endl;
		return 1;
	}

	

	try
	{
		std::string buildPath("");
		std::vector<std::string> paths;
		for (size_t i = 2, len = args.size(); i < len; i++)
			if (args[i] == "-B")
				buildPath = args[++i];
			else
				paths.push_back(args[i]);

		Build build(buildPath);
		for (auto& path : paths)
			build.addPath(path);
		build.createEntryPoint(args[1]);
		build.finishModuleLoad();
		build.compile(std::cout);

		return 0;
	}
	catch (Span::Error& err)
	{
		std::cerr << err.what();
		return -1;
	}
}
