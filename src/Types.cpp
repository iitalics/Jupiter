#include "Types.h"
#include <sstream>
#include <iostream>



Ty::Ty (TyKind k)
	: kind(k), name(""), idx(-1) {}

Ty::~Ty () {}

TyPtr Ty::makeConcrete (const std::string& t, const TyList& sub)
{
	auto ty = std::make_shared<Ty>(tyConcrete);
	ty->subtypes = sub;
	ty->name = t;
	return ty;
}
TyPtr Ty::makePoly (int idx)
{
	auto ty = std::make_shared<Ty>(tyPoly);
	ty->idx = idx;
	return ty;
}
TyPtr Ty::makePolyNamed (const std::string& name)
{
	auto ty = std::make_shared<Ty>(tyPolyNamed);
	ty->name = name;
	return ty;
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
		ss << name;
		if (subtypes.size() > 0)
		{
			ss << '(';
			for (size_t i = 0, len = subtypes.size(); i < len; i++)
			{
				if (i > 0)
					ss << ", ";
				subtypes[i]->_string(ss);
			}
			ss << ')';
		}
		break;

	case tyPoly:
		ss << '\\';
		if (idx < 10)
			ss << char('a' + idx);
		else
			ss << 't' << (idx - 10);
		break;
	case tyPolyNamed:
		ss << '\\' << name;
		break;

	case tyOverloaded:
		ss << "<overload>";
		break;

	case tyWildcard:
		ss << "_";
		break;

	default:
		ss << "??";
		break;
	}
}


