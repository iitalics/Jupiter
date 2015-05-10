#include "Env.h"
#include <sstream>
#include <iostream>
#include <functional>

int_t GlobEnv::getTag (const std::string& ident)
{
	// PLEASE NO HASH COLLISIONS THANK YOU
	std::hash<std::string> hash;
	return int_t(hash(ident) & 0x7fffffff);
}


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

static void generateCtor (GlobProto& proto, TyPtr conty, const std::string& ctorname,
                            const SigPtr& sig, const Span& span)
{
	/*
		type Ty = ctor1(...)

		func ctor1 (a : T1, b : T2, ...) {
			(^make (T1, T2, ...) -> Ty  "ctor1")
				(a, b, ...)
		}
		func ctor1? (a : Ty) {
			^tag? "ctor1" a
		}
	*/
	ExpList callArgs;
	auto exp_make = Exp::make(eiMake, ctorname, {}, span);
	callArgs.push_back(exp_make);

	for (auto& arg : sig->args)
		callArgs.push_back(Exp::make(eVar, arg.first, {}, span));

	auto fnty = Ty::makeFn(sig->tyList(conty));
	exp_make->setType(fnty);

	auto exp_body = Exp::make(eCall, callArgs, span);

	proto.funcs.push_back({
		ctorname,
		sig,
		exp_body,
		span
	});


	auto cmp_exp = Exp::make(eiTag, ctorname,
		{ Exp::make(eVar, std::string("a"), {}, span) }, span);
	auto cmp_sig = Sig::make({ { "a", conty } }, span);

	proto.funcs.push_back({
		ctorname + "?",
		cmp_sig,
		cmp_exp,
		span
	});
}

void GlobEnv::generateType (TypeDecl& tydecl, GlobProto& proto)
{
//	auto tyi = getType(tydecl.name);
	auto conty = Ty::makeConcrete(tydecl.name, tydecl.polytypes);

	for (auto& ctor : tydecl.ctors)
	{
		auto span = ctor.signature->span;
		auto sig = Sig::make({}, span);

		for (auto& arg : ctor.signature->args)
			sig->args.push_back({
				arg.first,
				fillTypes(arg.second, tydecl.polytypes, span)
			});

		generateCtor(proto, conty, ctor.name, sig, span);
	}
}