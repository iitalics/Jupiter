#include "Infer.h"



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

	case tyOverloaded:
	default:
		return ty;
	}
}








Infer::Infer (const FuncOverload& overload, SigPtr sig)
	: parent(overload.parent),
	  fn { overload.name(), sig, Ty::makePoly() }
{
	auto lenv = LocEnv::make();
	for (auto arg : sig->args)
		lenv->newVar(arg.first, arg.second);

	auto res = infer(overload.body, lenv, mainSubs);
	unify(mainSubs, res, fn.returnType, overload.body->span);
	fn.returnType = res;
}



TyPtr Infer::infer (ExpPtr exp, LocEnvPtr lenv, Subs& subs)
{
	return Ty::makePoly();
}




void Infer::unify (Subs& subs, TyPtr a, TyPtr b, Span span)
{
	if (!unifyList(subs, TyList(a), TyList(b)))
	{
		std::ostringstream ss;
		ss << "incompatible types " << a->string() << " and " << b->string();
		throw span.die(ss.str());
	}
}

bool Infer::unifyList (Subs& subs, TyList la, TyList lb)
{
	if (la.nil() || lb.nil())
		return true;

	auto a = la.head();
	auto b = lb.head();
	++la;
	++lb;

	return unifyList(subs, la, lb);
}


