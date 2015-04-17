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
	auto String = Ty::makeConcrete("String");
	auto Unit = Ty::makeUnit();

	env.bake("+", { Int, Int }, Int);
	env.bake("-", { Int }, Int);
	env.bake("<", { Int, Int }, Bool);
	env.bake("==", { Int, Int }, Bool);
	env.bake("nil", { }, Ty::makeConcrete("List", {Ty::makePoly()}));
	env.bake("println", { String }, Unit);
	env.bake("string", { Ty::makePoly() }, String);

	try
	{
		Lexer lex;
		lex.openFile("test.j");
		
		env.loadToplevel(Parse::parseToplevel(lex));
		lex.expect(tEOF);

		auto toplevelSpan = Span();
		auto toplevelSig = Sig::make();

		auto mainCall = Exp::make(eCall,
							{ Exp::make(eVar, "main", int(-1), {}, toplevelSpan) },
							toplevelSpan);

		Desugar des(env);
		mainCall = des.desugar(mainCall, LocEnv::make());

		Infer inf({
				env,
				"#<toplevel>",
				toplevelSig,
				mainCall,
				{} },
			toplevelSig);

		std::cout << "-> " << inf.fn.returnType->string() << std::endl;

		return 0;
	}
	catch (std::exception& err)
	{
		std::cout << "error: " << err.what() << std::endl;
		return 1;
	}
}