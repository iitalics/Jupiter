#include "Infer.h"
#include "Compiler.h"
#include <stdexcept>


Subs::Subs (const RuleList& _rules)
	: rules(_rules) {}


Subs Subs::operator+ (const Rule& rule) const
{ return Subs(RuleList(rule, rules)); }

Subs& Subs::operator+= (const Rule& rule)
{
	rules = RuleList(rule, rules);
	return *this;
}

TyPtr Subs::operator() (TyPtr ty) const
{ return apply(ty, rules.reverse()); }

TyPtr Subs::apply (TyPtr ty, const RuleList& ru) const
{
	if (ru.nil())
		return ty;

	auto rule = ru.head();
	auto tail = ru.tail();

	switch (ty->kind)
	{
	case tyOverloaded:
	case tyPoly:
		if (rule.left == ty)
			ty = rule.right;

		return apply(ty, tail);

	case tyConcrete:
		if (ty->subtypes.size() == 0)
			return ty;
		else
		{
			auto diff = false;
			auto newTypes = ty->subtypes.map([&] (TyPtr t)
			{
				auto t2 = apply(t, ru);
				if (t != t2)
					diff = true;
				return t2;
			});
			
			if (diff)
				return Ty::makeConcrete(ty->name, newTypes);
		}
	default:
		return ty;
	}
}








Infer::Infer (CompileUnit* _cunit, SigPtr sig)
	: env(_cunit->overload->env),
	  fn(_cunit->funcInst)
{
	auto overload = _cunit->overload;
	auto lenv = LocEnv::make();

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



void Infer::unify (TyPtr t1, TyPtr t2, Subs& subs, const Span& span)
{
	if (!unify(subs, TyList(t1), TyList(t2)))
	{
		std::ostringstream ss;
		if (t1->kind == tyOverloaded)
		{
			ss << "function \"" << t1->name
			   << "\" incompatible with type " << t2->string();
		}
		else
		{
			auto strs = Ty::stringAll({ t1, t2 });

			ss << "incompatible types "
		       << strs[0] << " and " << strs[1];
		}
		throw span.die(ss.str());
	}
	else
	{
		// update each circular call
		for (auto& cu : circular)
		{
			auto& inst = cu->funcInst;
			inst.returnType = subs(inst.returnType);
		}
	}
}

bool Infer::unify (Subs& out, TyList l1, TyList l2)
{
	while (!(l1.nil() || l2.nil()))
	{
		// apply older substitution first!
		auto t1 = out(l1.head()); // variable
		auto t2 = out(l2.head()); // model
		++l1, ++l2;

		if (t1->kind == tyOverloaded)
			return unifyOverload(out, t1, t2, l1, l2);

	//	if (t1->kind == tyWildcard || t2->kind == tyWildcard)
	//		continue;

		if (t1->kind != tyPoly && t2->kind == tyPoly)
		{
			// swap arguments
			auto tmp = t1;
			t1 = t2;
			t2 = tmp;
		}

		switch (t1->kind)
		{
		case tyPoly:
			if (t2 == t1)
				break;
			//if (Ty::contains(t2, t1))
			//	goto fail;
			out += Subs::Rule { t1, t2 }; /* t1 := t2 */
			break;

		case tyConcrete:
			if (t2->kind != tyConcrete)
				return false;

			if (t1->name != t2->name ||
					t1->subtypes.size() != t2->subtypes.size())
				return false;

			if (!unify(out, t1->subtypes, t2->subtypes))
				return false;
			else
				break;

		default:
			return false;
		}
	}

	return true;
}


bool Infer::unifyOverload (Subs& out,
                             TyPtr t1, TyPtr t2,
                             TyList l1, TyList l2)
{
	auto& name = t1->name;
	auto globfn = env.getFunc(name);

	using Valid = std::tuple<OverloadPtr, TyPtr, Subs>;
	std::vector<Valid> valid;

	auto ret = Ty::makePoly();

	for (auto& overload : globfn->overloads)
	{
		// create type for overload's signature
		auto args = overload->signature->tyList(ret);
		auto fnty = Ty::newPoly(Ty::makeFn(args));

		// try to unify, if it fails then try next overload
		Subs subs = out;
		if (!unify(subs, TyList(t2, l1), TyList(fnty, l2)))
			goto cont;

		fnty = subs(fnty);

		// if any of the arguments are complete polytypes,
		//  it is considered an invalid overload
		for (auto s = fnty->subtypes; !s.tail().nil(); ++s)
			if (s.head()->kind == tyPoly)
				goto cont;

		// add to list of potential overloads
		valid.push_back(Valid { overload, fnty, subs });
	cont:;
	}

	// no overloads :(
	if (valid.empty())
		return false;

	// TODO: decide best overload here
	//  right now it assumes that all
	//  overloads are equally valid
	auto& best = valid.front();
	if (valid.size() > 1)
	{
		std::ostringstream ss;
		ss << "ambiguous arguments to function '" << globfn->name << "'";
		throw t1->srcExp->span.die(ss.str());
	}

	auto overload = std::get<0>(best);
	auto fnty = std::get<1>(best);
	out = std::get<2>(best);

	// construct signature from overloaded function
	auto sig = Sig::make();
	size_t i = 0, len = overload->signature->args.size();
	for (auto ty : fnty->subtypes)
		if (i >= len)
			break;
		else
		{
			sig->args.push_back(Sig::Arg {
				overload->signature->args[i].first, ty
			});
			i++;
		}

	// instanciate overloaded function with
	//  this signature
	auto inst = Overload::inst(overload, sig, fn.cunit->compiler);
	TyPtr resty;

	if (inst.cunit->finishedInfer)
		resty = Ty::newPoly(inst.type());
	else
	{
		// circular call, aka (in)direct recursion
		resty = inst.type();
		circular.insert(inst.cunit);
	}

	// push final substitutions
	if (!unify(out, TyList(resty), TyList(t2)))
		return false;
	out += { t1, resty }; /* t1 := rety */

	// push overload instance name for the compiler to use
	fn.cunit->special[t1->srcExp] = inst.cunit->internalName;

	return true;
}

TyPtr Infer::infer (ExpPtr exp, LocEnvPtr lenv)
{
	static auto tyInt = Ty::makeConcrete("Int");
	static auto tyReal = Ty::makeConcrete("Real");
	static auto tyString = Ty::makeConcrete("String");
	static auto tyBool = Ty::makeConcrete("Bool");

	switch (exp->kind)
	{
	case eInt:
		return tyInt;
	case eReal:
		return tyReal;
	case eString:
		return tyString;
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
//	case eLambda:
//		return inferLambda(exp, lenv);
	case eBlock:
		return inferBlock(exp, lenv);
	case eLet:
		return inferLet(exp, lenv);

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
