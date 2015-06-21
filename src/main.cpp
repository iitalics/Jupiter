#include <iostream>
#include "Jupiter.h"
#include "Ast.h"
#include "Types.h"
#include "Desugar.h"
#include "Compiler.h"

#define VERSION   "jupiter version 0.0.5 dev"
#define COPYRIGHT "copyright (C) 2015 iitalics"

static int Main (std::vector<std::string>&& args);

int main (int argc, char** argv)
{
	std::vector<std::string> args;
	args.reserve(argc);

	for (int i = 0; i < argc; i++)
		args.push_back(argv[i]);

	return Main(std::move(args));
}

static int Main (std::vector<std::string>&& args)
{
	if (args.size() > 1 && args[1] == "-keymaps")
	{
		Lexer::generateKeyMaps();
		return 0;
	}

	if (args.size() == 1)
	{
		std::cout << "Usage: jup file1 [, file2...]" << std::endl
		          << std::endl
			  << VERSION << std::endl
			  << COPYRIGHT << std::endl;
		return 1;
	}

	GlobEnv env;
	using Op = GlobEnv::OpPrecedence;

	// TODO: integrate order-of-ops into jupiter syntax
	env.operators.push_back(Op("==", 90, Assoc::Left));
	env.operators.push_back(Op("!=", 90, Assoc::Left));
	env.operators.push_back(Op(">=", 90, Assoc::Left));
	env.operators.push_back(Op("<=", 90, Assoc::Left));
	env.operators.push_back(Op(">", 90, Assoc::Left));
	env.operators.push_back(Op("<", 90, Assoc::Left));
	env.operators.push_back(Op("::", 80, Assoc::Right));
	env.operators.push_back(Op("+",  70, Assoc::Left));
	env.operators.push_back(Op("-",  70, Assoc::Left));
	env.operators.push_back(Op("*",  60, Assoc::Left));
	env.operators.push_back(Op("/",  60, Assoc::Left));
	env.operators.push_back(Op("%",  60, Assoc::Left));
	env.operators.push_back(Op("^",  50, Assoc::Right));


	try
	{
		env.loadStdlib();

		for (size_t i = 1, len = args.size(); i < len; i++)
			env.loadToplevel(args[i]);

		auto entrySpan = Span();
		auto entrySig = Sig::make();
		auto entryBody =
			Exp::make(eCall,
				{ Exp::make(eVar, "main", bool(false), {}, entrySpan) },
				entrySpan);
		try
		{
			Desugar entryDesugar(env);
			entryBody = entryDesugar.desugar(entryBody, LocEnv::make());
		}
		catch (Span::Error& err)
		{
			throw entrySpan.die("no entry point function 'main' defined");
		}

		auto entryOverload =
			Overload::make(
				env,
				"#entry",
				entrySig,
				entryBody,
				false);

		auto cunit = env.compiler->compile(entryOverload, entrySig);
		env.compiler->entryPoint(cunit);
		cunit->compile();

		env.compiler->output(std::cout);


		return 0;
	}
	catch (Span::Error& err)
	{
		std::cerr << err.what();
		return -1;
	}
}
