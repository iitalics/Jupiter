#include <iostream>
#include "Jupiter.h"
#include "Ast.h"
#include "Types.h"
#include "Desugar.h"
#include "Compiler.h"

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
	Compiler compiler;
	using Op = GlobEnv::OpPrecedence;

	env.operators.push_back(Op("::", 80, Assoc::Right));
	env.operators.push_back(Op("+",  70, Assoc::Left));
	env.operators.push_back(Op("-",  70, Assoc::Left));
	env.operators.push_back(Op("*",  60, Assoc::Left));
	env.operators.push_back(Op("/",  60, Assoc::Left));
	env.operators.push_back(Op("^",  50, Assoc::Right));

	auto Int = Ty::makeConcrete("Int");
	auto Real = Ty::makeConcrete("Real");
	auto Bool = Ty::makeConcrete("Bool");
	auto String = Ty::makeConcrete("String");
	auto Unit = Ty::makeUnit();
	auto polyA = Ty::makePoly();
	auto polyB = Ty::makePoly();
	auto polyList = Ty::makeConcrete("List", { polyA });

	env.bake(&compiler, "juStd_addInt",       "+", { Int, Int }, Int);
	env.bake(&compiler, "juStd_negInt",       "-", { Int }, Int);
	env.bake(&compiler, "juStd_ltInt",        "<", { Int, Int }, Bool);
	env.bake(&compiler, "juStd_eqInt",        "==", { Int, Int }, Bool);
	env.bake(&compiler, "juStd_printString",  "print", { String }, Unit);
	env.bake(&compiler, "juStd_printInt",     "print", { Int }, Unit);
	env.bake(&compiler, "juStd_printReal",    "print", { Real }, Unit);
	env.bake(&compiler, "juStd_println",      "println", { }, Unit);

	try
	{
		Lexer lex;
		lex.openFile("test.j");
		
		env.loadToplevel(Parse::parseToplevel(lex));
		lex.expect(tEOF);

		auto entrySpan = Span();
		auto entrySig = Sig::make();
		auto entryBody =
			Exp::make(eCall,
				{ Exp::make(eVar, "main", int(-1), {}, entrySpan) },
				entrySpan);
		auto entryOverload =
			Overload::make(
				env,
				"#entry",
				entrySig,
				entryBody);

		auto cunit = compiler.compile(entryOverload, entrySig);
		compiler.entryPoint(cunit);
		cunit->compile();

		std::ostringstream ss;
		compiler.output(ss);
		std::cout << ss.str() << std::endl;


		return 0;
	}
	catch (std::exception& err)
	{
		std::cerr << "error: " << err.what() << std::endl;
		return 1;
	}
}