#include "Lexer.h"
#include <iostream>
#include <fstream>
#include <algorithm>


// configurable?

enum
{
	DIGIT_MASK = 1,
	CONTROL_MASK = 2,
	SPACE_MASK = 4,
	IDENT_MASK = 8,
	OPERATOR_MASK = 16,
};

static const char COMMENT = '#';


#ifndef NO_KEY_MAPS
#define USE_KEY_MAP
#endif

#ifdef USE_KEY_MAP

static int key_map[128] = {
	4,24,24,24,24,24,24,24,4,4,4,24,24,4,24,24,
	24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,24,
	4,24,0,0,24,24,24,24,2,2,24,24,2,24,2,24,
	25,25,25,25,25,25,25,25,25,25,24,2,24,24,24,24,
	24,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,2,2,2,24,24,
	24,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,2,24,2,24,24,
};




static bool is_digit (char c)    { return key_map[int(c)] & DIGIT_MASK; }
static bool is_control (char c)  { return key_map[int(c)] & CONTROL_MASK; }
static bool is_space (char c)    { return key_map[int(c)] & SPACE_MASK; }
static bool is_ident (char c)    { return key_map[int(c)] & IDENT_MASK; }
static bool is_operator (char c) { return key_map[int(c)] & OPERATOR_MASK; }

#else


static bool is_digit (char c)
{
	if (c == COMMENT) return false;
	return c >= '0' && c <= '9';
}
static bool is_control (char c)
{
	if (c == COMMENT) return false;
	return c == tDot || c == tComma ||
		c == tLambda || c == tSemicolon ||
		c == tLParen || c == tRParen ||
		c == tLCurl  || c == tRCurl ||
		c == tLBrack || c == tRBrack;
}
static bool is_space (char c)
{
	if (c == COMMENT) return false;
	return c == ' '  || c == '\t' ||
	       c == '\n' || c == '\r' ||
	       c == '\b' || c == '\0';
}
static bool is_ident (char c)
{
	if (c == COMMENT) return false;
	return c != '\"' && !is_space(c) && !is_control(c);
}
static bool is_operator (char c)
{
	if (c == COMMENT) return false;
	return is_ident(c) &&
	       !((c >= 'a' && c <= 'z') ||
		     (c >= 'A' && c <= 'Z'));
}

#endif

std::vector<std::pair<std::string, int>> Lexer::keywords {
	{ "if",    tIf },
	{ "then",  tThen },
	{ "else",  tElse },
	{ "func",  tFunc },
	{ "type",  tType },
	{ "let",   tLet },
	{ "loop",  tLoop },
	{ "for",   tFor },
	{ "true",  tTrue },
	{ "false", tFalse },
	{ "pub",   tPub },
	{ "import",tImport },
	{ "^make", tiMake },
	{ "^get",  tiGet },
	{ "^put",  tiPut },
	{ "^tag?", tiTag },
	{ "^call", tiCall },
	{ "=",     tEqual },
	{ "_",     tWildcard },
	{ ":",     tColon },
	{ "->",    tArrow },
//	{ "=>",    tFatArrow }
};

static const char PRETTY_QUOTE = '\'';
static const char PRETTY_QUOTE_KEYWORD = '`';
static const size_t MAX_STRING_DISPLAY_LEN = 5;








void Lexer::generateKeyMaps ()
{
	const int NKEYS = 128;

	int map[NKEYS];

	size_t i;

	for (i = 0; i < NKEYS; i++)
	{
		map[i] = 0;

		if (is_digit(char(i)))
			map[i] |= DIGIT_MASK;
		if (is_control(char(i)))
			map[i] |= CONTROL_MASK;
		if (is_space(char(i)))
			map[i] |= SPACE_MASK;
		if (is_ident(char(i)))
			map[i] |= IDENT_MASK;
		if (is_operator(char(i)))
			map[i] |= OPERATOR_MASK;
	}

	std::cout << "static int key_map[" << NKEYS << "] = {";
	for (i = 0; i < NKEYS; i++)
	{
		if (i % 16 == 0)
			std::cout << "\n\t";
		std::cout << map[i] << ",";
	}
	std::cout << "\n};" << std::endl;
}









