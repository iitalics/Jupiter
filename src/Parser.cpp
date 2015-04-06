#include "Ast.h"

namespace Parse
{

ExpPtr parseExp (Lexer& lex)
{
	Span spStart, spEnd;
	ExpList vals;
	ExpPtr e;
	Token tok;

	e = parseTerm(lex);
	vals.push_back(e);
	spStart = spEnd = e->span;

	while (lex.current() == tIdent)
		if (lex.current().isOperator())
		{
			// <op>
			tok = lex.advance();
			e = Exp::make(eVar, tok.str, {}, tok.span);
			vals.push_back(e);

			// <term>
			e = parseTerm(lex);
			vals.push_back(e);

			spEnd = e->span;
		}
		else
			lex.expect("operator");

	if (vals.size() == 1)
		return vals[0];
	else if (vals.size() == 3)
		return Exp::make(eCall, { vals[1], vals[0], vals[2] },
							spStart + spEnd);
	else
		return Exp::make(eInfix, vals, spStart + spEnd);
}
ExpPtr parseTerm (Lexer& lex)
{
	return parseTermPostfix(lex, parseTermPrefix(lex));
}
ExpPtr parseTermPrefix (Lexer& lex)
{
	Token tok = lex.current();

	switch (tok.tok)
	{
	case tNumberInt:
		tok = lex.advance();
		return Exp::make(eInt, tok.valueInt, {}, tok.span);

	case tNumberReal:
		tok = lex.advance();
		return Exp::make(eReal, tok.valueReal, {}, tok.span);

	case tString:
		tok = lex.advance();
		return Exp::make(eString, tok.str, {}, tok.span);

	case tTrue:
	case tFalse:
		tok = lex.advance();
		return Exp::make(eBool, tok == tTrue, {}, tok.span);

	case tIdent:
		tok = lex.advance();
		return Exp::make(eVar, tok.str, {}, tok.span);

	case tLParen:
		return parseTuple(lex);

	case tIf:
		return parseCond(lex);

	case tLCurl:
		return parseBlock(lex);

	default:
		//lex.expect("term");
		lex.unexpect();
		return nullptr;
	}
}
ExpPtr parseTermPostfix (Lexer& lex, ExpPtr in)
{
	for (;;)
		switch (lex.current().tok)
		{
		case tLParen:
			in = parseCall(lex, in);
			break;

		default:
			return in;
		}
}
ExpPtr parseCall (Lexer& lex, ExpPtr in)
{
	auto args = parseTuple(lex);
	ExpList exps;

	exps.reserve(args->subexps.size() + 1);
	exps.push_back(in);
	exps.insert(exps.end(), args->subexps.begin(), args->subexps.end());

	return Exp::make(eCall, exps, in->span + args->span);
}

//		()          OK
//		(a)         OK
//		(a,b)       OK
//		(a,b,)      OK
//		(a b)       BAD

ExpPtr parseTuple (Lexer& lex)
{
	ExpList exps;
	ExpPtr e;
	Span spStart, spEnd;

	spStart = lex.eat(tLParen).span;

	if (lex.current() != tRParen)
		while (lex.current() != tRParen)
		{
			e = parseExp(lex);
			exps.push_back(e);

			if (lex.current() == tComma)
				lex.advance();
			else
				break;
		}
	
	spEnd = lex.eat(tRParen).span;

	return Exp::make(eTuple, exps, spStart + spEnd);
}

ExpPtr parseCond (Lexer& lex)
{
	Span spStart, spEnd;
	ExpPtr expCond, expThen, expElse;
	bool blockRequired = true;

	spStart = lex.eat(tIf).span;

	expCond = parseExp(lex);

	if (lex.current() == tThen)
	{
		lex.advance();
		blockRequired = false;
	}

	expThen = blockRequired ? 
				parseBlock(lex) :
				parseExp(lex);

	if (lex.current() == tElse)
	{
		lex.advance();
		expElse = blockRequired ?
					parseBlock(lex) :
					parseExp(lex);

		spEnd = expElse->span;
	}
	else if (!blockRequired)
		lex.expect(tElse);
	else
	{
		expElse = Exp::make(eTuple, {}, spStart); // no else => unit (zero tuple)
		spEnd = expThen->span;
	}

	return Exp::make(eCond, { expCond, expThen, expElse }, spStart + spEnd);
}

ExpPtr parseBlock (Lexer& lex)
{
	Span spStart, spEnd;
	ExpList exps;

	spStart = lex.eat(tLCurl).span;

	while (lex.current() != tRCurl)
		parseBlockExp(lex, exps);

	spEnd = lex.eat(tRCurl).span;

	return Exp::make(eBlock, exps, spStart + spEnd);
}
void parseBlockExp (Lexer& lex, ExpList& list)
{
	switch (lex.current().tok)
	{
	case tSemicolon:
		lex.advance();
		break;

	case tIf:
		list.push_back(parseCond(lex));
		break;

	case tLet:
		list.push_back(parseLet(lex));
		lex.eat(tSemicolon);
		break;

	default:
		list.push_back(parseExp(lex));
		if (lex.current() != tSemicolon)
			lex.expect(tRCurl);
		else
			lex.eat(tSemicolon);
		break;
	}
}
ExpPtr parseLet (Lexer& lex)
{
	Span spStart, spEnd;
	std::string var;
	ExpPtr init;

	// 'let' <id> '=' <exp> ';'

	spStart = lex.eat(tLet).span;
	var = lex.eat(tIdent).str;
	lex.eat(tEqual);
	init = parseExp(lex);
	spEnd = init->span;

	return Exp::make(eLet, var, { init }, spStart + spEnd);
}





TyPtr parseType (Lexer& lex)
{
	switch (lex.current().tok)
	{
	case tLambda:
		return parseTypePoly(lex);
	case tLBrack:
		return parseTypeList(lex);
	case tLParen:
		return parseTypeTuple(lex);
	case tIdent:
		return parseTypeConcrete(lex);

	case tWildcard:
		lex.advance();
		return Ty::makeWildcard();

	default:
		lex.expect("type");
		return nullptr;
	}
}
TyPtr parseTypePoly (Lexer& lex)
{
	lex.eat(tLambda);
	auto name = lex.eat(tIdent).str;

	return Ty::makePolyNamed(name);
}
TyPtr parseTypeList (Lexer& lex)
{
	lex.eat(tLBrack);
	auto inner = parseType(lex);
	lex.eat(tRBrack);

	return Ty::makeConcrete("List", { inner });
}
static void parseTypeTupleRaw (Lexer& lex, TyList& out)
{
	lex.eat(tLParen);
	while (lex.current() != tRParen)
	{
		out.push_back(parseType(lex));

		if (lex.current() == tComma)
			lex.advance();
		else
			break;
	}
	lex.eat(tRParen);
}
TyPtr parseTypeTuple (Lexer& lex)
{
	TyList inners;
	parseTypeTupleRaw(lex, inners);

	if (lex.current() == tArrow)
	{
		lex.advance();
		inners.push_back(parseType(lex));

		return Ty::makeConcrete("Fn", inners);
	}

	if (inners.size() == 1)
		return inners[0];

	return Ty::makeConcrete("Tuple", inners);
}
TyPtr parseTypeConcrete (Lexer& lex)
{
	TyList sub;
	auto kind = lex.eat(tIdent).str;

	if (lex.current() == tLParen)
		parseTypeTupleRaw(lex, sub);

	return Ty::makeConcrete(kind, sub);
}


};