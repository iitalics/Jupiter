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
	Subs mainSubs;

	Infer (const FuncOverload& overload, SigPtr sig);

	Subs unify (TyPtr t1, TyPtr t2, const Span& span = Span());
	
	bool unify (Subs& out, TyList l1, TyList l2);
	bool unifyOverload (Subs& out,
	                       TyPtr t1, TyPtr t2,
	                       TyList l1, TyList l2);

	TyPtr infer (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferVar (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferTuple (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferCall (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferInfix (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferCond (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferBlock (ExpPtr exp, LocEnvPtr lenv);
	TyPtr inferLet (ExpPtr exp, LocEnvPtr lenv);

};
