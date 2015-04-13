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

	env.addFunc("+")->instances.push_back(FuncInstance {
		SigPtr(new Sig({ { "x", Int }, { "y", Int } })), Int,
		"std_add_Int"
	});

	env.addFunc("<")->instances.push_back(FuncInstance {
		SigPtr(new Sig({ { "x", Int }, { "y", Int } })), Int,
		"std_less_Int"
	});

	env.addFunc("==")->instances.push_back(FuncInstance {
		SigPtr(new Sig({ { "x", Int }, { "y", Int } })), Int,
		"std_eql_Int"
	});

	try
	{
		Lexer lex;
		lex.openFile("test.j");
		
		auto proto = Parse::parseToplevel(lex);

		for (auto& fn : proto.funcs)
			env.addFunc(fn.name);

		for (auto& fn : proto.funcs)
		{
			Desugar des(env);
			fn = des.desugar(fn);

			auto globfn = env.getFunc(fn.name);

			globfn->overloads.push_back({
					env,
					fn.signature, 
					fn.body
				});
		}

		for (auto& fn : proto.funcs)
			std::cout << "func " << fn.name << " " << fn.signature->string() << std::endl
			          << fn.body->string() << std::endl;

		lex.expect(tEOF);

		return 0;
	}
	catch (std::exception& err)
	{
		std::cout << "error: " << err.what() << std::endl;
		return 1;
	}
}