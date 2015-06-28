#include "Infer.h"
#include "Desugar.h"
#include "Compiler.h"
#include <sstream>






// ------------------------------------- GlobEnv -------------------------------------//

GlobEnv::GlobEnv (const GlobProto& _proto)
	: compiler(nullptr), proto(_proto)
{
	// TODO: integrate order-of-ops into jupiter syntax
	using Op = GlobEnv::OpPrecedence;
	operators.push_back(Op("==", 90, Assoc::Left));
	operators.push_back(Op("!=", 90, Assoc::Left));
	operators.push_back(Op(">=", 90, Assoc::Left));
	operators.push_back(Op("<=", 90, Assoc::Left));
	operators.push_back(Op(">",  90, Assoc::Left));
	operators.push_back(Op("<",  90, Assoc::Left));
	operators.push_back(Op("::", 80, Assoc::Right));
	operators.push_back(Op("+",  70, Assoc::Left));
	operators.push_back(Op("-",  70, Assoc::Left));
	operators.push_back(Op("*",  60, Assoc::Left));
	operators.push_back(Op("/",  60, Assoc::Left));
	operators.push_back(Op("%",  60, Assoc::Left));
	operators.push_back(Op("^",  50, Assoc::Right));

	loadToplevel();
}
void GlobEnv::loadBuiltinTypes ()
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
	for (auto& f : functions)
		delete f;
	for (auto& tyi : types)
		delete tyi;
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

void GlobEnv::loadToplevel ()
{
	for (auto& tydecl : proto.types)
		addType(TypeInfo(tydecl.name, tydecl.polytypes.size(), true));

	for (auto& tydecl : proto.types)
		generateType(tydecl);

	for (auto& fn : proto.funcs)
	{
		auto overload = Overload::make(
			*this,
			fn.name,
			fn.signature, 
			fn.body,
			fn.isPublic);

		addFunc(fn.name)->overloads.push_back(overload);
	}
}

OverloadPtr Overload::make (GlobEnv& env, const std::string& name,
                              SigPtr sig, ExpPtr body, bool isPub)
{
	return OverloadPtr(new Overload { env, name, sig, body, {}, false, isPub, false });
}

FuncInstance Overload::inst (OverloadPtr over, SigPtr sig, Compiler* origin)
{
	if (origin != over->env.compiler)
	{
		auto inst = Overload::inst(over, sig, over->env.compiler);
		origin->addExternal(inst.cunit);
		return inst;
	}


	for (auto cu : over->instances)
		if (cu->funcInst.signature->aEquiv(sig))
			return cu->funcInst;

	auto cunit = over->env.compiler->compile(over, sig);
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
	auto v = new Var { name, ty, false };
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