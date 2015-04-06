#pragma once
#include "Lexer.h"
#include "Types.h"
#include <sstream>
#include <iostream>
#include <memory>

class Exp;
class Sig;
using ExpPtr = std::shared_ptr<Exp>;
using SigPtr = std::shared_ptr<Sig>;
using ExpList = std::vector<ExpPtr>;


class Sig
{
	std::vector<std::string> names;
	TyList types;
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
	eLet,			// 	std::string + Typ
};

class Exp
{
public:
	using cExpList = const ExpList&;

	inline static ExpPtr
	make (ExpKind k = eInvalid,
			cExpList l = {},
			const Span& s = Span())
	{
		return std::make_shared<Exp>(k, l, s);
	}

	template <typename T>
	inline static ExpPtr
	make (ExpKind k, T d,
			cExpList l = {},
			const Span& s = Span())
	{
		return std::make_shared<Exp>(k, d, l, s);
	}



	ExpKind kind;
	ExpList subexps;
	Span span;




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

	template <typename T>
	T get () const { return *(T*)_primData; }

	inline bool is (ExpKind k) const { return kind == k; }

	std::string string (bool tag = false, int ind = 2) const;

private:
	static void _indent (std::ostringstream& ss, int ind);
	void _string (std::ostringstream& ss, bool tag, int incr, int ind) const;

	std::string _strData;
	char _primData[32];
};






namespace Parse
{

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
