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
	lenv->newVar(e->getString());
	return res;
}

struct SYard
{
	SYard (Desugar& _des)
		: des(_des) {}

	Desugar& des;
	std::vector<ExpPtr> ops;
	std::vector<ExpPtr> vals;

	void reduce ()
	{
		auto b = vals.back();
		vals.pop_back();

		auto op = ops.back();
		ops.pop_back();

		auto a = vals.back();
		vals.pop_back();

		vals.push_back(Exp::make(eCall, { op, a, b }, a->span + b->span));
	}

	int precOf (ExpPtr e) const
	{
		return std::get<1>(des.global->getPrecedence(e->getString()));
	}
	Assoc assocOf (ExpPtr e) const
	{
		return std::get<2>(des.global->getPrecedence(e->getString()));
	}

	void process (ExpPtr op, ExpPtr val)
	{
		auto prec = precOf(op);
		auto assoc = assocOf(op);

		while (ops.size() > 0 && (
				assoc == Assoc::Left ?
					precOf(ops.back()) <= prec :
					precOf(ops.back()) < prec))
			reduce();

		vals.push_back(val);
		ops.push_back(op);
	}

	ExpPtr result ()
	{
		while (ops.size() > 0)
			reduce();

		return vals[0];
	}
};

ExpPtr Desugar::desugarInfix (ExpPtr e, LocEnvPtr lenv)
{
	SYard syard(*this);
	syard.vals.push_back(desugar(e->subexps[0], lenv));

	for (size_t i = 1, len = e->subexps.size(); i < len; i += 2)
		syard.process(e->subexps[i], desugar(e->subexps[i + 1], lenv));

	return syard.result();
}







TyPtr Desugar::desugar (TyPtr ty)
{
	return ty;
}