#include <iostream>
#include "Jupiter.h"
#include "Ast.h"
#include "Types.h"
#include "Desugar.h"

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

	GlobEnv env;
	using Op = GlobEnv::OpPrecedence;

	env.operators.push_back(Op("+", 7, Assoc::Left));
	env.operators.push_back(Op("-", 7, Assoc::Left));
	env.operators.push_back(Op("*", 6, Assoc::Left));
	env.operators.push_back(Op("/", 6, Assoc::Left));
	env.operators.push_back(Op("^", 5, Assoc::Right));
	env.operators.push_back(Op("::", 4, Assoc::Right));

	auto Int = Ty::makeConcrete("Int");
	auto Bool = Ty::makeConcrete("Bool");

	env.addFunc("+")->instances.push_back(FuncInstance {
		"+",
		SigPtr(new Sig({ { "x", Int }, { "y", Int } })), Int
	});

	env.addFunc("<")->instances.push_back(FuncInstance {
		"<",
		SigPtr(new Sig({ { "x", Int }, { "y", Int } })), Bool
	});

	env.addFunc("==")->instances.push_back(FuncInstance {
		"==",
		SigPtr(new Sig({ { "x", Int }, { "y", Int } })), Bool
	});


	try
	{
		Lexer lex;
		lex.openFile("test.j");
		
		env.loadToplevel(Parse::parseToplevel(lex));
		lex.expect(tEOF);

		return 0;
	}
	catch (std::exception& err)
	{
		std::cout << "error: " << err.what() << std::endl;
		return 1;
	}
}