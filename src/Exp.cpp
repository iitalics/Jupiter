#include "Ast.h"
#include <iostream>


Exp::Exp (ExpKind _kind, cExpList sub, const Span& sp)
	: kind(_kind), subexps(sub), span(sp), _strData("") {}

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
		"let",
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
	
	case eLet:			// 	std::string, Type?
		ss << "let " << _strData << " = ";
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

	case eLambda:
	default:
		ss << "< ?? >";
		return;
	}
}