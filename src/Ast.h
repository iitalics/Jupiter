#pragma once
#include "Lexer.h"
#include "Types.h"
#include <sstream>
#include <iostream>
#include <memory>
#include <algorithm>

struct Sig
{
	using Arg = std::pair<std::string, TyPtr>;
	using ArgList = std::vector<Arg>;

	inline static
	SigPtr make (const ArgList& args = {},
		            const Span& span = Span())
	{
		return SigPtr(new Sig { args, span });
	}

	ArgList args;
	Span span;

	bool aEquiv (SigPtr other) const;

	TyList tyList (TyPtr ret = nullptr) const;
	std::string string (bool paren = true) const;

	/* (x : A, y : B) -> Sig(x, A, y, B) */

	TyPtr toSigType () const;
	static SigPtr fromSigType (TyPtr ty,
	                 const Span& sp = Span());
};



enum ExpKind
{
	// KIND         //  DATA
	eInvalid = 0,
	eInt,           //  int_t
	eReal,          //  real_t
	eString,        //  string
	eBool,          //  bool
	eVar,           //  string, bool
	eTuple,
	eCall,
	eMem,           // string
	eInfix,
	eCond,
	eLambda,
	eBlock,
	eLet,           //  string, type
	eiMake,         //  string, int_t, type
	eiGet,          //  int_t, type
	eiPut,          //  int_t
	eiTag,          //  string
	eiCall,         //  string, type
	eiEnv
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
	Span span;
};

struct TypeDecl
{
	std::string name;
	TyList polytypes;
	std::vector<FuncDecl> ctors;
	Span span;
};

struct GlobProto
{
	std::vector<FuncDecl> funcs;
	std::vector<TypeDecl> types;
};


namespace Parse
{


FuncDecl parseFuncDecl (Lexer& lex);
TypeDecl parseTypeDecl (Lexer& lex);
bool parseToplevel (Lexer& lex, GlobProto& proto);
GlobProto parseToplevel (Lexer& lex);

void parseVar (Lexer& lex, std::string& name, TyPtr& ty, Span&);
SigPtr parseSig (Lexer& lex, bool requireAnything = true);
SigPtr parseSigParens (Lexer& lex);

ExpPtr parseExp (Lexer& lex);
ExpPtr parseTerm (Lexer& lex);
ExpPtr parseTermPrefix (Lexer& lex);
ExpPtr parseTermPostfix (Lexer& lex, ExpPtr in);
ExpPtr parseCall (Lexer& lex, ExpPtr in);
ExpPtr parseMem (Lexer& lex, ExpPtr in);
ExpPtr parseTuple (Lexer& lex);
ExpPtr parseCond (Lexer& lex);
ExpPtr parseLambda (Lexer& lex);
ExpPtr parseBlock (Lexer& lex);
void parseBlockExp (Lexer& lex, ExpList& list);
ExpPtr parseLet (Lexer& lex);
ExpPtr parseiMake (Lexer& lex);
ExpPtr parseiGet (Lexer& lex);
ExpPtr parseiPut (Lexer& lex);
ExpPtr parseiCall (Lexer& lex);

//TyPtr parseType (Lexer& lex);
TyPtr parseType (Lexer& lex, Span& sp);
TyPtr parseTypePoly (Lexer& lex, Span& sp);
TyPtr parseTypeList (Lexer& lex, Span& sp);
TyPtr parseTypeTuple (Lexer& lex, Span& sp);
TyPtr parseTypeConcrete (Lexer& lex, Span& sp);

};
