#include "Compiler.h"
#include <sstream>
#include <iomanip>
#include <ctime>


Compiler::Compiler ()
	: nameId(0), entry(nullptr) {}

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

CompileUnit* Compiler::compile (OverloadPtr overload, SigPtr sig)
{
	auto cu = new CompileUnit(this, overload, sig);
	units.push_back(cu);
	return cu;
}
CompileUnit* Compiler::bake (OverloadPtr overload,
                              SigPtr sig, TyPtr ret,
                              const std::string& intName)
{
	auto cu = new CompileUnit(this, overload, sig, ret, intName);
	cu->funcInst.cunit = cu;
	units.push_back(cu);
	return cu;
}
void Compiler::entryPoint (CompileUnit* cunit)
{
	if (entry != nullptr)
		throw cunit->overload->signature->
				span.die("only one entry point per program allowed");

	entry = cunit;
}
void Compiler::output (std::ostream& os)
{
    auto now = std::time(nullptr);

	os << "; compiled: "
	   << std::asctime(std::localtime(&now))
	   << std::endl;

	outputRuntimeHeader(os);

	os << std::endl << std::endl;

	for (auto cu : units)
		cu->output(os);

	if (entry != nullptr)
		outputEntryPoint(os);
}
void Compiler::outputRuntimeHeader (std::ostream& os)
{
	os
		<< "; jupiter runtime header for version 0.0.1" << std::endl
		<< "declare i32 @ju_to_int (i8*)" << std::endl
		<< "declare i1  @ju_to_bool (i8*)" << std::endl
		<< "declare i8* @ju_from_int (i32)" << std::endl
		<< "declare i8* @ju_from_bool (i1)" << std::endl
		<< "declare i1  @ju_is_int (i8*)" << std::endl
		<< "declare i8* @ju_make_buf (i32, i32, i32, ...)" << std::endl
		<< "declare i8* @ju_make_str (i8*, i32)" << std::endl
		<< "declare i8* @ju_make_real (double)" << std::endl
		<< "declare i8* @ju_get (i8*, i32)" << std::endl

		;
}
void Compiler::outputEntryPoint (std::ostream& os)
{
	os << std::endl
	   << ";;;   jupiter entry point -> main()" << std::endl
	   << "define ccc i32 @main (i32 %argc, i8** %argv)" << std::endl << "{" << std::endl
	   << "call i8* @" << entry->internalName << " ()" << std::endl
	   << "ret i32 0" << std::endl
	   << "}" << std::endl;
}







CompileUnit::Env::Env (CompileUnit* cunit, EnvPtr _parent)
	: parent(_parent) {}

CompileUnit::Var CompileUnit::Env::get (const std::string& name) const
{
	for (auto& v : vars)
		if (v.name == name)
			return v;
	return parent->get(name);
}



// baked
CompileUnit::CompileUnit (Compiler* comp, OverloadPtr over,
                            SigPtr sig, TyPtr ret,
                            const std::string& intName)
	: compiler(comp),
	  overload(over),
	  internalName(intName),
	  funcInst(this, sig, ret),
	  finishedInfer(true)
{
	// declare baked signature
	ssPrefix << "declare i8* @" << intName
	         << " (";

	for (size_t i = 0, len = sig->args.size(); i < len; i++)
	{
		if (i > 0)
			ssPrefix << ", ";

		ssPrefix << "i8*";
	}
	ssPrefix << ")" << std::endl;
}

// normal
CompileUnit::CompileUnit (Compiler* comp, OverloadPtr overload, SigPtr sig)
	: compiler(comp),
	  overload(overload),
	  internalName(comp->genUniqueName("fn")),
	  funcInst(this, sig),
	  finishedInfer(false),

	  lifetime(0)
{}




CompileUnit::EnvPtr CompileUnit::makeEnv (EnvPtr parent)
{
	return std::make_shared<Env>(this, parent);
}
std::string CompileUnit::makeUnique (const std::string& str)
{
	// TODO: binary search? or too much
	bool unique;
	std::string res = str;

	for (int k = 2; ; k++)
	{
		unique = true;
		for (auto& nu : nonUnique)
			if (nu == res)
			{
				unique = false;
				break;
			}

		if (unique)
		{
			nonUnique.push_back(res);
			return "%" + res;
		}
		else
		{
			std::ostringstream ss;
			if (str[0] == '.')
				ss << str << k;
			else
				ss << str << '.' << k;
			res = ss.str();
		}
	}
}
std::string CompileUnit::getTemp ()
{
	size_t i, len;
	for (i = 0, len = tempLifetimes.size(); i < len; i++)
	{
		if (tempLifetimes[i] == -1)
			break;
	}
	
	if (i >= len)
		tempLifetimes.push_back(lifetime);
	else
		tempLifetimes[i] = lifetime;

	std::ostringstream ss;
	ss << "%.t" << i;

	if (i >= len)
		stackAlloc(ss.str());
	return ss.str();
}
void CompileUnit::pushLifetime () { lifetime++; }
void CompileUnit::popLifetime ()
{
	for (size_t i = 0, len = tempLifetimes.size(); i < len; i++)
		if (tempLifetimes[i] >= lifetime)
			tempLifetimes[i] = -1;

	lifetime--;
}






void CompileUnit::compile ()
{
	if (finishedInfer)
		return;
	
	auto& sig = funcInst.signature;

	Infer inf(this, sig);

	finishedInfer = true;

	// TODO: put in arguments 
	auto env = makeEnv();
	for (size_t i = 0, len = sig->args.size(); i < len; i++)
		env->vars.push_back({
			sig->args[i].first,
			makeUnique(sig->args[i].first),
			false });

	writePrefix(env);
	writeEnd();

	auto res = compile(overload->body, env, false);
	ssBody << "ret " << res << std::endl;
}


