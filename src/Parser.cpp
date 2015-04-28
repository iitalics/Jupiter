#include "Ast.h"

namespace Parse
{

void parseVar (Lexer& lex, std::string& name, TyPtr& ty, Span& sp)
{
	name = lex.eat(tIdent).str;

	if (lex.current() == tColon)
	{
		lex.advance();
		ty = parseType(lex);
	}
	else
		ty = Ty::makeWildcard();
}
void parseVar (Lexer& lex, std::string& name, TyPtr& ty)
{
	Span dummmy;
	parseVar(lex, name, ty, dummmy);
}
static void parseVar (Lexer& lex, SigPtr sig)
{
	std::string name;
	TyPtr ty;
	Span sp;

	parseVar(lex, name, ty, sp);

	for (auto& a : sig->args)
		if (a.first == name)
		{
			std::ostringstream ss;
			ss << "variable '" << name << "' already declared in signature";
			throw sp.die(ss.str());
		}

	sig->args.push_back(Sig::Arg { name, ty });
	sig->span = sig->span + sp;
}
SigPtr parseSig (Lexer& lex, bool anything)
{
	auto res = std::make_shared<Sig>();
	
	// sig := (<var> (',' <var>)*)?

	for (;; anything = true)
	{
		if (anything)
			lex.expect(tIdent);
		else if (lex.current() != tIdent)
			break;

		parseVar(lex, res);

		if (lex.current() == tComma)
			lex.advance();
		else
			break;
	}

	return res;
}
SigPtr parseSigParens (Lexer& lex)
{
	Span spStart, spEnd;

	spStart = lex.eat(tLParen).span;
	auto res = parseSig(lex, false);
	spEnd = lex.eat(tRParen).span;

	res->span = spStart + spEnd;
	return res;
}





FuncDecl parseFuncDecl (Lexer& lex)
{
	Span spStart, spEnd;

	spStart = lex.eat(tFunc).span;
	auto name = lex.eat(tIdent).str;
	auto sig = parseSigParens(lex);
	
	ExpPtr body;
/*	if (lex.current() == tEqual)
	{
		lex.advance();
		body = parseExp(lex);
	}
	else */
		body = parseBlock(lex);

	spEnd = sig->span;

	return FuncDecl
		{
			.name = name,
			.signature = sig,
			.body = body,
			.span = spStart + spEnd
		};
}

bool parseToplevel (Lexer& lex, GlobProto& proto)
{
	switch (lex.current().tok)
	{
	case tFunc:
		proto.funcs.push_back(parseFuncDecl(lex));
		return true;

	default:
		return false;
	}
}

GlobProto parseToplevel (Lexer& lex)
{
	GlobProto res;
	while (parseToplevel(lex, res)) ;
	return res;
}





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
			e = Exp::make(eVar, tok.str, bool(true), {}, tok.span);
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
//	else if (vals.size() == 3)
//		return Exp::make(eCall, { vals[1], vals[0], vals[2] },
//							spStart + spEnd);
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
		return Exp::make(eVar, tok.str, bool(true), {}, tok.span);

	case tLParen:
		return parseTuple(lex);
	case tIf:
		return parseCond(lex);
	case tLCurl:
		return parseBlock(lex);

	case tiMake:
		return parseiMake(lex);
	case tiGet:
		return parseiGet(lex);
	case tiPut:
		return parseiPut(lex);

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
		if (blockRequired)
		{
			// if {} else {}
			// if {} else if ...
			
			if (lex.current() != tIf)
				lex.expect(tLCurl);
			expElse = parseExp(lex);
		}
		else
			expElse = parseExp(lex);

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

	case tLet:
		list.push_back(parseLet(lex));
		lex.eat(tSemicolon);
		break;

	// these expression don't proceeding require semicolons
	case tLCurl:
	case tIf:
		list.push_back(parseExp(lex));
		break;

	// everthing else does
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
	std::string name;
	TyPtr ty;
	ExpPtr init;

	// 'let' <var> '=' <exp> ';'

	spStart = lex.eat(tLet).span;
	parseVar(lex, name, ty);
	lex.eat(tEqual);
	init = parseExp(lex);
	spEnd = init->span;

	auto e = Exp::make(eLet, ty, name, { init }, spStart + spEnd);
	e->set<int>(-1);
	return e;
}

ExpPtr parseiMake (Lexer& lex)
{
	// ^make <ty>
	Span spStart, spEnd;

	spStart = lex.eat(tiMake).span;
	auto ty = parseType(lex);
	spEnd = lex.current().span;
	auto tag = lex.eat(tString).valueInt;

	if (ty->kind != tyConcrete || ty->name != "Fn")
		throw spStart.die("^make expects function type");

	// TODO: hash 'tag' into a unique integer
	auto e = Exp::make(eiMake, int_t(0), {}, spStart + spEnd);
	e->setType(ty);

	return e;
}
ExpPtr parseiGet (Lexer& lex)
{
	// ^get <ty> <idx> <term>

	Span spStart, spEnd;

	spStart = lex.eat(tiGet).span;
	auto ty = parseType(lex);
	auto idx = lex.eat(tNumberInt).valueInt;
	auto body = parseTerm(lex);
	spEnd = body->span;

	auto e = Exp::make(eiGet, idx, { body }, spStart + spEnd);
	e->setType(ty);

	return e;
}
ExpPtr parseiPut (Lexer& lex)
{
	throw lex.current().span.die("^put unimplemented");
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

	return Ty::makePoly(name);
}
TyPtr parseTypeList (Lexer& lex)
{
	lex.eat(tLBrack);
	auto inner = parseType(lex);
	lex.eat(tRBrack);

	return Ty::makeConcrete("List", { inner });
}
static void parseTypeTupleRaw (Lexer& lex, std::vector<TyPtr>& out)
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
	std::vector<TyPtr> inners;
	parseTypeTupleRaw(lex, inners);

	if (lex.current() == tArrow)
	{
		lex.advance();
		inners.push_back(parseType(lex));

		return Ty::makeFn(inners);
	}

	if (inners.size() == 1)
		return inners[0];

	return Ty::makeConcrete("Tuple", inners);
}
TyPtr parseTypeConcrete (Lexer& lex)
{
	std::vector<TyPtr> sub;
	auto sp = lex.current().span;
	auto kind = lex.eat(tIdent).str;

	if (lex.current() == tLParen)
		parseTypeTupleRaw(lex, sub);

	if (kind == "Fn" && sub.empty())
		throw sp.die("malformed function type");

	return Ty::makeConcrete(kind, sub);
}


};