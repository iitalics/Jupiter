#include "Infer.h"
#include "Compiler.h"
#include <stdexcept>







Infer::Infer (CompileUnit* _cunit, SigPtr sig)
	: env(_cunit->overload->env),
	  fn(_cunit->funcInst)
{
	auto overload = _cunit->overload;
	auto lenv = LocEnv::make();

	unify(mainSubs, overload->signature->tyList(), sig->tyList());

	// construct environment for arguments
	for (size_t len = sig->args.size(), i = 0; i < len; i++)
	{
		auto& arg = overload->signature->args[i];
		auto ty = sig->args[i].second;

		lenv->newVar(arg.first, ty);
	}

	// infer return type
	auto ret = infer(overload->body, lenv);
	
	// check with temporary return type
	unify(ret, fn.returnType, mainSubs, overload->signature->span);

	// update to new type
	fn.returnType = mainSubs(fn.returnType);
}



TyPtr Infer::infer (ExpPtr exp, LocEnvPtr lenv)
{
	static auto tyInt = Ty::makeConcrete("Int");
	static auto tyReal = Ty::makeConcrete("Real");
	static auto tyStr = Ty::makeConcrete("Str");
	static auto tyBool = Ty::makeConcrete("Bool");

	switch (exp->kind)
	{
	case eInt:
		return tyInt;
	case eReal:
		return tyReal;
	case eString:
		return tyStr;
	case eBool:
		return tyBool;

	case eVar:
		return inferVar(exp, lenv);
	case eTuple:
		return inferTuple(exp, lenv);
	case eCall:
		return inferCall(exp, lenv);
	case eCond:
		return inferCond(exp, lenv);
	case eLambda:
		return inferLambda(exp, lenv);
	case eBlock:
		return inferBlock(exp, lenv);
	case eLet:
		return inferLet(exp, lenv);
	case eAssign:
		return inferAssign(exp, lenv);

	case eiMake:
	case eiGet:
	case eiCall:
		return mainSubs(exp->getType());
	case eiPut:
		return Ty::makeUnit();
	case eiTag:
		return tyBool;

	default:
		throw exp->span.die("cannot infer this kind of expression");
	}
}

TyPtr Infer::inferVar (ExpPtr exp, LocEnvPtr lenv)
{
	if (exp->get<bool>()) // global
	{
		auto fn = env.getFunc(exp->getString());

		if (fn == nullptr || fn->overloads.empty())
			throw exp->span.die("invalid global");

		return Ty::makeOverloaded(exp, fn->name);
	}
	else
	{
		auto var = lenv->get(exp->getString());
		if (var == nullptr)
			throw exp->span.die("DESUGAR SHOULD MAKE THIS UNREACHABLE!!");

		return Ty::newPoly(var->ty);
	}
}
TyPtr Infer::inferBlock (ExpPtr exp, LocEnvPtr lenv)
{
	TyPtr res = nullptr;

	LocEnvPtr lenv2 = LocEnv::make(lenv);

	for (auto& e : exp->subexps)
	{
		auto dummy = Ty::makePoly();
		res = infer(e, lenv2);

		// instance overloaded types
		if (res != nullptr)
			unify(res, dummy, e->span);
	}

	// empty block returns unit "()"
	if (res == nullptr)
		res = Ty::makeUnit();

	return res;
}


TyPtr Infer::inferLet (ExpPtr exp, LocEnvPtr lenv)
{
	/*
		infer  let x1 : a = e1 
			= ()
			where
				t1 = infer e1
				S  = unify t1 a
				env += { x1 : S a }
	*/
	auto ty = mainSubs(exp->getType());      // written type
	auto tye = infer(exp->subexps[0], lenv); // inferred type

	unify(tye, ty, mainSubs, exp->subexps[0]->span);

	ty = mainSubs(ty);

	if (ty->kind == tyOverloaded)
		throw exp->span.die("invalid for variable to have overloaded type");

	lenv->newVar(exp->getString(), ty);

	return nullptr;
}

TyPtr Infer::inferCall (ExpPtr exp, LocEnvPtr lenv)
{
	/*
		infer  e1(e2,...,en)
			= S a
			where
				t1,...,tn = infer e1,...,infer en
				a : Ty    = newtype
				S : Subs  = unify t1 "Fn"(t2,...,tn,a)
	*/
	auto fnty = infer(exp->subexps[0], lenv);
	auto ret = Ty::makePoly();

	// add args backward o_o
	TyList args(ret);
	for (size_t len = exp->subexps.size(), i = len; i-- > 1; )
		args = TyList(infer(exp->subexps[i], lenv), args);

	auto model = Ty::makeFn(args);
	auto subs = unify(fnty, model, exp->span);

	return subs(ret);
}

TyPtr Infer::inferTuple (ExpPtr exp, LocEnvPtr lenv)
{
	/*
		infer  (e1,...,en)
			= "Tuple"(t1,...,tn)
			where
				t1,...,tn = infer e1,....,infer en
	*/
	TyList args;
	auto dummy = Ty::makePoly();

	for (auto it = exp->subexps.crbegin();
			it != exp->subexps.crend(); ++it)
	{
		auto ty = infer(*it, lenv);
		args = TyList(ty, args);
		
		// instance overloaded types
		unify(ty, dummy, (*it)->span);
	}

	return Ty::makeConcrete("Tuple", args);
}
TyPtr Infer::inferCond (ExpPtr exp, LocEnvPtr lenv)
{
	/*
		infer  if e1 then e2 else e3
			= S t2
			where
				t1,t2,t3 = infer t1,...,infer t3
				    unify t1 "Bool"
				S = unify t2 t3
	*/
	TyPtr tc, t1, t2;

	tc = infer(exp->subexps[0], lenv);
	t1 = infer(exp->subexps[1], lenv);
	t2 = infer(exp->subexps[2], lenv);

	unify(tc, Ty::makeConcrete("Bool"), exp->subexps[0]->span);
	auto subs = unify(t1, t2, exp->span);

	return subs(t1); // == subs(t2)
}
TyPtr Infer::inferLambda (ExpPtr exp, LocEnvPtr lenv)
{
	// this special expression retrieves the environment object
	//  of the current closure
	static auto envExp = Exp::make(eiEnv);

	auto sig = mainSubs(Sig::fromSigType(exp->getType(), exp->span));

	ExpPtr body;

	if (exp->subexps.size() > 1)
	{
		body = Exp::make(eBlock, {}, exp->span);
		body->subexps.reserve(exp->subexps.size());
		for (size_t i = 0, len = exp->subexps.size() - 1; i < len; i++)
		{
			auto var = exp->subexps[i + 1]->getString();
			auto get = Exp::make(eiGet, int_t(i), { envExp });
			get->setType(lenv->get(var)->ty);

			// retrieve variables from environment
			body->subexps.push_back(
				Exp::make(eLet, get->getType(), var, { get }));
		}
		body->subexps.push_back(exp->subexps[0]);
	}
	else
	{
		// no need
		body = exp->subexps[0];
	}

	// create seperate function containing lambda
	auto lamName = fn.cunit->compiler->genUniqueName("#lambda");
	auto overload = Overload::make(env, lamName, sig, body);
	overload->hasEnv = true;
	env.addFunc(lamName)->overloads.push_back(overload);

	return Ty::makeOverloaded(exp, lamName);
}
TyPtr Infer::inferAssign (ExpPtr exp, LocEnvPtr lenv)
{
	auto t1 = infer(exp->subexps[0], lenv);
	auto t2 = infer(exp->subexps[1], lenv);
	unify(t1, t2, exp->span);

	return Ty::makeUnit();
}
