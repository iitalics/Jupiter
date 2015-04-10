#include "Infer.h"


Subs::Subs (int emptyTypes)
{
	aliases.resize(emptyTypes);
}
Subs::Subs (const Subs& other)
	: aliases(other.aliases) {}

Subs::~Subs () {}

TyPtr Subs::newType ()
{
	auto ty = Ty::makePoly(aliases.size());
	aliases.push_back(nullptr);
	return ty;
}
void Subs::set (int poly, TyPtr alias)
{
	while (int(aliases.size()) <= poly)
		aliases.push_back(nullptr);

	for (int i = 0, len = aliases.size(); i < len; i++)
		if (i != poly)
			aliases[i] = applyRule(poly, alias, aliases[i]);

	aliases[poly] = alias;
}
TyPtr Subs::apply (TyPtr ty) const
{
	for (int i = 0, len = aliases.size(); i < len; i++)
		ty = applyRule(i, aliases[i], ty);
	return ty;
}

TyPtr Subs::applyRule (int poly, TyPtr alias, TyPtr ty) const
{
	if (ty == nullptr) return nullptr;

	switch (ty->kind)
	{
	case tyConcrete:
		{
			std::vector<TyPtr> buf;
			buf.reserve(ty->subtypes.size());

			auto differs = false;
			for (auto t : ty->subtypes)
			{
				auto t2 = applyRule(poly, alias, t);
				buf.push_back(t2);

				if (t2 != t)
					differs = true;
			}

			if (!differs)
				return ty;
			else
				return Ty::makeConcrete(ty->name, buf);
		}

	case tyPoly:
		if (ty->idx == poly)
			return alias;
		else
			return ty;

	default:
		return ty;
	}
}