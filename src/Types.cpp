#include "Types.h"
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
		break;

	case tyPoly:
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


