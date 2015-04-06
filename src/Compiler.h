#pragma once
#include "Jupiter.h"
#include "Ast.h"

class GlobEnv;
using GlobEnvPtr = std::shared_ptr<GlobEnv>;



class Compiler
{
public:
	std::ostream& output ();
};