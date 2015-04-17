#include "Infer.h"
#include <sstream>
#include <iostream>



Ty::Ty (TyKind k)
	: kind(k), subtypes(), name("") {}

Ty::~Ty () {}

TyPtr Ty::makeConcrete (const std::string& t, const TyList& sub)
{
	auto ty = std::make_shared<Ty>(tyConcrete);
	ty->subtypes = sub;
	ty->name = t;
	return ty;
}
TyPtr Ty::makePoly (const std::string& name)
{
	auto ty = std::make_shared<Ty>(tyPoly);

	ty->name = name;

	return ty;
}
TyPtr Ty::makeOverloaded (const std::string& name)
{
	auto ty = std::make_shared<Ty>(tyOverloaded);
	ty->name = name;
	return ty;
}
TyPtr Ty::makeWildcard ()
{
	static auto ty = std::make_shared<Ty>(tyWildcard);
	return ty;
}
TyPtr Ty::makeInvalid ()
{
	static auto ty = std::make_shared<Ty>(tyInvalid);
	return ty;
}
TyPtr Ty::makeUnit ()
{
	static auto ty = makeConcrete("Tuple");
	return ty;
}


bool Ty::aEquiv (TyPtr other) const
{
	if (kind != other->kind)
		return false;

	switch (kind)
	{
	case tyPoly:
		return true;

	case tyConcrete:
		if (name != other->name)
			return false;

		for (auto s1 = subtypes, s2 = other->subtypes; ; ++s1, ++s2)
			if (s1.nil() && s2.nil())
				return true;
			else if (s1.nil() || s2.nil())
				return false;
			else
				if (!s1.head()->aEquiv(s2.head()))
					return false;

	default:
		return false;
	}
}

TyPtr Ty::newPoly (TyPtr ty)
{
	Subs subs;
	return newPoly(ty, subs);
}

TyPtr Ty::newPoly (TyPtr ty, Subs& subs)
{
	switch (ty->kind)
	{
	case tyPoly:
		{
			for (const auto& r : subs.rules)
				if (r.left == ty)
					return r.right;
			
			auto newtype = Ty::makePoly();		
			subs += Subs::Rule { ty, newtype };
			return newtype;
		}

	case tyConcrete:
		if (ty->subtypes.nil())
			return ty;
		return
			Ty::makeConcrete(ty->name,
				ty->subtypes.map([&] (TyPtr t)
				{
					return newPoly(t, subs);
				}));

	default:
		return ty;
	}
}


std::string Ty::string () const
{
	Pretty pr;
	_string(pr);
	return pr.ss.str();
}

std::vector<std::string> Ty::stringAll (const TyList& tys)
{
	std::vector<std::string> res;
	Pretty pr;

	for (auto ty : tys)
	{
		pr.ss.str("");
		ty->_string(pr);
		res.push_back(pr.ss.str());
	}

	return res;
}

void Ty::_string (Pretty& pr) const
{
	switch (kind)
	{
	case tyConcrete:
		_concreteString(pr);
		break;

	case tyPoly:
		_polyString(pr);
		break;

	case tyOverloaded:
		pr.ss << "<overloaded function \"" << name << "\">";
		break;

	case tyWildcard:
		pr.ss << "_";
		break;

	default:
		pr.ss << "??";
		break;
	}
}

void Ty::_polyString (Pretty& pr) const
{
	if (!name.empty())
	{
		pr.ss << "\\" << name;
		return;
	}

	size_t i, len = pr.poly.size();

	for (i = 0; i < len; i++)
		if (pr.poly[i] == this)
			break;

	if (i >= len)
		pr.poly.push_back(this);

	pr.ss << "\\^";

	if (i <= 10)
		pr.ss << char('a' + i);
	else
		pr.ss << "t" << (i - 10);
}

void Ty::_concreteString (Pretty& pr) const
{
	if (name == "List")
	{
		pr.ss << "[";
		subtypes.front()->_string(pr);
		pr.ss << "]";
	}
	else if (name == "Fn" && !subtypes.nil())
	{
		pr.ss << '(';
		auto s = subtypes;
		for (size_t i = 0; !s.tail().nil(); ++s)
		{
			if (i++ > 0)
				pr.ss << ", ";
			s.head()->_string(pr);
		}
		pr.ss << ") -> ";
		s.head()->_string(pr);
	}
	else if (name == "Tuple")
	{
		pr.ss << '(';
		size_t i = 0;
		for (auto t : subtypes)
		{
			if (i++ > 0)
				pr.ss << ", ";
			t->_string(pr);
		}
		pr.ss << ')';
	}
	else
	{
		pr.ss << name;
		if (!subtypes.nil())
		{
			size_t i = 0;
			pr.ss << '(';
			for (auto t : subtypes)
			{
				if (i++ > 0)
					pr.ss << ", ";
				t->_string(pr);
			}
			pr.ss << ')';
		}
	}
}

