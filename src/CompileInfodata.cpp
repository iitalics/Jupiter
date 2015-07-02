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
	_ssPrefix << "declare " JUP_CCONV " i8* @" << internal
	          << " (" << joinCommas(nargs, "i8*") << ")" << std::endl;
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


void Compiler::readInfodata (const std::string& filename)
{
	// TODO: use lexer and parser to read infodata file according to rules at top
}
