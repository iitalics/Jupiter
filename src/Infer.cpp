#include "Infer.h"
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
			return
				Ty::makeConcrete(ty->name,
					ty->subtypes.map([=] (TyPtr t)
					{
						return apply(t, ru);
					}));
	default:
		return ty;
	}
}








Infer::Infer (const FuncOverload& overload, SigPtr sig)
	: parent(overload.parent),
	  fn { overload.name(), sig, Ty::makePoly() }
{
	auto lenv = LocEnv::make();

	for (size_t len = sig->args.size(), i = 0; i < len; i++)
	{
		auto& arg = overload.signature->args[0];
		auto ty = sig->args[i].second;

		lenv->newVar(arg.first, ty);
	}

	if (!unify(mainSubs, overload.signature->tyList(), sig->tyList()))
		throw overload.signature->span.
				die("invalid types when instancing overload!");

	auto ret = infer(overload.body, lenv);
	
	unify(ret, fn.returnType, mainSubs, overload.signature->span);

	ret = fn.returnType = mainSubs(fn.returnType);

	if (ret->kind == tyOverloaded)
		throw sig->span.die("invalid for function to return overloaded type");
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
			ss << "incompatible types "
		       << t1->string() << " and " << t2->string();
		throw span.die(ss.str());
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

		if (t2->kind == tyPoly)
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
			out += Subs::Rule { t1, t2 };
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
	auto fn = parent->env.getFunc(name);

	using Valid = std::tuple<FuncOverload&, Subs, TyPtr>;
	std::vector<Valid> valid;

	for (auto& over : fn->overloads)
	{
		// create type for overload's signature
		// edit: "wildcard types" are a bad idea
		auto args = over.signature->tyList(Ty::makePoly());
		auto fnty = Ty::newPoly(Ty::makeFn(args));

		// try to unify, if it fails then try next overload
		Subs subs = out;
		if (!unify(subs, TyList(t2, l1), TyList(fnty, l2)))
			continue;

		// add to list of potential overloads
		valid.push_back(Valid { over, subs, fnty });
	}

	// no overloads :(
	if (valid.empty())
		return false;

	// decide best overload here
	// TODO: ambiguous overloads = subs failure or compile error?
	auto& best = valid.front();
	if (valid.size() > 1)
		return false;

	auto& over = std::get<0>(best);
	auto fnty  = std::get<2>(best);
	out = std::get<1>(best);


	// construct signature from overloaded function
	auto sig = Sig::make();
	size_t i = 0, len = over.signature->args.size();
	for (auto ty : fnty->subtypes)
		if (i >= len)
			break;
		else
		{
			sig->args.push_back(Sig::Arg {
				over.signature->args[i].first,
				out(ty)
			});
			i++;
		}

	// instanciate overloaded function
	// TODO: cache?
	auto resty = over.inst(sig).type();

	// push final substitutions
	if (!unify(out, TyList(resty), TyList(t2)))
		return false;
	out += { t1, resty };

	return true;
}

TyPtr Infer::infer (ExpPtr exp, LocEnvPtr lenv)
{
	switch (exp->kind)
	{
	case eInt:
		return Ty::makeConcrete("Int");
	case eReal:
		return Ty::makeConcrete("Real");
	case eString:
		return Ty::makeConcrete("String");
	case eBool:
		return Ty::makeConcrete("Bool");

	case eVar:
		return inferVar(exp, lenv);
	case eTuple:
		return inferTuple(exp, lenv);
	case eCall:
		return inferCall(exp, lenv);
	case eInfix:
		return inferInfix(exp, lenv);
	case eCond:
		return inferCond(exp, lenv);
//	case eLambda:
//		return inferLambda(exp, lenv);
	case eBlock:
		return inferBlock(exp, lenv);
	case eLet:
		return inferLet(exp, lenv);

	default:
		throw exp->span.die("unimplemented kind of expression");
	}
}

TyPtr Infer::inferVar (ExpPtr exp, LocEnvPtr lenv)
{
	auto idx = exp->get<int>();

	if (idx == -1)
	{
		auto fn = parent->env.getFunc(exp->getString());

		if (fn == nullptr || fn->overloads.empty())
			throw exp->span.die("invalid global");

		return Ty::makeOverloaded(fn->name);
	}
	else
	{
		auto var = lenv->get(idx);
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
		res = infer(e, lenv2);

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

	auto subs = unify(tye, ty, exp->subexps[0]->span);

	ty = subs(ty);

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
	Subs subs = unify(fnty, model, exp->span);

	return subs(ret);
}

TyPtr Infer::inferTuple (ExpPtr exp, LocEnvPtr lenv)
{
	TyList args;

	for (auto it = exp->subexps.crbegin();
			it != exp->subexps.crend(); ++it)
		args = TyList(infer(*it, lenv), args);

	return Ty::makeConcrete("Tuple", args);
}
TyPtr Infer::inferInfix (ExpPtr exp, LocEnvPtr lenv) { return Ty::makeInvalid(); }
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
