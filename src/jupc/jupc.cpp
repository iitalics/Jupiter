#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
# define WEXITSTATUS(x) x
#else
# include <sys/wait.h>
#endif

// Let the spahgetti C begin

#define VERSION   "Jupiter Compiler toolchain (jupc) 0.1.0"
#define COPYRIGHT "Copyright (C) 2015 iitalics"

enum Op
{
	Output = 0, Mode, Compiler, Assembler, Linker, Runtime, NumOps
};

enum CompileMode
{
	LLVM = 0, Asm = 1, Bin = 2,
};

struct Result
{
	std::vector<std::string> files;
	std::string output, compiler, assembler, linker, runtime;
	int mode;
};


CompileMode getMode (const std::string& str);
bool parse (Result& result, size_t argc, char** argv);
bool doSomething (char flag);
void usage ();
void version ();
void go (Result& result);



static inline std::runtime_error die (const std::string& str)
{
	return std::runtime_error(str);
}
static inline std::runtime_error die
				(const std::string& a,
				 const std::string& b)
{
	std::ostringstream ss;
	ss << a << b;
	return std::runtime_error(ss.str());
}

static inline std::runtime_error die
				(const std::string& a,
				 const std::string& b,
				 const std::string& c)
{
	std::ostringstream ss;
	ss << a << b << c;
	return std::runtime_error(ss.str());
}




static char flagNames[] = "ocCALR";
static const char* defaults[] = 
{
#ifdef _WIN32
	"a.exe",
	"bin",
	"jup.exe",
#else
	"a.out",
	"bin",
	"./jup",
#endif
	"llc",
	"clang",
	"lib/runtime.a"
};



int main (int argc, char** argv)
{
	try
	{
		Result result;
		if (parse(result, argc, argv))
			go(result);
		else
			return 0;
	}
	catch (std::exception& e)
	{
		std::cerr << "jupc error: " << e.what() << std::endl;
		return -1;
	}
}




void usage ()
{
	printf(
"Usage: jupc [options] files...\n"
"Options:\n"
"    -h, --help        Show this help message\n"
"    --version         Show version information\n"
"    -o <file>         Output to file                default: '%s'\n"
"    -c <mode>         Compile mode (see \"Modes\")    default: '%s'\n"
"    -C <program>      Jupiter compiler program      default: '%s'\n"
"    -A <program>      LLVM assembler program        default: '%s'\n"
"    -L <program>      Linker program                default: '%s'\n"
"    -R <path>         Location of runtime archive   default: '%s'\n"
"\n"
"Modes:\n"
"    'bin'             Compiles to executable binary\n"
"    'asm'             Compiles to assembler file (.s)\n"
"    'llvm'            Compiles to LLVM IR (.ll)\n",
		defaults[0],
		defaults[1],
		defaults[2],
		defaults[3],
		defaults[4],
		defaults[5]);
}
void version ()
{
	printf("%s\n%s\n", VERSION, COPYRIGHT);
}


CompileMode getMode (const std::string& str)
{
	if (str == "bin") return Bin;
	if (str == "asm") return Asm;
	if (str == "llvm") return LLVM;

	throw die("invalid compile mode '", str, "'");
}

bool parse (Result& result, size_t argc, char** argv)
{
	std::string values[NumOps];
	size_t i, j;

	for (i = 0; i < NumOps; i++)
		values[i] = std::string(defaults[i]);

	for (i = 1; i < argc; i++)
		if (argv[i][0] == '-')
		{
			if (std::string(argv[i]) == "--help")
			{
				usage();
				return false;
			}
			else if (std::string(argv[i]) == "--version")
			{
				version();
				return false;
			}

			char flag = argv[i][1];
			for (j = 0; j < NumOps; j++)
				if (flag == flagNames[j])
					break;

			if (j >= NumOps)
			{
				if (doSomething(flag))
					return false;
				else
					throw die("invalid argument ", std::string(argv[i]));
			}

			if (argv[i][2] == '\0')
			{
				if (i == argc - 1)
					throw die("missing argument to flag");

				values[j] = std::string(argv[++i]);
			}
			else
				values[j] = std::string(argv[i] + 2);
		}
		else
			result.files.push_back(std::string(argv[i]));

	result.output    = values[Op::Output];
	result.compiler  = values[Op::Compiler];
	result.assembler = values[Op::Assembler];
	result.linker    = values[Op::Linker];
	result.runtime   = values[Op::Runtime];
	result.mode      = getMode(values[Op::Mode]);

	if (result.files.empty())
		throw die("no input files");
	return true;
}

bool doSomething (char flag)
{
	if (flag == 'h')
	{
		usage();
		return true;
	}
	else if (flag == 'v')
	{
		version();
		return true;
	}
	else
		return false;
}





// C++ voodoo magics
struct escape
{
	std::string _str;

	inline escape (const std::string& str)
		: _str(str) {}
};

std::ostream& operator<< (std::ostream& ss, const escape& esc)
{
	for (auto c : esc._str)
		if (c == '\\' || c == '\"' || c == '\'' || c == ' ')
			ss << '\\' << c;
		else
			ss << c;
	return ss;
}



static std::string temporary (const std::string& ext = "")
{
	char buf[L_tmpnam];
	tmpnam(buf);
	if (buf[0] == '\\') buf[0] = 't';
	return std::string(buf) + ext;
}

static int shell_exec (const std::string& cmd)
{
	int ret = system(cmd.c_str());
	return WEXITSTATUS(ret);
}

static bool compileLLVM (Result& data, const std::string& outFile)
{
	std::ostringstream ss;
	ss << escape(data.compiler);

	for (auto& file : data.files)
		ss << " " << escape(file);

	ss << " > " << escape(outFile);
	return shell_exec(ss.str()) == 0;
}

static bool compileAsm (Result& data, const std::string& inFile, const std::string& outFile)
{
	std::ostringstream ss;
	ss << escape(data.assembler)
	   << " -o " << escape(outFile) << " "
	   << escape(inFile);

	return shell_exec(ss.str()) == 0;
}

static bool compileLink (Result& data, const std::string& inFile, const std::string& outFile)
{
	std::ostringstream ss;
	ss << escape(data.linker)
	   << " -o " << escape(outFile) << " "
	   << escape(inFile) << " "
	   << escape(data.runtime);
	// TODO: linker flags?
	return shell_exec(ss.str()) == 0;
}


void go (Result& res)
{
	std::vector<std::string> temps;
	std::string templlvm, tempasm, tempbin, last;

	temps.push_back(last = templlvm = temporary(".ll"));
	if (!compileLLVM(res, templlvm))
		goto fail;

	if (res.mode >= CompileMode::Asm)
	{
		temps.push_back(last = tempasm = temporary(".s"));
		if (!compileAsm(res, templlvm, tempasm))
			goto fail;
	}

	if (res.mode >= CompileMode::Bin)
	{
		temps.push_back(last = tempbin = temporary());
		if (!compileLink(res, tempasm, tempbin))
			goto fail;
	}

	if (rename(last.c_str(), res.output.c_str()) != 0)
		goto fail;

	for (auto s : temps)
		remove(s.c_str());
	return;
fail:
	for (auto s : temps)
		remove(s.c_str());
	throw die("compile failed");
}


