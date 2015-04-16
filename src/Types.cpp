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

	if (name.empty())
	{
		static int idx = 0;
		std::ostringstream ss;
		
		if (idx <= 10)
			ss << char('a' + idx);
		else
			ss << "t" << (idx - 10);
		idx++;

		ty->name = ss.str();
	}
	else
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
	std::ostringstream ss;
	_string(ss);
	return ss.str();
}

void Ty::_string (std::ostringstream& ss) const
{
	switch (kind)
	{
	case tyConcrete:
		_concreteString(ss);
		break;

	case tyPoly:
		ss << '\\' << name;
		break;

	case tyOverloaded:
		ss << "<overloaded function \"" << name << "\">";
		break;

	case tyWildcard:
		ss << "_";
		break;

	default:
		ss << "??";
		break;
	}
}

void Ty::_concreteString (std::ostringstream& ss) const
{
	if (name == "List")
	{
		ss << "[";
		subtypes.front()->_string(ss);
		ss << "]";
	}
	else if (name == "Fn" && !subtypes.nil())
	{
		ss << '(';
		auto s = subtypes;
		for (size_t i = 0; !s.tail().nil(); ++s)
		{
			if (i++ > 0)
				ss << ", ";
			s.head()->_string(ss);
		}
		ss << ") -> ";
		s.head()->_string(ss);
	}
	else if (name == "Tuple")
	{
		ss << '(';
		size_t i = 0;
		for (auto t : subtypes)
		{
			if (i++ > 0)
				ss << ", ";
			t->_string(ss);
		}
		ss << ')';
	}
	else
	{
		ss << name;
		if (!subtypes.nil())
		{
			size_t i = 0;
			ss << '(';
			for (auto t : subtypes)
			{
				if (i++ > 0)
					ss << ", ";
				t->_string(ss);
			}
			ss << ')';
		}
	}
}

