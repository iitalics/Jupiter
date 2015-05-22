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
SigPtr Subs::operator() (SigPtr sig) const
{
	auto sig2 = Sig::make({}, sig->span);
	sig2->args.reserve(sig->args.size());

	auto revRules = rules.reverse();

	for (auto& arg : sig->args)
		sig2->args.push_back({
			arg.first,
			apply(arg.second, revRules)
		});

	return sig2;
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
		std::vector<std::string> extra;

		ss << "ambiguous use of function '" << globfn->name << "'";

		extra.push_back("given: " + t2->string());
		for (auto& v : valid)
			extra.push_back("  candidate: " + std::get<1>(v)->string());

		throw t1->srcExp->span.die(ss.str(), extra);
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

	// push overload instance for the compiler to use
	fn.cunit->special[t1->srcExp] = inst.cunit;

	return true;
}
