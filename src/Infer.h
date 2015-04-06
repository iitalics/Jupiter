#pragma once
#include "Env.h"



struct Subs
{
	Subs (int emptyTypes = 0);
	Subs (const Subs& other);
	~Subs ();

	std::vector<TyPtr> aliases;

	TyPtr newType ();
	TyPtr add (int poly, TyPtr alias);
	TyPtr apply (TyPtr ty) const;
};