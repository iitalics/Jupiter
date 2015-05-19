#include "Infer.h"
#include "Desugar.h"
#include "Compiler.h"
#include <sstream>






// ------------------------------------- GlobEnv -------------------------------------//

GlobEnv::GlobEnv ()
{
	// create some primitive types that are required for basic
	//  operations to work
	addType(TypeInfo("Fn", TypeInfo::OneOrMore));
	addType(TypeInfo("Tuple", TypeInfo::ZeroOrMore));
	addType(TypeInfo("Int"));
	addType(TypeInfo("Bool"));
	addType(TypeInfo("Char"));
	addType(TypeInfo("Real"));
	addType(TypeInfo("Str"));
}

GlobEnv::~GlobEnv ()
{
	for (auto f : functions)
		delete f;
}

GlobEnv::OpPrecedence GlobEnv::getPrecedence (const std::string& operName) const
{
	for (auto& op : operators)
		if (std::get<0>(op) == operName)
			return op;

	return OpPrecedence(std::string(operName), 0, Assoc::Left);
}

GlobFuncPtr GlobEnv::getFunc (const std::string& name) const
{
	for (auto& f : functions)
		if (f->name == name)
			return f;

	return nullptr;
}

GlobFuncPtr GlobEnv::addFunc (const std::string& name)
{
	auto f = getFunc(name);
	if (f == nullptr)
	{
		f = new GlobFunc(*this, name);
		functions.push_back(f);
	}

	return f;
}

void GlobEnv::addType (const TypeInfo& tyi)
{
	types.push_back(new TypeInfo(tyi));
}
TypeInfo* GlobEnv::getType (const std::string& name) const
{
	for (auto& tyi : types)
		if (tyi->name == name)
			return tyi;
	return nullptr;
}

void GlobEnv::loadToplevel (const std::string& filename)
{
	Lexer lex;
	lex.openFile(filename);
	auto toplevel = Parse::parseToplevel(lex);
	lex.expect(tEOF);
	
	loadToplevel(toplevel);
}

void GlobEnv::loadToplevel (GlobProto& proto)
{
	// each loop is two-pass so thatt declaration order doesn't matter
	for (auto& tydecl : proto.types)
	{
		// add type
		if (getType(tydecl.name) != nullptr)
			throw tydecl.span.die("redeclaration of existing type");

		addType(TypeInfo(tydecl.name, tydecl.polytypes.size(), true));
	}

	for (auto& tydecl : proto.types)
	{
		// generate type
		generateType(tydecl, proto);
	}


	for (auto& fn : proto.funcs)
	{
		// add function
		addFunc(fn.name);
	}

	for (auto& fn : proto.funcs)
	{
		// generate function
		auto globfn = getFunc(fn.name);

		Desugar des(*this);
		auto fnd = des.desugar(fn);

		auto overload = Overload::make(
			*this,
			fnd.name,
			fnd.signature, 
			fnd.body);

		globfn->overloads.push_back(overload);
	}
}

void GlobEnv::bake (Compiler* comp, const std::string& intName,
                        const std::string& name,
                        const std::vector<TyPtr>& args,
                        TyPtr ret)
{
	auto sig = Sig::make();
	for (const auto& ty : args)
		sig->args.push_back({ "_", ty });

	auto overload = Overload::make(*this, name, sig, Exp::make(eInvalid));
	auto cu = comp->bake(overload, sig, ret, intName);

	addFunc(name)->overloads.push_back(overload);
	overload->instances.push_back(cu);
}

OverloadPtr Overload::make (GlobEnv& env, const std::string& name,
                              SigPtr sig, ExpPtr body)
{
	return OverloadPtr(new Overload { env, name, sig, body, {} });
}

FuncInstance Overload::inst (OverloadPtr over, SigPtr sig, Compiler* compiler)
{
	for (auto cu : over->instances)
		if (cu->funcInst.signature->aEquiv(sig))
			return cu->funcInst;

//	std::cerr << "instancing '" << over->name << "' with: " << sig->string() << std::endl;
	auto cunit = compiler->compile(over, sig);
	over->instances.push_back(cunit);
	cunit->compile();
	return cunit->funcInst;
}

FuncInstance::FuncInstance (CompileUnit* _cunit, SigPtr sig, TyPtr ret)
	: name(_cunit->overload->name),
	  signature(sig),
	  returnType(ret),
	  cunit(_cunit) {}

TyPtr FuncInstance::type () const
{
	return Ty::makeFn(signature->tyList(returnType));
}



// ------------------------------------- LocEnv -------------------------------------//

LocEnvPtr LocEnv::make ()
{
	return LocEnvPtr(new LocEnv(
				nullptr,
				std::make_shared<int>(0)));
}

LocEnvPtr LocEnv::make (LocEnvPtr parent)
{
	return LocEnvPtr(new LocEnv(parent, parent->_count));
}

LocEnv::LocEnv (LocEnvPtr _parent, Counter _c)
	: parent(_parent), uses(nullptr), _count(_c) {}

LocEnv::~LocEnv ()
{
	for (auto v : vars)
		delete v;
}

LocEnv::VarPtr LocEnv::newVar (TyPtr ty)
{
	std::ostringstream ss;
	ss << "#t" << (*_count)++;
	return newVar(ss.str(), ty);
}

LocEnv::VarPtr LocEnv::newVar (const std::string& name, TyPtr ty)
{
	auto v = new Var { name, ty };
	vars.push_back(v);
	return v;
}

LocEnv::VarPtr LocEnv::get (const std::string& name)
{
	for (auto& v : vars)
		if (v->name == name)
			return v;

	if (parent == nullptr)
		return nullptr;
	else
		return parent->get(name);
}

bool LocEnv::has (const std::string& name)
{
	for (auto& v : vars)
		if (v->name == name)
			return true;
	return false;
}




//