#pragma once
#include "Env.h"



struct Subs
{
	Subs (int emptyTypes = 0);
	Subs (const Subs& other);
	~Subs ();

	std::vector<TyPtr> aliases;

	TyPtr newType ();
	void set (int poly, TyPtr alias);
	TyPtr apply (TyPtr ty) const;

private:
	TyPtr applyRule (int poly, TyPtr alias, TyPtr ty) const;
};