#pragma once
#include "Env.h"


struct Subs
{
	struct Rule
	{
		TyPtr left;
		TyPtr right;
	};
	using RuleList = list<Rule>;

	RuleList rules;

	Subs (const RuleList& rules = RuleList());

	TyPtr operator() (TyPtr ty) const;
	Subs operator+ (const Rule& r) const;
	Subs& operator+= (const Rule& r);

private:
	TyPtr apply (TyPtr ty, const RuleList& r) const;
};

struct Infer;
using InferPtr = Infer*;
using InferList = list<InferPtr>;

struct Infer
{
	GlobFunc* parent;
	FuncInstance fn;

	Infer (const FuncOverload& overload, SigPtr sig);

	void unify (Subs& out, TyList l1, TyList l2,
				const Span& span = Span());

	inline void unify (Subs& out, TyPtr t1, TyPtr t2,
						const Span& span = Span())
	{
		unify(out, TyList(t1), TyList(t2), span);
	}

	void unifyOverload (Subs& out,
	                       const std::string& name,
	                       TyPtr t2,
	                       TyList l1, TyList l2,
	                       Span span);

	TyPtr infer (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferVar (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferTuple (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferCall (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferInfix (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferCond (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferBlock (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferLet (ExpPtr exp, LocEnvPtr lenv);

};
