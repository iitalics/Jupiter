
#define ENV_VAR   "%.env"
#define JUP_CCONV "fastcc"

static std::string joinCommas (int n, const std::string& str)
{
	std::ostringstream ss;
	for (int i = 0; i < n; i++)
	{
		if (i > 0)
			ss << ", ";
		ss << str;
	}
	return ss.str();
}