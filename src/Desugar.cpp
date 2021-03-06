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
	case eMem:
		return desugarMem(exp, lenv);
	case eInfix:
		return desugarInfix(exp, lenv);
	case eCond:
		return desugarCond(exp, lenv);
	case eLambda:
		return desugarLambda(exp, lenv);
	case eAssign:
		return desugarAssign(exp, lenv);
	case eBlock:
		return desugarBlock(exp, lenv);

	case eForEach:
		return desugarForEach(exp, lenv);
	case eForRange:
		return desugarForRange(exp, lenv);

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
	if (e->subexps.size() == 0 && e->getType() == nullptr)
		return e;
	else
	{
		auto e2 = e->mapSubexps([=] (ExpPtr e2) 
		{
			return desugar(e2, lenv);
		});

		if (e2->getType() != nullptr)
			e2->setType(desugar(e2->getType(), e2->span));

		return e2;
	}
}

ExpPtr Desugar::desugarVar (ExpPtr e, LocEnvPtr lenv)
{
	auto name = e->getString();
	auto var = lenv->get(name);

	if (var == nullptr)
		return desugarGlobal(e);
	else
	{
		e->set<bool>(false);

		for (auto le = lenv; ; le = le->parent)
		{
			if (le->has(name))
				break;
			else if (le->uses != nullptr)
				le->uses->insert(name);
		}

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

	e->set<bool>(true);
	return e;
}

ExpPtr Desugar::desugarTuple (ExpPtr e, LocEnvPtr lenv)
{
	// if (e->subexps.size() == 0) // TODO: unit?

	// break out single-tuple
	if (e->subexps.size() == 1)
		return desugar(e->subexps[0], lenv);
	else
		return desugarSubexps(e, lenv);
}
ExpPtr Desugar::desugarCall (ExpPtr e, LocEnvPtr lenv)
{
	if (e->subexps[0]->kind == eMem)
	{
		// e1.f(e2, e3, ...)  =>  f(e1, e2, e3, ...)

		auto mem = e->subexps[0];
		auto memVar = Exp::make(eVar, mem->getString(), mem->span);
		memVar = desugarGlobal(memVar);

		ExpList args;
		args.push_back(memVar);
		for (size_t i = 0, len = e->subexps.size(); i < len; i++)
		{
			auto a = (i == 0) ? mem->subexps[0] : e->subexps[i];

			args.push_back(desugar(a, lenv));
		}

		return Exp::make(eCall, args, e->span);
	}
	else
		return desugarSubexps(e, lenv);
}
ExpPtr Desugar::desugarCond (ExpPtr e, LocEnvPtr lenv)
{
	return desugarSubexps(e, lenv);
}
ExpPtr Desugar::desugarLambda (ExpPtr e, LocEnvPtr lenvp)
{
	auto sig = desugar(Sig::fromSigType(e->getType()));
	auto lenv = LocEnv::make(lenvp);
	// keep track of which local variables this lambda uses
	LocEnv::UseSet uses;

	for (auto& arg : sig->args)
		lenv->newVar(arg.first, arg.second);

	lenv->uses = &uses;
	auto body = desugar(e->subexps[0], lenv);

	// list used variables in resulting lambda expression
	ExpList exps;
	exps.reserve(1 + uses.size());

	exps.push_back(body);
	for (auto& var : uses)
		exps.push_back(Exp::make(eVar, var));

	return Exp::make(eLambda, sig->toSigType(), "", std::move(exps), e->span);
}
ExpPtr Desugar::desugarBlock (ExpPtr e, LocEnvPtr lenvp)
{
	auto lenv = LocEnv::make(lenvp);

	auto res = e->mapSubexps([=] (ExpPtr e2)
	{
		ExpPtr res;

		if (e2->kind == eLet)
			res = desugarLet(e2, lenv);
		else
			res = desugar(e2, lenv);

		return res;
	});

	// back-track to confirm each var is mutable
	for (auto& var : lenv->vars)
		if (var->mut)
		{
			for (auto& e : res->subexps)
				if (e->kind == eLet && e->getString() == var->name)
				{
					e->set<bool>(true);
					break;
				}
		}

	return res;
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









// code generation
ExpPtr Desugar::genVar (ExpPtr block, ExpPtr initial, TyPtr ty)
{
	std::stringstream name;
	name << "#s" << (vars++);

	auto let = Exp::make(eLet, ty, name.str(), { initial }, block->span);
	auto var = Exp::make(eVar, name.str(), bool(false), {}, block->span);

	block->subexps.push_back(let);
	return var;
}



ExpPtr Desugar::desugarAssign (ExpPtr e, LocEnvPtr lenv)
{
	auto left = e->subexps[0];
	auto right = desugar(e->subexps[1], lenv);

	if (left->kind == eVar)
	{
		left = desugarVar(left, lenv);
		if (left->get<bool>())
			throw e->span.die("cannot assign to global");

		lenv->get(left->getString())->mut = true;

		return Exp::make(eAssign, { left, right }, e->span);
	}
	else if (left->kind == eMem)
	{
		//   x.t = y   ->   t=(x, y)

		auto fnName = left->getString() + "=";
		auto fn = Exp::make(eVar, fnName, true, {}, e->span);

		fn = desugarGlobal(fn);
		left = desugar(left->subexps[0], lenv);

		return Exp::make(eCall, { fn, left, right }, e->span);
	}
	else
		throw left->span.die("cannot assign to this expression");
}

ExpPtr Desugar::desugarMem (ExpPtr e, LocEnvPtr lenv)
{
	// (e1.f)  =>  f(e1)
	
	auto memVar = Exp::make(eVar, e->getString(), e->span);
	memVar = desugarGlobal(memVar);

	return Exp::make(eCall,
		{
			memVar,
			desugar(e->subexps[0], lenv),
		}, e->span);
}

ExpPtr Desugar::desugarForRange (ExpPtr e, LocEnvPtr lenv)
{
	static auto wc = Ty::makeWildcard();
	/*	for x1 : (e1) -> (e2) { e3 }
	{
		let t1 = (e1);
		let t2 = (e2);
		loop <(t1, t2) {
			let x1 = t1;
			{ e3 };
			t1 = succ(t1);
		}
	} */
	auto span = e->span;
	auto v_less = Exp::make(eVar, std::string("<"), {}, span);
	auto v_succ = Exp::make(eVar, std::string("succ"), {}, span);
	auto block1 = Exp::make(eBlock, {}, span);
	auto block2 = Exp::make(eBlock, {}, span);
	auto t1 = genVar(block1, e->subexps[0]);
	auto t2 = genVar(block1, e->subexps[1]);

	auto cond = Exp::make(eCall, { v_less, t1, t2 }, span);
	auto loop = Exp::make(eLoop, { cond, block2 }, span);
	auto let = Exp::make(eLet, wc, e->getString(), { t1 }, span);
	auto succ = Exp::make(eCall, { v_succ, t1 }, span);
	auto assign = Exp::make(eAssign, { t1, succ }, span);

	block2->subexps.reserve(3);
	block2->subexps.push_back(let);
	block2->subexps.push_back(e->subexps[2]);
	block2->subexps.push_back(assign);
	block1->subexps.push_back(loop);

	return desugar(block1, lenv);
}

ExpPtr Desugar::desugarForEach (ExpPtr e, LocEnvPtr lenv)
{
	static auto wc = Ty::makeWildcard();
	/*	for x1 : (e1) { e2 }
	{
		let t1 = (e1);
		loop cons?(t1) {
			let x1 = hd(t1);
			{ e2 };
			t1 = tl(t1);
		}
	} */
	auto span = e->span;
	auto v_cons = Exp::make(eVar, std::string("cons?"), {}, span);
	auto v_hd = Exp::make(eVar, std::string("hd"), {}, span);
	auto v_tl = Exp::make(eVar, std::string("tl"), {}, span);
	auto block1 = Exp::make(eBlock, {}, span);
	auto block2 = Exp::make(eBlock, {}, span);
	auto t1 = genVar(block1, e->subexps[0]);

	auto cons = Exp::make(eCall, { v_cons, t1 }, span);
	auto loop = Exp::make(eLoop, { cons, block2 }, span);
	auto hd = Exp::make(eCall, { v_hd, t1 }, span);
	auto let = Exp::make(eLet, wc, e->getString(), { hd }, span);
	auto tl = Exp::make(eCall, { v_tl, t1 }, span);
	auto assign = Exp::make(eAssign, { t1, tl }, span);

	block2->subexps.reserve(3);
	block2->subexps.push_back(let);
	block2->subexps.push_back(e->subexps[1]);
	block2->subexps.push_back(assign);
	block1->subexps.push_back(loop);

	return desugar(block1, lenv);
}









TyPtr Desugar::desugar (TyPtr ty, const Span& span)
{
	switch (ty->kind)
	{
	case tyWildcard:
		return Ty::makePoly();

	case tyConcrete:
		{
			auto tyi = global.getType(ty->name);
			if (tyi == nullptr)
			{
				std::ostringstream ss;
				ss << "undefined type '" << ty->name << "'";
				throw span.die(ss.str());
			}
			else if (!tyi->isType(ty))
			{
				std::ostringstream ss;
				ss << "malformed number of arguments for '" << ty->name << "'";
				throw span.die(ss.str());
			}

			if (!ty->subtypes.nil())
				return Ty::makeConcrete(ty->name,
						ty->subtypes.map([&] (TyPtr t2) { return desugar(t2, span); }));
			else
				return ty;
		}

	case tyPoly:
		if (!ty->name.empty())
		{
			auto it = polynames.find(ty->name);
			if (it == polynames.end())
			{
				auto t2 = Ty::makePoly(ty->name);
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
							desugar(sig->args[i].second, sig->span)));

	return Sig::make(args, sig->span);
}
void Desugar::desugar (OverloadPtr overload)
{
	if (overload->isDesugared) return;

	overload->isDesugared = true;
	overload->signature = desugar(overload->signature);

	auto env = LocEnv::make();
	for (auto& a : overload->signature->args)
		env->newVar(a.first, a.second);
	overload->body = desugar(overload->body, env);
}
