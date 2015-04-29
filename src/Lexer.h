#pragma once
#include "Jupiter.h"
#include <sstream>
#include <istream>
#include <utility>

enum
{
	// 0 .. 255  :=  [ ] ( ) { } . , ;

	tEOF = 0x10000,	// <eof>
	tNumber,
	tNumberInt, 	// <int>
	tNumberReal, 	// <real>
	tIdent, 		// <ident>
	tString, 		// <string>

	tIf,
	tThen,
	tElse,
	tFunc,
	tLet,
	tLoop,
	tFor,
	tTrue,
	tFalse,
	
	tiMake, tiGet, tiPut,

	tEqual,         // =
	tWildcard,		// _
	tColon,			// :
	tArrow,			// ->
	tFatArrow,		// =>

	tLambda = '\\', tSemicolon = ';',
	tLParen = '(', tRParen = ')',
	tLCurl = '{', tRCurl = '}',
	tLBrack = '[', tRBrack = ']',
	tComma = ',', tDot = '.'
};



class LexFile
{
public:
	using ptr = std::shared_ptr<LexFile>;
	~LexFile ();


	static ptr make (const std::string& fname, std::istream& is)
	{
		return std::shared_ptr<LexFile>(
			new LexFile(fname, is));
	}
	static ptr make (const std::string& fname, const std::string& contents)
	{
		std::istringstream ss(contents);

		return std::shared_ptr<LexFile>(
			new LexFile(fname, ss));
	}


	inline std::string filename () const { return _fname; }
	inline size_t filesize () const { return _data.size(); }
	inline bool badfile () const { return _badfile; }

	char get (size_t pos) const;
	std::string get (size_t start, size_t end) const;

private:
	LexFile (const std::string& fname, std::istream& is);

	std::string _fname;
	std::vector<char> _data;
	bool _badfile;
};

struct Span
{
	LexFile::ptr file;
	size_t start, end;

	using Error = std::runtime_error;

	inline explicit
	Span (const LexFile::ptr& _file,
					size_t _start, size_t _end)
		: file(_file), start(_start), end(_end) {}

	inline explicit
	Span (const LexFile::ptr& _file = nullptr,
					size_t _start = 0)
		: file(_file), start(_start), end(_start) {}

	inline int length () const { return end - start; }
	inline bool invalid () const
	{ return length() <= 0 || file == nullptr; }

	// combine spans 
	Span operator+ (const Span& other) const;
	
	// '+' = add to length
	inline Span operator+ (size_t len) const
	{ return Span(file, start, end + len); }
	
	// '*' = set length
	inline Span operator* (size_t len) const 
	{ return Span(file, start, start + len); }

	inline Span& operator*= (size_t len)
	{
		end = start + len;
		return *this;
	}

	inline std::string data () const
	{
		if (file == nullptr)
			return "";
		else
			return file->get(start, end);
	}

	Error die (const std::string& msg,
	              const std::vector<std::string>& extra = {}) const;
};

struct Token
{
	static std::string string (int tok);

	int tok;
	Span span;

	std::string str;
	union
	{
		int_t valueInt;
		real_t valueReal;
	};

	inline explicit Token (const Span& sp = Span())
		: tok(tEOF), span(sp) {}

	std::string string () const;
	bool isOperator () const;

	bool operator== (int tok) const;
	inline bool operator!= (int tok) const
	{
		return !(*this == tok);
	}

	inline std::runtime_error die (const std::string& msg) const
	{
		return span.die(msg);
	}

};



class Lexer
{
public:
	Lexer ();
	~Lexer ();

	void openFile (const std::string& filename);
	void openString (const std::string& contents, 
						const std::string& filename = "<input>");

	inline const Token& current () const { return _current; }

	Token advance (); // returns old token and advances
	Token expect (int tok); // if current() != tok { unexpect() }
	void expect (const std::string& word);
	void unexpect (); // throw "unexpected token"
	Token eat (int tok); // expect(tok); advance()
	Token eat (int tok1, int tok2); // tok1 or tok2


	static std::vector<std::pair<std::string, int>> keywords;

	static void generateKeyMaps ();
private:
	LexFile::ptr _file;
	size_t _filepos;
	Token _current;

	void _open ();

	void _trim ();
	bool _eof ();
	char _peek ();
	char _adv ();

	void _number (const std::string& str);
	void _ident ();
	void _string ();
};