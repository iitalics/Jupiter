#include "Desugar.h"


Desugar::~Desugar () {}


ExpPtr Desugar::desugar (ExpPtr exp, LocEnvPtr lenv)
{
//	std::cout << ":: desugar " << exp->string(true) << std::endl;
	switch (exp->kind)
	{
	case eVar:
		return desugarVar(exp, lenv);
	case eTuple:
		return desugarTuple(exp, lenv);
	case eCall:
		return desugarCall(exp, lenv);
	case eInfix:
		return desugarInfix(exp, lenv);
	case eCond:
		return desugarCond(exp, lenv);
	case eBlock:
		return desugarBlock(exp, lenv);

	case eLet:
		throw exp->span.die("unexpected let at this location");

	case eInt:
	case eReal:
	case eString:
	case eBool:
	default:
		return desugarSubexps(exp, lenv);
	}
}
ExpPtr Desugar::desugarSubexps (ExpPtr e, LocEnvPtr lenv)
{
	if (e->subexps.size() == 0)
		return e;
	else
		return e->mapSubexps([=] (ExpPtr e2) 
		{
			return desugar(e2, lenv);
		});
}

ExpPtr Desugar::desugarVar (ExpPtr e, LocEnvPtr lenv)
{
	auto var = lenv->get(e->getString());

	if (var == nullptr)
		e->set<int>(-1); // TODO: look through global env?
	else
		e->set<int>(var->idx);

	return e;
}

ExpPtr Desugar::desugarTuple (ExpPtr e, LocEnvPtr lenv)
{
	// if (e->subexps.size() == 0) // TODO: unit?

	if (e->subexps.size() == 1)
		return desugar(e->subexps[0], lenv);
	else
		return desugarSubexps(e, lenv);
}
ExpPtr Desugar::desugarCall (ExpPtr e, LocEnvPtr lenv)
{
	return desugarSubexps(e, lenv);
}
ExpPtr Desugar::desugarCond (ExpPtr e, LocEnvPtr lenv)
{
	return desugarSubexps(e, lenv);
}
ExpPtr Desugar::desugarBlock (ExpPtr e, LocEnvPtr lenv)
{
	auto newenv = LocEnv::make(lenv);

	return e->mapSubexps([&] (ExpPtr e2)
	{
		ExpPtr res;

		if (e2->kind == eLet)
			res = desugarLet(e2, newenv);
		else
			res = desugar(e2, newenv);

		return res;
	});
}
ExpPtr Desugar::desugarLet (ExpPtr e, LocEnvPtr lenv)
{
	// TODO: destructuring let
	auto name = e->getString();

	if (lenv->has(name))
		throw e->span.die("variable with same name already declared in scope");

	auto res = desugarSubexps(e, lenv);
	
	// letrec?
	lenv->newVar(e->getString());
	return res;
}
ExpPtr Desugar::desugarInfix (ExpPtr e, LocEnvPtr lenv)
{
	return desugarSubexps(e, lenv);
}