#pragma once
#include "Lexer.h"
#include "Types.h"
#include <sstream>
#include <iostream>
#include <memory>
#include <algorithm>

class Exp;
struct Sig;
using ExpPtr = std::shared_ptr<Exp>;
using SigPtr = std::shared_ptr<Sig>;
using ExpList = std::vector<ExpPtr>;


struct Sig
{
	using Arg = std::pair<std::string, TyPtr>;
	using ArgList = std::vector<Arg>;

	ArgList args;
	Span span;

	inline Sig (const ArgList& _args = {})
		: args(_args) {}

	TyList tyList () const;
	std::string string () const;
};



enum ExpKind
{
	// KIND        	//	INTERNAL
	eInvalid = 0,
	eInt,			//	int_t
	eReal,			//	real_t
	eString,		//  std::string
	eBool,			// 	bool
	eVar,			//	std::string
	eTuple,
	eCall,
	eInfix,
	eCond,
	eLambda,		// 	Sig*
	eBlock,
	eLet,			// 	std::string, int
};

class Exp
{
public:
	using cExpList = const ExpList&;

	// make (kind, subexps, span)
	inline static ExpPtr
	make (ExpKind k = eInvalid,
			cExpList l = {},
			const Span& s = Span())
	{
		return std::make_shared<Exp>(k, l, s);
	}

	// make (kind, data, subexps, span)
	template <typename T>
	inline static ExpPtr
	make (ExpKind k, T d,
			cExpList l = {},
			const Span& s = Span())
	{
		return std::make_shared<Exp>(k, d, l, s);
	}

	// make (kind, str, data, subexps, span)
	template <typename T>
	inline static ExpPtr
	make (ExpKind k, const std::string& str, T d,
			cExpList l = {},
			const Span& s = Span())
	{
		return ExpPtr(
			(new Exp(k, d, l, s))->setString(str));
	}

	// make (kind, type, str, subexps, span)
	inline static ExpPtr
	make (ExpKind k, TyPtr ty, const std::string& str,
			cExpList l = {},
			const Span& s = Span())
	{
		return ExpPtr(
			(new Exp(k, str, l, s))->setType(ty));
	}



	ExpKind kind;
	ExpList subexps;
	Span span;


	ExpPtr withSpan (const Span& newspan) const;
	ExpPtr withSubexps (cExpList sub) const;
	template <typename F>
	ExpPtr mapSubexps (F operation) const
	{
		ExpList temp;
		temp.resize(subexps.size());

		std::transform(subexps.begin(),
			           subexps.end(),
			           temp.begin(),
			           operation);

		return withSubexps(temp);
	}


	explicit Exp (ExpKind _kind = eInvalid, cExpList sub = {},
					const Span& sp = Span());

	Exp (ExpKind _kind, std::string data,
			cExpList sub = {}, const Span& sp = Span());

	template <typename T>
	Exp (ExpKind _kind, T data, cExpList sub = {},
					const Span& sp = Span())
		: Exp(_kind, sub, sp)
	{
		*(T*)_primData = data;
	}

	~Exp ();



	const std::string& getString () const { return _strData; }
	const Sig& getSig () const { return *(Sig*)_primData; }
	TyPtr getType () const { return _type; }

	template <typename T>
	T get () const { return *(T*)_primData; }

	inline Exp* setString (const std::string& s) { _strData = s; return this; }
	inline Exp* setType (TyPtr t) { _type = t; return this; }
	template <typename T>
	inline Exp* set (T t) { *(T*)_primData = t; return this; }

	inline bool is (ExpKind k) const { return kind == k; }

	std::string string (bool tag = false, int ind = 2) const;

private:
	static void _indent (std::ostringstream& ss, int ind);
	void _string (std::ostringstream& ss, bool tag, int incr, int ind) const;

	TyPtr _type;
	std::string _strData;
	char _primData[16];
};



struct FuncDecl
{
	std::string name;
	SigPtr signature;
	ExpPtr body;
};

struct TypeDecl {};



namespace Parse
{

enum Parsed
{
	Nothing,
	ParsedFunc,
	ParsedType,
};

bool parseFuncDecl (Lexer& lex, FuncDecl& out);
bool parseTypeDecl (Lexer& lex, TypeDecl& out);
Parsed parseToplevel (Lexer& lex,
						FuncDecl& outf,
						TypeDecl& outt);

void parseVar (Lexer& lex, std::string& name, TyPtr& ty, Span&);
void parseVar (Lexer& lex, std::string& name, TyPtr& ty);
SigPtr parseSig (Lexer& lex, bool requireAnything = true);
SigPtr parseSigParens (Lexer& lex);

ExpPtr parseExp (Lexer& lex);
ExpPtr parseTerm (Lexer& lex);
ExpPtr parseTermPrefix (Lexer& lex);
ExpPtr parseTermPostfix (Lexer& lex, ExpPtr in);
ExpPtr parseCall (Lexer& lex, ExpPtr in);
ExpPtr parseTuple (Lexer& lex);
ExpPtr parseCond (Lexer& lex);
ExpPtr parseBlock (Lexer& lex);
void parseBlockExp (Lexer& lex, ExpList& list);
ExpPtr parseLet (Lexer& lex);

TyPtr parseType (Lexer& lex);
TyPtr parseTypePoly (Lexer& lex);
TyPtr parseTypeList (Lexer& lex);
TyPtr parseTypeTuple (Lexer& lex);
TyPtr parseTypeConcrete (Lexer& lex);

};