LexFile::LexFile (const std::string& fname, std::istream& is)
	: _fname(fname), _badfile(!is.good())
{
	char buffer[512];
	size_t num;

	for (;;)
	{
		is.read(buffer, sizeof(buffer));
		num = is.gcount();

		if (num == 0)
			break;

		_data.insert(_data.end(), buffer, buffer + num);
	}
}
LexFile::~LexFile () {}
char LexFile::get (size_t pos) const
{
	if (pos >= _data.size())
		return '\0';
	else
		return _data[pos];
}
std::string LexFile::get (size_t start, size_t end) const
{
	end = std::min(end, _data.size());
	start = std::min(start, end);

	if (start == end)
		return "";
	else
		return std::string(_data.data() + start, end - start);
}





Lexer::Lexer ()
	: _file(nullptr) {}

Lexer::~Lexer () {}

void Lexer::openFile (const std::string& filename)
{
	std::ifstream fs(filename);
	_file = LexFile::make(filename, fs);

	if (_file->badfile())
	{
		std::ostringstream ss;
		ss << "cannot open file '" << filename << "'";
		throw Span(_file).die(ss.str());
	}
	else
		_open();
}
void Lexer::openString (const std::string& contents, 
					const std::string& filename)
{
	_file = LexFile::make(filename, contents);
	_open();
}


void Lexer::_open ()
{
	_filepos = 0;
	advance();
}
bool Lexer::_eof ()
{
	return _filepos >= _file->filesize();
}

char Lexer::_peek ()
{
	return _file->get(_filepos);
}
char Lexer::_adv ()
{
	auto c = _peek();
	_filepos++;
	return c;
}

void Lexer::_trim ()
{
	for (;;)
	{
		while (!_eof() && is_space(_peek()))
			_adv();

		if (_peek() == COMMENT)
			while (!_eof() && _peek() != '\n')
				_adv();
		else
			break;
	}
}

void Lexer::_number (const std::string& str)
{
	int_t num = 0;

	for (size_t i = 0; i < str.size(); i++)
		if (!is_digit(str[i]))
		{
			auto sp = _current.span;
			sp.start += i;
			sp *= 1;

			throw sp.die("invalid character in number literal");
		}
		else
			num = (num * 10) + int_t(str[i] - '0');

	if (_peek() == '.')
	{
		_adv();

		auto start = _filepos;

		while (is_ident(_peek()))
			_adv();

		auto sp = Span(_file, start, _filepos);
		auto str = sp.data();
		auto dec = real_t(num), mag = 10.0;

		for (size_t i = 0; i < str.size(); i++)
			if (!is_digit(str[i]))
			{
				sp.start += i;
				sp *= 1;

				throw sp.die("invalid character in number literal");
			}
			else
			{
				dec += int_t(str[i] - '0') / mag;
				mag *= 10;
			}

		_current.span.end = _filepos;
		_current.tok = tNumberReal;
		_current.valueReal = dec;
	}
	else
	{
		_current.tok = tNumberInt;
		_current.valueInt = num;
	}
}
void Lexer::_ident ()
{
	while (is_ident(_peek()))
		_adv();

	_current.span.end = _filepos;

	auto str = _current.span.data();

	if (is_digit(str[0]))
		_number(str);
	else
	{
		for (auto& p : keywords)
			if (p.first == str)
			{
				_current.tok = p.second;
				return;
			}

		_current.tok = tIdent;
		_current.str = str;
	}
}
void Lexer::_string ()
{
	auto quote = _adv();

	std::ostringstream ss;

	while (_peek() != quote)
	{
		if (_eof())
			throw (_current.span * 1).die(
				"expected closing \" before <end-of-file>");

		ss << _adv();
	}
	
	_adv();

	_current.span.end = _filepos;
	_current.tok = tString;
	_current.str = ss.str();
}


Token Lexer::advance ()
{
	auto old = _current;

	_trim();

	_current.span = Span(_file, _filepos);

	if (_eof())
	{
		_current.tok = tEOF;
	}
	else if (is_control(_peek()))
	{
		_current.tok = _adv();
		_current.span *= 1;
	}
	else if (_peek() == '\"')
		_string();
	else
		_ident();

	return old;
}



