#pragma once
#include "Env.h"

struct Infer;
using InferPtr = std::shared_ptr<Infer>;
using InferList = list<InferPtr>;



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

	Subs operator+ (const Rule& r) const;
	Subs& operator+= (const Rule& r);
	TyPtr operator() (TyPtr ty) const;

private:
	TyPtr apply (TyPtr ty, const RuleList& r) const;
};