#include "Infer.h"
#include "Desugar.h"
#include "Compiler.h"
#include <sstream>






// ------------------------------------- GlobEnv -------------------------------------//

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

void GlobEnv::loadToplevel (const GlobProto& proto)
{
	for (const auto& fn : proto.funcs)
		addFunc(fn.name);

	for (const auto& fn : proto.funcs)
	{
		auto globfn = getFunc(fn.name);

		Desugar des(*this);
		auto fnd = des.desugar(fn);

		globfn->overloads.push_back({
				*this,
				fnd.name,
				fnd.signature, 
				fnd.body,
				{}
			});
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

	// env.bake() is currently messed up and this
	//  routine won't work until I convert `FuncOverload`
	//  to a pointer type

	addFunc(name)->overloads.push_back({
		*this,
		name,
		sig,
		Exp::make(eInvalid),
		{}
	});
}

FuncInstance FuncOverload::inst (SigPtr sig, Compiler* compiler)
{
	for (auto& inst : instances)
		if (inst.signature->aEquiv(sig))
			return inst;

	std::cout << "instancing '" << name << "' with: " << sig->string() << std::endl;

	auto cunit = compiler->compile(*this, sig);
	instances.push_back(cunit->funcInst);
	return cunit->funcInst;
}

FuncInstance::FuncInstance (CompileUnit* _cunit, SigPtr sig, TyPtr ret)
	: name(_cunit->overload.name),
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
	: parent(_parent), _count(_c) {}

LocEnv::~LocEnv ()
{
	for (auto v : vars)
		delete v;
}

LocEnv::VarPtr LocEnv::newVar (TyPtr ty)
{
	std::ostringstream ss;
	ss << "#temp" << *_count;
	return newVar(ss.str(), ty);
}

LocEnv::VarPtr LocEnv::newVar (const std::string& name, TyPtr ty)
{
	int& r = *_count;
	auto v = new Var { name, ty, r++ };
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

LocEnv::VarPtr LocEnv::get (int idx)
{
	for (auto& v : vars)
		if (v->idx == idx)
			return v;

	if (parent == nullptr)
		return nullptr;
	else
		return parent->get(idx);
}

bool LocEnv::has (const std::string& name)
{
	for (auto& v : vars)
		if (v->name == name)
			return true;
	return false;
}




//