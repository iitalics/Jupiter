#include "Compiler.h"
#include <sstream>
#include <cctype>
#include <fstream>
#include "Compiler.cc"

/*
 <infofile> := <infodata>*
 <infodata> := <nameId> | <external> | <include> | <instance>
 <nameId> := <int>
 <external> := '(' <string> ')'
	'(' [internal name] ')'

 <include> := '(' <string> <int> ')'
	'(' [internal name] [# args] ')'

 <instance> := <id> <psig> ':' <ty> <string>
	[name] [overload sig] ':' [inst type] [internal name]

*/

void Compiler::outputInfodata (std::ostream& os)
{
	for (auto& cu : _units)
		_serializeCUnit(cu);

	_ssInfodata << _nameId << std::endl;
	os << _ssInfodata.str();
}


void Compiler::_addedExternal (const std::string& name)
{
	_ssInfodata << "(\"" << name << "\")" << std::endl;
}
void Compiler::_addedInclude (const std::string& internal, size_t nargs)
{
	_ssInfodata << "(\"" << internal << "\" " << nargs << ")" << std::endl;
}


void Compiler::_serializeCUnit (CompileUnit* cunit)
{
	auto over = cunit->overload;
	auto& inst = cunit->funcInst;

	if (over->hasEnv) // don't serialize lambdas
		return;

	_ssInfodata << over->name << " "
	  << over->signature->string() << " : "
	  << Ty::makeFn(inst.signature->tyList(inst.returnType))->string()
	  << " \"" << cunit->internalName << "\"" << std::endl;
}


void Compiler::readInfodata (GlobEnv& env, const std::string& filename)
{
	Lexer lex;
	lex.openFile(filename);

	while (_parseInfodata(env, lex))
		;

	lex.expect(tEOF);

	_needsHeader = false;
}

bool Compiler::_parseInfodata (GlobEnv& env, Lexer& lex)
{
	switch (lex.current().tok)
	{
	case tLParen:
		return _parseInclude(env, lex);
	case tIdent:
		return _parseInstance(env, lex);
	case tNumberInt:
		_nameId = int(lex.advance().valueInt);
		return true;

	default:
		return false;
	}
}
bool Compiler::_parseInclude (GlobEnv& env, Lexer& lex)
{
	lex.eat(tLParen);
	auto name = lex.eat(tString).str;

	if (lex.current() == tNumberInt) // <include>
	{
		auto nargs = size_t(lex.advance().valueInt);
		_includes.insert(name);
		_addedInclude(name, nargs);
	}
	else // <external>
	{
		_externals.insert(name);
		_addedExternal(name);
	}

	lex.eat(tRParen);
	return true;
}
bool Compiler::_parseInstance (GlobEnv& env, Lexer& lex)
{
	Span instTypeSpan;

	// read syntax
	auto name = lex.eat(tIdent).str;
	auto sig = Parse::parseSigParens(lex);
	lex.eat(tColon);
	auto instType = Parse::parseType(lex, instTypeSpan);
	auto internalName = lex.eat(tString).str;

	// find overload
	auto globfn = env.addFunc(name);
	OverloadPtr overload(nullptr);
	for (auto& over : globfn->overloads)
		if (over->signature->aEquiv(sig))
			overload = over;
	if (overload == nullptr)
		throw sig->span.die("no such overload with this signature");

	// turn type into signature
	auto instSig = Sig::make({}, sig->span);
	instSig->args.reserve(sig->args.size());

	if (instType->kind != tyConcrete ||
			instType->name != "Fn")
		throw instTypeSpan.die("expected function type");

	auto tys = instType->subtypes;
	if (tys.length() != sig->args.size() + 1)
		throw instTypeSpan.die("mismatched number of arguments");

	for (size_t i = 0, len = sig->args.size(); i < len; i++, ++tys)
		instSig->args.push_back({
			sig->args[i].first,
			tys.head()
		});

	// create the fake "baked" instance
	overload->instances.push_back(
		bake(overload, instSig,
			tys.head(), internalName));

	return true;
}
