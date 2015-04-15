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

	auto ret = infer(overload.body, lenv);
	fn.returnType = ret;

	if (ret->kind == tyOverloaded)
		throw sig->span.die("invalid for function to return overloaded type");
}



void Infer::unify (Subs& out, TyList l1, TyList l2,
					const Span& span)
{
	TyPtr t1, t2;

	while (!l1.nil() && !l2.nil())
	{
		t1 = l1.head();
		t2 = l2.head();
		++l1, ++l2;

		if (t2->kind == tyPoly ||
				(t2->kind == tyOverloaded &&
				 t1->kind != tyPoly))
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
				break;;
			//if (Ty::contains(t2, t1))
			//	goto fail;
			out += Subs::Rule { t1, t2 };
			break;

		case tyConcrete:
			if (t2->kind != tyConcrete)
				goto fail;

			if (t1->name != t2->name ||
					t1->subtypes.size() != t2->subtypes.size())
				goto fail;

			unify(out, t1->subtypes, t2->subtypes, span);
			break;

		case tyOverloaded:
			unifyOverload(out, t1->name, t2, l1, l2, span);
			break;

		default:
			goto fail;
		}
	}

fail:
	std::ostringstream ss;
	ss << "cannot unify types " << t1->string() << " and " << t2->string();
	throw span.die(ss.str());
}


void Infer::unifyOverload (Subs& out,
						const std::string& name,
						TyPtr t2,
						TyList l1, TyList l2,
						Span span)
{
	// TODO: unify overloaded type
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

		if (fn->overloads.size() > 1)
			throw exp->span.die("unimplemented: overloaded type");

		// instance the only available overload
		//  this is only a hacky temporary replacement
		//  for overloaded types
		auto& overload = fn->overloads.front();
		auto inst = overload.inst(overload.signature);

		return Ty::newPoly(inst.type());	
	}
	else
	{
		auto var = lenv->get(idx);
		if (var == nullptr)
			throw exp->span.die("undeclared variable "
				                "(DESUGAR SHOULD MAKE THIS UNREACHABLE!!)");

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

	auto ty = exp->getType();                // given type
	auto tye = infer(exp->subexps[0], lenv); // init type

	Subs subs;
	unify(subs, ty, tye, exp->span);

	lenv->newVar(exp->getString(), subs(ty));

	return nullptr;
}

TyPtr Infer::inferCall (ExpPtr exp, LocEnvPtr lenv)
{
	/*
		infer  e1(e2,...,en)
			= S a
			where
				t1,...,tn = infer(e1,...,en)
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
	Subs subs;
	unify(subs, model, fnty, exp->span);

	return subs(ret);
}

TyPtr Infer::inferTuple (ExpPtr exp, LocEnvPtr lenv) { return Ty::makeInvalid(); }
TyPtr Infer::inferInfix (ExpPtr exp, LocEnvPtr lenv) { return Ty::makeInvalid(); }
TyPtr Infer::inferCond (ExpPtr exp, LocEnvPtr lenv) { return Ty::makeInvalid(); }

