#include "Helpers.h"
// fetches next non-comment empty line
std::string getNextValidLine(std::ifstream& in)
{
	std::string line;
	std::getline(in, line);
	while (!in.eof() && (line.empty() || line.at(0) == '#'))
	{
		getline(in, line);
	}
	if (in.eof() && (line.empty() || line.at(0) == '#'))
	{
		std::cout << "Error: unexpectedly reached end of file" << std::endl;
		abort();
	}
	return line;
}