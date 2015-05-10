#include "Env.h"
#include <sstream>
#include <iostream>

static TyPtr fillTypes (TyPtr ty, const TyList& pool, const Span& span)
{
	switch (ty->kind)
	{
	case tyPoly:
		for (auto p : pool)
			if (p->name == ty->name)
				return p;

		{
			std::ostringstream ss;
			ss << "unbound type '" << ty->string() << "'";
			throw span.die(ss.str());
		}

	case tyConcrete:
		if (ty->subtypes.size() == 0)
			return ty;
		else
			return Ty::makeConcrete(ty->name,
				ty->subtypes.map([=] (TyPtr ty2)
				{
					return fillTypes(ty2, pool, span);
				}));

	default:
		throw span.die("types in constructor must be defined");
	}
}

void GlobEnv::generateType (TypeDecl& tydecl, GlobProto& proto)
{
//	auto tyi = getType(tydecl.name);
	auto conty = Ty::makeConcrete(tydecl.name, tydecl.polytypes);

	int_t tag = 0;
	for (auto& ctor : tydecl.ctors)
	{
		/*
			type Ty = ctor1(...)

			func ctor1 (a : T1, b : T2, ...) {
				(^make (T1, T2, ...) -> Ty  "ctor1")
					(a, b, ...)
			}
		*/
		auto span = ctor.signature->span;

		Sig::ArgList sigArgs;
		ExpList callArgs;

		auto exp_make = Exp::make(eiMake, tag, {}, span);
		callArgs.push_back(exp_make);

		for (auto& arg : ctor.signature->args)
		{
			sigArgs.push_back({
				arg.first,
				fillTypes(arg.second, tydecl.polytypes, span)
			});

			callArgs.push_back(Exp::make(eVar, arg.first, {}, span));
		}

		auto sig = Sig::make(sigArgs, span);
		auto fnty = Ty::makeFn(sig->tyList(conty));
		exp_make->setType(fnty);

		auto exp_body = Exp::make(eCall, callArgs, span);

		proto.funcs.push_back({
			ctor.name,
			sig,
			exp_body,
			span
		});

		tag++;
	}
}