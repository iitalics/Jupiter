#include "Ast.h"
#include "Desugar.h"

namespace Parse
{

void parseVar (Lexer& lex, std::string& name, TyPtr& ty, Span& sp)
{
	sp = lex.current().span;
	name = lex.eat(tIdent).str;

	if (lex.current() == tColon)
	{
		Span spEnd;

		lex.advance();
		ty = parseType(lex, spEnd);

		sp = sp + spEnd;
	}
	else
		ty = Ty::makeWildcard();
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

static FuncDecl parseConstructor (Lexer& lex)
{
	static auto expInvalid = Exp::make(eInvalid);
	Span spStart, spEnd;
	FuncDecl result;

	result.name = lex.current().str;
	spStart = lex.eat(tIdent).span;
	result.signature = parseSigParens(lex);
	spEnd = result.signature->span;
	result.span = spStart + spEnd;
	result.body = expInvalid;
	return result;
}
TypeDecl parseTypeDecl (Lexer& lex)
{
	Span spStart, spEnd, spType;
	std::vector<FuncDecl> ctors;

	spStart = lex.eat(tType).span;

	auto ty = parseType(lex, spType);

	if (ty->kind != tyConcrete)
		throw spType.die("invalid form of type in declaration");

	for (auto sub = ty->subtypes; !sub.nil(); ++sub)
	{
		auto sty = sub.head();

		if (sty->kind != tyPoly)
			throw spType.die("arguments to type must be polytypes");

		for (auto sty2 : sub.tail())
			if (sty2->kind == tyPoly &&
					sty->name == sty2->name)
				throw spType.die("identical polytypes in declaration");
	}

	lex.eat(tEqual);
	for (;;)
	{
		auto ctor = parseConstructor(lex);
		spEnd = ctor.span;
		ctors.push_back(ctor);

		if (lex.current() == tComma)
			lex.advance();
		else
			break;
	}

	return TypeDecl
		{
			ty->name,
			ty->subtypes,
			std::move(ctors),
			spStart + spEnd
		};
}


bool parseToplevel (Lexer& lex, GlobProto& proto)
{
	switch (lex.current().tok)
	{
	case tFunc:
		proto.funcs.push_back(parseFuncDecl(lex));
		return true;

	case tType:
		proto.types.push_back(parseTypeDecl(lex));
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
		auto expUnit = Exp::make(eTuple, {}, spStart);
		expThen->subexps.push_back(expUnit);
		expElse = expUnit; // no else => unit (zero tuple)
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
	parseVar(lex, name, ty, spEnd);
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
	Span spStart, spEnd, spType;

	spStart = lex.eat(tiMake).span;
	auto ty = parseType(lex, spType);
	spEnd = lex.current().span;
	auto tag = lex.eat(tString).str;

	if (ty->kind != tyConcrete || ty->name != "Fn")
		throw spType.die("^make expects function type");

	// TODO: hash 'tag' into a unique integer
	auto e = Exp::make(eiMake, int_t(0), {}, spStart + spEnd);
	e->setType(ty);

	return e;
}
ExpPtr parseiGet (Lexer& lex)
{
	// ^get <ty> <idx> <term>

	Span spStart, spEnd, spType;

	spStart = lex.eat(tiGet).span;
	auto ty = parseType(lex, spType);
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





TyPtr parseType (Lexer& lex, Span& sp)
{
	switch (lex.current().tok)
	{
	case tLambda:
		return parseTypePoly(lex, sp);
	case tLBrack:
		return parseTypeList(lex, sp);
	case tLParen:
		return parseTypeTuple(lex, sp);
	case tIdent:
		return parseTypeConcrete(lex, sp);

	case tWildcard:
		sp = lex.current().span;
		lex.advance();
		return Ty::makeWildcard();

	default:
		lex.expect("type");
		return nullptr;
	}
}
TyPtr parseTypePoly (Lexer& lex, Span& sp)
{
	Span spStart, spEnd;

	spStart = lex.eat(tLambda).span;
	spEnd = lex.current().span;
	auto name = lex.eat(tIdent).str;

	sp = spStart + spEnd;
	return Ty::makePoly(name);
}
TyPtr parseTypeList (Lexer& lex, Span& sp)
{
	Span spStart, spEnd;

	spStart = lex.eat(tLBrack).span;
	auto inner = parseType(lex, spEnd);
	spEnd = lex.eat(tRBrack).span;

	sp = spStart + spEnd;
	return Ty::makeConcrete("List", { inner });
}
static void parseTypeTupleRaw (Lexer& lex, std::vector<TyPtr>& out, Span& sp)
{
	Span spStart, spEnd;

	spStart = lex.eat(tLParen).span;
	while (lex.current() != tRParen)
	{
		out.push_back(parseType(lex, spEnd));

		if (lex.current() == tComma)
			lex.advance();
		else
			break;
	}
	spEnd = lex.eat(tRParen).span;

	sp = spStart + spEnd;
}
TyPtr parseTypeTuple (Lexer& lex, Span& sp)
{
	Span spStart, spEnd;

	std::vector<TyPtr> inners;
	parseTypeTupleRaw(lex, inners, spStart);

	if (lex.current() == tArrow)
	{
		lex.advance();
		inners.push_back(parseType(lex, spEnd));

		sp = spStart + spEnd;
		return Ty::makeFn(inners);
	}

	sp = spStart;

	if (inners.size() == 1)
		return inners[0];

	return Ty::makeConcrete("Tuple", inners);
}
TyPtr parseTypeConcrete (Lexer& lex, Span& sp)
{
	Span spStart, spEnd;

	std::vector<TyPtr> sub;
	spStart = lex.current().span;
	auto kind = lex.eat(tIdent).str;

	if (lex.current() == tLParen)
		parseTypeTupleRaw(lex, sub, spEnd);

	sp = spStart + spEnd;

	if (kind == "Fn" && sub.empty())
		throw sp.die("malformed function type");

	return Ty::makeConcrete(kind, sub);
}


};