void CompileUnit::output (std::ostream& out)
{
	out << ssPrefix.str() << ssBody.str() << ssEnd.str();
}
void CompileUnit::writePrefix (EnvPtr env)
{
	ssPrefix << std::endl << std::endl << std::endl
	         << ";;;   " << funcInst.name << " "
	         << funcInst.type()->string() << std::endl
	         << "define i8* @"
	         << internalName << " (";

	for (size_t i = 0, len = env->vars.size(); i < len; i++)
	{
		if (i > 0)
			ssPrefix << ", ";

		ssPrefix << "i8* " << env->vars[i].internal;
	}
	ssPrefix << ") unnamed_addr" << std::endl
	         << "{" << std::endl;
}
void CompileUnit::writeEnd ()
{
	ssEnd << "}" << std::endl;
}
void CompileUnit::stackAlloc (const std::string& name)
{
	ssPrefix << name << " = alloca i8*" << std::endl;
}



std::string CompileUnit::compile (ExpPtr e, EnvPtr env, bool retain)
{
	if (retain && e->kind != eInt && e->kind != eBool && e->kind != eVar)
	{
		auto res = compile(e, env, false);
		auto temp = getTemp();
		ssBody << "store " << res << ", i8** " << temp << std::endl;
		return res;
	}

	std::ostringstream ss;

	switch (e->kind)
	{
	case eInt:
		ss << "i8* inttoptr (i32 "
		   << (1 | (e->get<int_t>() << 1))
		   << " to i8*)";
		return ss.str();

	case eBool:
		if (e->get<bool>())
			return "i8* null";
		else
			return "i8* inttoptr (i32 1 to i8*)";

	case eVar:    return compileVar(e, env);
	case eString: return compileString(e, env);
	case eReal:   return compileReal(e, env);
	case eCall:   return compileCall(e, env);
	case eLet:    return compileLet(e, env);
	case eBlock:  return compileBlock(e, env);

	default:
		throw e->span.die("cannot compile expression: " + e->string());
	}
}

static bool needs_escape (char c)
{
	static const char list[] = 
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789`-=[];',./""~!@#"
		"$%^&*()_+{}|:<>? ";

	for (size_t i = 0; i < sizeof(list) - 1; i++)
		if (list[i] == c)
			return false;
	return true;
}
static char hex_char (int k)
{
	if (k < 10)
		return char('0' + k);
	else
		return char('a' + k - 10);
}

std::string CompileUnit::compileString (ExpPtr e, EnvPtr env)
{
	auto str = e->getString();
	std::ostringstream strType;
	strType << "[" << str.size() << " x i8]";

	auto strConst = compiler->genUniqueName("string");
	auto res = makeUnique(".str");

	ssEnd << "@" << strConst
	      << " = private unnamed_addr constant "
	      << strType.str() << " c\"";

	for (size_t i = 0, len = str.size(); i < len; i++)
		if (needs_escape(str[i]))
			ssEnd << "\\"
			      << hex_char(int(str[i]) / 16)
			      << hex_char(int(str[i]) & 0xf);
		else
			ssEnd << str[i];

	ssEnd << "\"" << std::endl;

	ssBody << res << " = call i8* @ju_make_str ("
		   << "i8* getelementptr (" << strType.str() << "* @" << strConst
		   << ", i32 0, i32 0), i32 " << str.size() << ")" << std::endl;
	
	return "i8* " + res;
}

std::string CompileUnit::compileReal (ExpPtr e, EnvPtr env)
{
	auto res = makeUnique(".real");
	auto val = e->get<real_t>();

	ssBody << res << " = call i8* @ju_make_real (double 0x";

	// hex representation of double
	int64_t i64cast = 0;
	*((double*) &i64cast) = val;
	for (size_t i = 16; i-- > 0; )
		ssBody << hex_char((i64cast >> (4 * i)) & 0xf);

	ssBody << ")" << std::endl;

	return "i8* " + res;
}

std::string CompileUnit::compileVar (ExpPtr e, EnvPtr env)
{
	if (e->get<bool>()) // global
		throw e->span.die("cannot compile 'naked' globals");
	else
	{
		auto var = env->get(e->getString());

		if (var.stackAlloc)
		{
			auto val = makeUnique(".v");
			ssBody << val << " = load i8** " << var.internal << std::endl;

			return "i8* " + val;
		}
		else
			return "i8* " + var.internal;
	}
}

std::string CompileUnit::compileCall (ExpPtr e, EnvPtr env)
{
	std::ostringstream args;
	auto fn = e->subexps.front();

	if (fn->kind == eVar && fn->get<bool>()) ;
	else
		throw fn->span.die("cannot call non-global");

	pushLifetime();
	for (size_t i = 1, len = e->subexps.size(); i < len; i++)
	{
		if (i > 1)
			args << ", ";

		args << compile(e->subexps[i], env, true);
	}
	popLifetime();

	auto res = makeUnique(".r");

	ssBody << res << " = call i8* @"
	       << special[fn] << " (" << args.str() << ")" << std::endl;

	return "i8* " + res;
}

std::string CompileUnit::compileLet (ExpPtr e, EnvPtr env)
{
	auto internal = makeUnique(e->getString());
	stackAlloc(internal);

	env->vars.push_back({
		e->getString(),
		internal,
		true
	});

	auto res = compile(e->subexps.front(), env, false);
	ssBody << "store " << res << ", i8** " << internal << std::endl;

	return "i8* null"; // returns unit
}

std::string CompileUnit::compileBlock (ExpPtr e, EnvPtr env)
{
	std::string res = "i8* null";

	for (auto e2 : e->subexps)
		res = compile(e2, env, false);

	return res;
}
