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
		Parse::Parsed status;
		FuncDecl func;
		TypeDecl type;

		while ((status = Parse::parseToplevel(lex, func, type))
					!= Parse::Nothing)
		{
			if (status == Parse::ParsedType)
				continue;

			std::cout
				<< "name: " << func.name << std::endl
				<< " sig: " << func.signature->string() << std::endl
				<< "body: " << std::endl << func.body->string() << std::endl;
		}

		lex.expect(tEOF);

		return 0;
	}
	catch (std::exception& err)
	{
		std::cout << "error: " << err.what() << std::endl;
		return 1;
	}
}