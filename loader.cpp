#include "stdafx.h"
#include "container.h"
#include <string>
#include <fstream>
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
		*const_cast<char*>(fnd) = 0;
		int id=strtol(pos, nullptr, 10);
		std::vector<size_t> vec;
		do
		{
			const char* fnd2=strchr(fnd+1, ' ');
			if(fnd2 == nullptr) break;
			*const_cast<char*>(fnd2) = '\0';
			size_t i = strtol(fnd + 1, nullptr, 10);
			if (id!=i) vec.push_back(i);
			fnd = fnd2;
		} while (true);
		size_t i = strtol(fnd + 1, nullptr, 10);
		if (id != i) vec.push_back(i);
		res->AddNode(id, vec.data(), vec.size());
	}
	res->End_loading();
	return res;
}