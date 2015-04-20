#include "Compiler.h"
#include <sstream>
#include <iomanip>
#include <ctime>


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
void Compiler::output (std::ostream& os)
{
    auto now = std::time(nullptr);

	os << "; compiled: "
	   << std::asctime(std::localtime(&now))
	   << std::endl << std::endl;
	for (auto cu : units)
		cu->output(os);
}






CompileUnit::CompileUnit (Compiler* comp, OverloadPtr over,
                            SigPtr sig, TyPtr ret,
                            const std::string& intName)
	: compiler(comp),
	  overload(over),
	  internalName(intName),
	  funcInst(this, sig, ret)
{}

CompileUnit::CompileUnit (Compiler* comp, OverloadPtr overload, SigPtr sig)
	: compiler(comp),
	  overload(overload),
	  internalName(comp->genUniqueName("fn")),
	  funcInst(this, sig),

	  ssPrefix(), ssBody(), ssEnd(),
	  regs(0)
{
	Infer inf(this, sig);
	funcInst = inf.fn;

	writePrefix();
	writeEnd();

	auto env = makeEnv();
	compileOp(compile(overload->body, env));
	ssBody << "ret i8* " << tempString() << std::endl;
}


// Env
CompileUnit::Env::Env (CompileUnit* cunit, EnvPtr _parent)
	: parent(_parent),
	  life(cunit) {}
CompileUnit::Var CompileUnit::Env::get (const std::string& name) const
{
	for (auto& v : vars)
		if (v.name == name)
			return v;

	return parent->get(name);
}
CompileUnit::EnvPtr CompileUnit::makeEnv (EnvPtr parent)
{
	return std::make_shared<Env>(this, parent);
}

// Lifetime
CompileUnit::Lifetime::Lifetime (CompileUnit* _cunit)
	: cunit(_cunit) {}
CompileUnit::Lifetime::~Lifetime ()
{ relinquish(); }

void CompileUnit::Lifetime::relinquish ()
{
	for (auto idx : regs)
		cunit->regs[idx] = nullptr;
}
int CompileUnit::Lifetime::claim (int idx)
{
	cunit->regs[idx] = this;
	return idx;
}
int CompileUnit::findRegister (Lifetime* life)
{
	// look for available registers
	int idx, len = regs.size();
	for (idx = 0; idx < len; idx++)
		if (regs[idx] == nullptr)
			return life->claim(idx);

	// none found, add one
	regs.push_back(nullptr);


	// declare new register
//	ssPrefix << regString(len) << " = alloca i8*" << std::endl;

	return life->claim(len);
}


/*   Compiling   */


void CompileUnit::writePrefix ()
{
	ssPrefix << ";;; " << overload->name << " : "
	         << funcInst.type()->string() << std::endl
	         << "define i8* @" << internalName << " () {" << std::endl;
}
void CompileUnit::writeEnd ()
{
	ssEnd << "}\n";
}
void CompileUnit::output (std::ostream& out)
{
	out << ssPrefix.str() << ssBody.str() << ssEnd.str();
}

// \x25 = '%'
std::string CompileUnit::regString (int idx) const
{
	std::ostringstream ss; ss << '%' << "s" << idx;
	return ss.str();
}
std::string CompileUnit::argString (int idx) const
{
	std::ostringstream ss; ss << '%' << "a" << idx;
	return ss.str();
}
std::string CompileUnit::tempString (int id) const
{
	std::ostringstream ss; ss << '%' << "t" << id;
	return ss.str();
}

CompileUnit::Operand CompileUnit::compile (ExpPtr e, EnvPtr env)
{
	switch (e->kind)
	{
	case eInt:
	case eBool:
	case eCall:
		return { opLit, 0, e };

	case eVar:
		if (e->get<bool>())
			break; // how to do globals?
		else
		{
			auto idx = env->get(e->getString()).idx;

			if (idx < 0) // argument
				return { opArg, -idx, e };
			else
				return { opVar, idx, e };
		}

	case eBlock:
		{
			auto env2 = makeEnv(env);
			Operand op { opLit, 0, e };

			for (size_t i = 0, len = e->subexps.size(); i < len; i++)
			{
				op = compile(e->subexps[i], env2);

				// reduce all but last op
				if (i < len - 1)
					compileOp(op);
			}

			return op;
		}

	default: break;
	}

	static std::vector<std::string> kinds = {
		"invalid", "int", "real",
		"string", "bool", "var",
		"tuple", "call", "infix",
		"cond", "lambda", "block",
		"let",
	};

	std::ostringstream ss;
	ss << "not sure how to compile '" << kinds[int(e->kind)] << "'";
	throw e->span.die(ss.str());
}

void CompileUnit::compileOp (const Operand& op, int tempId)
{
	ssBody << tempString(tempId) << " = ";
	switch (op.kind)
	{
	case opLit:
		switch (op.src->kind)
		{
		case eInt:
			ssBody << "inttoptr i32 "
			       << ((op.src->get<int_t>() << 1) | 1) << " to i8*"
			       << "  ; int";
			break;

		case eBool:
			ssBody << "inttoptr i32 "
			       << ((op.src->get<bool>() ? 3 : 1)) << " to i8*"
			       << "  ; boolean";
			break;

		default:
			ssBody << "bitcast i8* null to i8*  ; ??";
			break;
		}
		break;

	case opVar:
		ssBody << "load i8*, i8** "
		       << regString(op.idx);
		break;

	case opArg:
		ssBody << "i8* " << argString(op.idx);
		break;

	default:
		ssBody << "i8* null";
		break;
	}

	ssBody << std::endl;
}