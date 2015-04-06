#include "Env.h"
#include <sstream>













// ------------------------------------ LocEnv --------------------------------------//

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