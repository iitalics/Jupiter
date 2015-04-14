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
	Subs mainSubs;
	GlobFunc* parent;
	FuncInstance fn;

	Infer (const FuncOverload& overload, SigPtr sig);

	void unify (Subs& subs, TyPtr a, TyPtr b, Span span);
	bool unifyList (Subs& subs, TyList la, TyList lb);

	TyPtr infer (ExpPtr exp, LocEnvPtr lenv, Subs& subs);
};
