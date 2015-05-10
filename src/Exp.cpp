#include "Ast.h"
#include <iostream>
#include <cstring>


bool Sig::aEquiv (SigPtr other) const
{
	if (args.size() != other->args.size())
		return false;

	for (auto it1 = args.cbegin(), it2 = other->args.cbegin();
			it1 != args.cend(); it1++, it2++)
		if (!it1->second->aEquiv(it2->second))
			return false;

	return true;
}

TyList Sig::tyList (TyPtr ret) const
{
	auto res = ret == nullptr ? TyList() : TyList(ret);
	
	for (auto it = args.rbegin(); it != args.rend(); it++)
		res = TyList(it->second, res);
	return res;
}
std::string Sig::string () const
{
	std::ostringstream ss;

	ss << "(";

	size_t i, len = args.size();
	auto strs = Ty::stringAll(tyList());

	for (i = 0; i < len; i++)
	{
		if (i > 0)
			ss << ", ";
		ss << args[i].first << " : " << strs[i];
	}
	ss << ")";

	return ss.str();
}




Exp::Exp (ExpKind _kind, cExpList sub, const Span& sp)
	: kind(_kind), subexps(sub), span(sp), _type(nullptr), _strData("")
{
	memset(_primData, 0, sizeof(_primData));
}

Exp::Exp (ExpKind _kind, std::string data,
			cExpList sub, const Span& sp)
	: Exp(_kind, sub, sp)
{
	_strData = data;
}

Exp::~Exp ()
{
	// yuck
	if (kind == eLambda)
		delete get<Sig*>();
}


ExpPtr Exp::withSpan (const Span& newspan) const
{
	auto res = make(kind, _type, _strData, subexps, newspan);
	memcpy(res->_primData, _primData, sizeof(_primData));
	return res;
}
ExpPtr Exp::withSubexps (cExpList newsub) const
{
	auto res = make(kind, _type, _strData, newsub, span);
	memcpy(res->_primData, _primData, sizeof(_primData));
	return res;
}


std::string Exp::string (bool tag, int ind) const
{
	std::ostringstream ss;
	_string(ss, tag, ind, 0);
	return ss.str();
}

void Exp::_indent (std::ostringstream& ss, int ind)
{
	while (ind-- > 0)
		ss << ' ';
}

void Exp::_string (std::ostringstream& ss, bool tag, int increase, int ind) const
{
	static std::vector<std::string> kinds = {
		"invalid", "int", "real",
		"string", "bool", "var",
		"tuple", "call", "infix",
		"cond", "lambda", "block",
		"let", "^make", "^get", "^put"
	};

	if (tag)
		ss << "[" << kinds[int(kind)] << "] ";

	switch (kind)
	{
	case eInt:
		ss << get<int_t>();
		break;

	case eReal:
		ss << get<real_t>();
		break;

	case eString:
		ss << "\"" << _strData << "\"";
		break;

	case eBool:
		if (get<bool>())
			ss << "true";
		else
			ss << "false";
		break;

	case eVar:
		ss << _strData;
		break;

	case eTuple:
		ss << '(';
		for (size_t i = 0, len = subexps.size(); i < len; i++)
		{
			if (i > 0)
				ss << ", ";
			subexps[i]->_string(ss, tag, increase, ind + increase);
		}
		ss << ')';
		break;

	case eCond:
		ss << "if ";
		ind += increase;
		subexps[0]->_string(ss, tag, increase, ind);

		ss << std::endl;
		_indent(ss, ind);
		subexps[1]->_string(ss, tag, increase, ind);

		ind -= increase;
		ss << std::endl;
		_indent(ss, ind);
		ss << "else";
		ind += increase;
		
		ss << std::endl;
		_indent(ss, ind);
		subexps[2]->_string(ss, tag, increase, ind);
		break;
	
	case eLet:
		ss << "let " << _strData;
		
		ss << " : " << _type->string() << " = ";
		subexps[0]->_string(ss, tag, increase, ind);
		break;

	case eBlock:
		ss << '{';

		ind += increase;
		if (subexps.size() > 1)
			ss << std::endl;

		for (auto& e : subexps)
		{
			if (subexps.size() > 1)
				_indent(ss, ind);
			else
				ss << ' ';

			e->_string(ss, tag, increase, ind);

			if (subexps.size() > 1)
				ss << std::endl;
		}

		ind -= increase;
		if (subexps.size() > 1)
			_indent(ss, ind);
		else
			ss << ' ';

		ss << '}';
		break;

	case eCall:
		subexps[0]->_string(ss, tag, increase, ind);
		ss << '(';
		for (size_t i = 1, len = subexps.size(); i < len; i++)
		{
			if (i > 1)
				ss << ", ";

			subexps[i]->_string(ss, tag, increase, ind + increase);
		}
		ss << ')';
		break;

	case eInfix:
		subexps[0]->_string(ss, tag, increase, ind);
		for (size_t i = 1, len = subexps.size(); i < len; i++)
		{
			ss << ' ';
			subexps[i]->_string(ss, tag, increase, ind);
		}
		break;

	case eiMake:
		ss << "(^make " << getType()->string() << ")";
		break;

	case eiGet:
		ss << "^get " << getType()->string() << " "
		   << get<int_t>() << " ";
		subexps[0]->_string(ss, tag, increase, ind);
		break;

	case eLambda:
	default:
		ss << "< ?? >";
		return;
	}
}