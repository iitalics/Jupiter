#include "Infer.h"
#include "Desugar.h"
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
				globfn,
				fnd.signature, 
				fnd.body
			});
	}
}

void GlobEnv::bake (const std::string& name,
						const std::vector<TyPtr>& argTypes,
						TyPtr ret)
{
	auto globfn = addFunc(name);
	
	Sig::ArgList args;
	for (auto t : argTypes)
		args.push_back({ "_", t });
	auto sig = std::make_shared<Sig>(args);

	globfn->overloads.push_back({
		globfn,
		sig,
		Exp::make(eInvalid)
	});
	globfn->instances.push_back({
		name, sig, ret
	});
}

std::string FuncOverload::name () const { return parent->name; }
FuncInstance FuncOverload::inst (SigPtr sig) const
{
	for (auto& inst : parent->instances)
		if (inst.signature->aEquiv(sig))
			return inst;

	std::cout << "instancing '" << parent->name << "' with: " << sig->string() << std::endl;

	Infer inf(*this, sig);
	auto& inst = inf.fn;
	parent->instances.push_back(inst);
	return inst;
}

TyPtr FuncInstance::type () const
{
	TyList list(returnType);

	const auto& args = signature->args;

	for (auto it = args.crbegin(); it != args.crend(); ++it)
		list = TyList(it->second, list);

	return Ty::makeFn(list);
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