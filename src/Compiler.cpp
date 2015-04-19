#include "Compiler.h"
#include <sstream>


Compiler::Compiler ()
	: nameId(0) {}

Compiler::~Compiler ()
{
	for (auto cu : units)
		delete cu;
}

std::string Compiler::genUniqueName (const std::string& prefix)
{
	std::ostringstream ss;
	ss << prefix << "_u" << (nameId++);
	return ss.str();
}

CompileUnit* Compiler::compile (FuncOverload& overload, SigPtr sig)
{
	auto cu = new CompileUnit(this, overload, sig);
	units.push_back(cu);
	return cu;
}
CompileUnit* Compiler::bake (FuncOverload& overload,
                              SigPtr sig, TyPtr ret,
                              const std::string& intName)
{
	auto cu = new CompileUnit(this, overload, sig, ret, intName);
	cu->funcInst.cunit = cu;
	units.push_back(cu);
	return cu;
}


CompileUnit::CompileUnit (Compiler* comp, FuncOverload& over,
                            SigPtr sig, TyPtr ret,
                            const std::string& intName)
	: compiler(comp),
	  overload(over),
	  internalName(intName),
	  funcInst(this, sig, ret)
{}

CompileUnit::CompileUnit (Compiler* comp, FuncOverload& over, SigPtr sig)
	: compiler(comp),
	  overload(over),
	  internalName(comp->genUniqueName("fn")),
	  funcInst(this, sig)
{
	std::cout << "compiling '" << over.name << "'" << std::endl;

	Infer inf(this, sig);
	funcInst = inf.fn;

	// TODO: actually compile anything
}
