#include "container.h"
#include <sstream>
#include <string>
#include <fstream>
#include <cassert>
#include <vector>

References* LoadFile(std::string filename)
{
	std::ifstream infile(filename);

	std::string line;
	References* res = new References;
	while (std::getline(infile, line))
	{
		const char* pos = line.c_str();
		const char* fnd=strchr(pos, '#');
		*((char*)fnd) = 0;
		int id=strtol(pos, NULL, 10);
		std::vector<size_t> vec;
		do
		{
			const char* fnd2=strchr(fnd+1, ' ');
			if (fnd2 == NULL) break;
			*((char*)fnd2) = '\0';
			size_t i = strtol(fnd+1, NULL, 10);
			if (id!=i) vec.push_back(i);
			fnd = fnd2;
		} while (true);
		size_t i = strtol(fnd+1, NULL, 10);
		if (id != i) vec.push_back(i);
		res->AddNode(id, vec.data(), vec.size());
	}
	res->End_loading();
	return res;
}