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

	auto env = std::make_shared<GlobEnv>();
	using Op = GlobEnv::OpPrecedence;

	env->operators.push_back(Op("+", 7, Assoc::Left));
	env->operators.push_back(Op("-", 7, Assoc::Left));
	env->operators.push_back(Op("*", 6, Assoc::Left));
	env->operators.push_back(Op("/", 6, Assoc::Left));
	env->operators.push_back(Op("^", 5, Assoc::Right));
	env->operators.push_back(Op("::", 4, Assoc::Right));

	try
	{
		Lexer lex;
		lex.openFile("test.j");

		auto e = Parse::parseExp(lex);
		lex.expect(tEOF);

		std::cout << e->string(true) << std::endl;

		Desugar des(env);
		auto ed = des.desugar(e, LocEnv::make());

		std::cout << ed->string() << std::endl;

		return 0;
	}
	catch (std::exception& err)
	{
		std::cout << "error: " << err.what() << std::endl;
		return 1;
	}
}