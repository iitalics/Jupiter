#include "Env.h"
#include "Compiler.h"
#include <sstream>
#include <iostream>
#include <functional>
#include <set>

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

static void generateCtor (GlobProto& proto,
	                        std::set<std::string>& fields,
                            TyPtr mainty, const std::string& ctorname,
                            const SigPtr& sig, const Span& span)
{
	ExpList callArgs;
	auto exp_make = Exp::make(eiMake, ctorname, {}, span);
	callArgs.push_back(exp_make);

	for (auto& arg : sig->args)
		callArgs.push_back(Exp::make(eVar, arg.first, {}, span));

	auto fnty = Ty::makeFn(sig->tyList(mainty));
	exp_make->setType(fnty);

	proto.funcs.push_back({
		true,
		ctorname,
		sig,
		Exp::make(eCall, callArgs, span),
		span
	});


	auto util_sig = Sig::make({ { "a", mainty } }, span);
	auto util_var = Exp::make(eVar, std::string("a"), {}, span);

	auto exp_cmp = Exp::make(eiTag, ctorname, { util_var }, span);

	proto.funcs.push_back({
		true,
		ctorname + "?",
		util_sig,
		exp_cmp,
		span
	});

	for (size_t i = 0, len = sig->args.size(); i < len; i++)
	{
		auto field = sig->args[i].first;

		if (!fields.insert(field).second)
		{
			std::ostringstream ss;
			ss << "duplicate field '" << field << "'";
			throw sig->span.die(ss.str());
		}

		auto exp_get = Exp::make(eiGet, int_t(i), { util_var }, span);
		exp_get->setType(sig->args[i].second);
		exp_get->setString(ctorname); // ju_safe_get  instead of  ju_get

		proto.funcs.push_back({
			true,
			field,
			util_sig,
			exp_get,
			span
		});
	}
}

void GlobEnv::generateType (TypeDecl& tydecl)
{
//	auto tyi = getType(tydecl.name);
	auto mainty = Ty::makeConcrete(tydecl.name, tydecl.polytypes);

	std::set<std::string> fields;

	size_t i = 0;
	for (auto& ctor : tydecl.ctors)
	{
		auto span = ctor.signature->span;
		auto sig = Sig::make({}, span);

		for (size_t j = 0; j < i; j++)
			if (tydecl.ctors[j].name == ctor.name)
				throw span.die("constructors must have distinct names");
		i++;

		for (auto& arg : ctor.signature->args)
			sig->args.push_back({
				arg.first,
				fillTypes(arg.second, tydecl.polytypes, span)
			});

		generateCtor(proto, fields, mainty, ctor.name, sig, span);
	}
}