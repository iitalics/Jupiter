#include "Compiler.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cctype>


Compiler::Compiler ()
	: nameId(0), entry(nullptr) {}

Compiler::~Compiler ()
{
	for (auto cu : units)
		delete cu;
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

std::string Compiler::mangle (const std::string& ident)
{
	std::ostringstream ss;

	for (auto c : ident)
		if (c == '_') ss << "__";
		else if (c == '-') ss << "_mn";
		else if (c == '+') ss << "_pl";
		else if (c == '*') ss << "_st";
		else if (c == '/') ss << "_sl";
		else if (c == '<') ss << "_ls";
		else if (c == '>') ss << "_gr";
		else if (c == '=') ss << "_eq";
		else if (c == '?') ss << "_is";
		else if (c == '#') ss << "_J";
		else if (!(isalpha(c) || isdigit(c)))
			ss << "_";
		else
			ss << c;

	return ss.str();
}

std::string Compiler::genUniqueName (const std::string& prefix)
{
	std::ostringstream ss;
	ss << prefix << "_" << (nameId++);
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
		<< "; jupiter runtime header for version 0.0.4" << std::endl
		<< "declare void @ju_init ()" << std::endl
		<< "declare void @ju_destroy ()" << std::endl
		<< "declare void @juGC_root (i8**)" << std::endl
		<< "declare void @juGC_unroot (i32)" << std::endl
		<< "declare void @juGC_store (i8**, i8*)" << std::endl
		<< "declare i32 @ju_to_int (i8*)" << std::endl
		<< "declare i1  @ju_to_bool (i8*)" << std::endl
		<< "declare i8* @ju_from_int (i32)" << std::endl
		<< "declare i8* @ju_from_bool (i1)" << std::endl
		<< "declare i1  @ju_is_int (i8*)" << std::endl
		<< "declare i8* @ju_make_buf (i32, i32, i32, ...)" << std::endl
		<< "declare i8* @ju_make_str (i8*, i32)" << std::endl
		<< "declare i8* @ju_make_real (double)" << std::endl
		<< "declare i8* @ju_get (i8*, i32)" << std::endl
		<< "declare i8* @ju_safe_get (i8*, i8*, i32, i32)" << std::endl
		<< "declare i32 @ju_get_tag (i8*)" << std::endl

		<< std::endl;

	for (auto& name : declares)
		os << "declare i8* @" << name << " (...)" << std::endl;
}
void Compiler::outputEntryPoint (std::ostream& os)
{
	os << std::endl
	   << ";;;   < jupiter entry point >" << std::endl
	   << "define ccc i32 @main (i32 %argc, i8** %argv)" << std::endl << "{" << std::endl
	   << "call void @ju_init ()" << std::endl
	   << "call i8* @" << entry->internalName << " ()" << std::endl
	   << "call void @ju_destroy ()" << std::endl
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
	  internalName(comp->genUniqueName("fn_" +
	  					comp->mangle(overload->name))),
	  funcInst(this, sig),
	  finishedInfer(false),

	  lifetime(0),
	  nroots(0)
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

	ssBody << std::endl;

	auto res = compile(overload->body, env, false);

	if (nroots > 0)
		ssBody << "call void @juGC_unroot (i32 "
		       << nroots << ")" << std::endl;
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
	ssPrefix << name << " = alloca i8*" << std::endl
	         << "call void @juGC_root (i8** " << name << ")" << std::endl;

	nroots++;
}
void CompileUnit::stackStore (const std::string& name, 
                                const std::string& value)
{
	ssBody << "call void @juGC_store (i8** " << name << ", " << value << ")" << std::endl;
}
bool CompileUnit::needsRetain (ExpPtr exp)
{
	switch (exp->kind)
	{
	case eInt:
	case eBool:
	case eVar:
	case eCond:
		return false;

	default: return true;
	}
}




std::string CompileUnit::compile (ExpPtr e, EnvPtr env, bool retain)
{
	if (retain && needsRetain(e))
	{
		auto res = compile(e, env, false);
		auto temp = getTemp();
		stackStore(temp, res);
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
			return "i8* inttoptr (i32 1 to i8*)";
		else
			return "i8* null";

	case eVar:    return compileVar(e, env);
	case eString: return compileString(e, env);
	case eReal:   return compileReal(e, env);
	case eCall:   return compileCall(e, env);
	case eLet:    return compileLet(e, env);
	case eBlock:  return compileBlock(e, env);
	case eCond:   return compileCond(e, env);
	case eiGet:   return compileiGet(e, env);
	case eiTag:   return compileiTag(e, env);
	case eTuple:
		return "i8* null";

	case eiMake:
	case eiCall:
		throw e->span.die("expression must be called as function"); 

	default:
		throw e->span.die("cannot compile expression: " + e->string());
	}
}

static char hex_char (int k)
{
	if (k < 10)
		return char('0' + k);
	else
		return char('a' + k - 10);
}

std::string CompileUnit::makeGlobalString (const std::string& str, bool nullterm)
{
	auto strConst = compiler->genUniqueName("string");

	std::ostringstream strType;
	strType << "[" << (str.size() + (nullterm ? 1 : 0)) << " x i8]";

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

	if (nullterm)
		ssEnd << "\\00";

	ssEnd << "\"" << std::endl;

	std::ostringstream ss;
	ss << "i8* getelementptr (" << strType.str() << "* @" << strConst
	   << ", i32 0, i32 0)";
	
	return ss.str();
}

std::string CompileUnit::compileString (ExpPtr e, EnvPtr env)
{
	auto str = makeGlobalString(e->getString());
	auto res = makeUnique(".str");

	ssBody << res << " = call i8* @ju_make_str ("
	       << str << ", i32 " << e->getString().size()
	       << ")" << std::endl;
	
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

	if (fn->kind == eVar && fn->get<bool>())
	{
		args << "call i8* @" << special[fn] << " (";
	}
	else if (fn->kind == eiMake)
	{
		auto tag = GlobEnv::getTag(fn->getString());

		args << "call i8* (i32, i32, i32, ...)* @ju_make_buf (i32 "
		     << tag << ", i32 0, i32 "
		     << (e->subexps.size() - 1);

		if (e->subexps.size() > 1)
			args << ", ";
	}
	else if (fn->kind == eiCall)
	{
		args << "call i8* bitcast (i8* (...)* @" << fn->getString()
			 << " to i8* (";

		for (size_t i = 1, len = e->subexps.size(); i < len; i++)
		{
			if (i > 1)
				args << ", ";

			args << "i8*";
		}

		args << ")*) (";

		compiler->declares.insert(fn->getString());
	}
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
	ssBody << res << " = " << args.str() << ")" << std::endl;

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
	stackStore(internal, res);

	return "i8* null"; // returns unit
}

std::string CompileUnit::compileBlock (ExpPtr e, EnvPtr env)
{
	std::string res = "i8* null";

	for (auto e2 : e->subexps)
		res = compile(e2, env, false);

	return res;
}

std::string CompileUnit::compileCond (ExpPtr e, EnvPtr env)
{
	/*
		br <cond>, <then>, <else>
	*/
	auto temp = getTemp();
	auto cmp = makeUnique(".cond");
	auto lthen = makeUnique("Lthen");
	auto lelse = makeUnique("Lelse");
	auto lend = makeUnique("Lend");

	auto cond = compile(e->subexps[0], env, false);
	ssBody << cmp << " = icmp eq "
	       << cond << ", null" << std::endl
	       << "br i1 " << cmp
	       << ", label " << lelse
	       << ", label " << lthen << std::endl
	       << std::endl
	       << lthen.substr(1) << ":" << std::endl;

	{
		stackStore(temp, compile(e->subexps[1], env, false));
		ssBody << "br label " << lend << std::endl
		       << std::endl
		       << lelse.substr(1) << ":" << std::endl;
	}

	{
		stackStore(temp, compile(e->subexps[2], env, false));
		ssBody << "br label " << lend << std::endl
		       << std::endl
		       << lend.substr(1) << ":" << std::endl;
	}

	auto res = makeUnique(".r");
	ssBody << res << " = load i8** " << temp << std::endl;

	return "i8* " + res;
}

std::string CompileUnit::compileiGet (ExpPtr e, EnvPtr env)
{
	auto res = makeUnique(".get");
	auto inp = compile(e->subexps[0], env, true);

	if (e->getString() == "")
		ssBody << res << " = call i8* @ju_get (" << inp
		       << ", i32 " << e->get<int_t>() << ")" << std::endl;
	else
	{
		auto tag = GlobEnv::getTag(e->getString());
		auto str = makeGlobalString(e->getString(), true);

		ssBody << res << " = call i8* @ju_safe_get ("
			   << inp << ", " << str << ", i32 " << tag
			   << ", i32 " << e->get<int_t>() << ")" << std::endl;
	}
	
	return "i8* " + res;
}

std::string CompileUnit::compileiTag (ExpPtr e, EnvPtr env)
{
	auto tag = makeUnique(".tag");
	auto cmp = makeUnique(".cmp");
	auto res = makeUnique(".r");
	auto inp = compile(e->subexps[0], env, true);

	ssBody << tag << " = call i32 @ju_get_tag (" << inp << ")" << std::endl
	       << cmp << " = icmp eq i32 " << tag << ", "
	       << GlobEnv::getTag(e->getString()) << std::endl
	       << res << " = inttoptr i1 " << cmp << " to i8* " << std::endl;

	return "i8* " + res;
}
