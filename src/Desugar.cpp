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
	{
		auto e2 = e->mapSubexps([=] (ExpPtr e2) 
		{
			return desugar(e2, lenv);
		});

		if (e2->getType() != nullptr)
			e2->setType(desugar(e2->getType()));

		return e2;
	}
}

ExpPtr Desugar::desugarVar (ExpPtr e, LocEnvPtr lenv)
{
	auto var = lenv->get(e->getString());

	if (var == nullptr)
		return desugarGlobal(e);
	else
	{
		e->set<int>(var->idx);
		return e;
	}
}
ExpPtr Desugar::desugarGlobal (ExpPtr e)
{
	if (global.getFunc(e->getString()) == nullptr)
	{
		std::ostringstream ss;
		ss << "undeclared global \"" << e->getString() << "\"";
		throw e->span.die(ss.str());
	}

	e->set<int>(-1);
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

	return e->mapSubexps([=] (ExpPtr e2)
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
	auto newvar = lenv->newVar(e->getString());
	res->set<int>(newvar->idx);
	return res;
}








TyPtr Desugar::desugar (TyPtr ty)
{
	switch (ty->kind)
	{
	case tyWildcard:
		return Ty::makePoly();

	case tyConcrete:
		if (!ty->subtypes.nil())
			return Ty::makeConcrete(ty->name,
					ty->subtypes.map([&] (TyPtr t2) { return desugar(t2); }));
		else
			return ty;

	case tyPoly:
		if (!ty->name.empty())
		{
			auto it = polynames.find(ty->name);
			if (it == polynames.end())
			{
				auto t2 = Ty::makePoly();
				return (polynames[ty->name] = t2);
			}
			else
				return it->second;
		}
	default:
		return ty;
	}
}



SigPtr Desugar::desugar (SigPtr sig)
{
	Sig::ArgList args;
	args.reserve(sig->args.size());

	for (size_t i = 0, len = sig->args.size(); i < len; i++)
		args.push_back(Sig::Arg(sig->args[i].first, 
							desugar(sig->args[i].second)));

	return std::make_shared<Sig>(args, sig->span);
}
FuncDecl Desugar::desugar (const FuncDecl& func)
{
	auto env = LocEnv::make();
	for (auto& a : func.signature->args)
		env->newVar(a.first, a.second);

	return FuncDecl {
		.name = func.name,
		.signature = desugar(func.signature),
		.body = desugar(func.body, env),
		.span = func.span
	};
}