Token Lexer::expect (int tok)
{
	if (_current == tok)
		return _current;
	else
	{
		std::ostringstream ss;
		ss << "expected " << Token::string(tok) << ", got "
		                  << _current.string();
		throw _current.die(ss.str());
	}
}
void Lexer::expect (const std::string& word)
{
	std::ostringstream ss;
	ss << "expected " << word << ", got " << _current.string();
	throw _current.die(ss.str());
}
void Lexer::unexpect ()
{
	std::ostringstream ss;
	ss << "unexpected token " << _current.string();
	throw _current.die(ss.str());
}
Token Lexer::eat (int tok)
{
	expect(tok);
	return advance();
}
Token Lexer::eat (int tok1, int tok2)
{
	if (_current == tok1 || _current == tok2)
		return advance();
	else
	{
		std::ostringstream ss;
		ss << "expected " << Token::string(tok1) << " or "
		                  << Token::string(tok2) << ", got "
		                  << _current.string();
		throw _current.die(ss.str());
	}
}




// corin is not bae



Span Span::operator+ (const Span& other) const
{
	if (invalid()) return other;
	if (other.invalid()) return *this;
	
	return Span(file, std::min(start, other.start),
		              std::max(end, other.end));
}


bool Token::isOperator () const
{
	return tok == tIdent && is_operator(str[0]);
}

bool Token::operator== (int otherTok) const
{
	if (otherTok == tNumber)
		return tok == tNumberReal ||
	           tok == tNumberInt;
	else
		return tok == otherTok;
}

static inline std::string quoted (const std::string& str, char q = PRETTY_QUOTE)
{
	std::ostringstream ss;
	ss << q << str << q;
	return ss.str();
}

std::string Token::string (int tok)
{
	if (tok < 256)
		return std::string(1, char(tok));
	
	switch (tok)
	{
	case tEOF:
		return "<end-of-file>";
	case tIdent:
		return "<ident>";
	case tString:
		return "<string>";
	case tNumber:
	case tNumberInt:
	case tNumberReal:
		return "<number>";

	default:
		for (auto& p : Lexer::keywords)
			if (p.second == tok)
				return quoted(p.first, PRETTY_QUOTE_KEYWORD);
		break;
	}

	return "??";
}

std::string Token::string () const
{
	std::ostringstream ss;

	switch (tok)
	{
	case tIdent:
		return quoted(str);

	case tString:
		ss << "\"";
		for (size_t i = 0; i < str.size(); i++)
			if (i >= MAX_STRING_DISPLAY_LEN)
			{
				ss << "...";
				break;
			}
			else
				ss << str[i];
		ss << "\"";
		return ss.str();

	case tNumberInt:
		ss << valueInt;
		return ss.str();

	case tNumberReal:
		ss << valueReal;
		return ss.str();

	default:
		return string(tok);
	}
}




Span::Error Span::die (const std::string& msg,
                       const std::vector<std::string>& extra) const
{
	std::ostringstream ss;

	size_t bol = 0, line = 0, len = 0;

	if (file != nullptr && !file->badfile())
	{
		for (size_t i = 0; i < start; i++)
			if (file->get(i) == '\n')
			{
				bol = i + 1;
				line++;
			}
		
		for (size_t i = bol; file->get(i) && file->get(i) != '\n'; i++)
			len++;
	}

	if (file != nullptr)
	{
		/*
			test.j:1:4: error: asdf
		*/
		ss << "\x1b[1m" << file->filename() << ":";

		if (len > 0)
			ss << (line + 1) << ":"
			   << (start - bol + 1) << ":\x1b[0m";

		ss << " ";
	}

	ss << "\x1b[1;31merror:\x1b[0m " << msg << std::endl;

	if (!extra.empty())
	{
		for (auto& x : extra)
			ss << "  " << x << std::endl;
		ss << std::endl;
	}

	if (len > 0)
	{
		char buffer[len];
		for (size_t i = 0; i < len; i++)
		{
			auto c = file->get(bol + i);
			if (is_space(c))
				buffer[i] = ' ';
			else
				buffer[i] = c;
		}

		ss << std::string(buffer, len) << std::endl;

		for (size_t i = bol; i < start; i++)
			ss << ' ';

		ss << "\x1b[36;1m";

		for (size_t i = start; i < end && i < (bol + len); i++)
			ss << '~';

		ss << "\x1b[0m" << std::endl;
	}

	return Error(ss.str());
}


