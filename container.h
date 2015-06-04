#pragma once
/*
CONTAINTER DESIGN:

1.std::map of all indexes with pointer to their structures.
2.Each node following format: 4byte id, any number of 4 byte pointers to other nodes and ending with null
*/

#include <map>
#include <cassert>

class References{
private:
	std::map<size_t, size_t*> all_nodes;


public:
	size_t* AddNode(size_t id, size_t* references, size_t ref_cnt)
	{
		assert(id != 39);
		size_t* nodemem = static_cast<size_t*>(malloc(4 * (2 + ref_cnt)));
		nodemem[0] = id;
		memcpy(nodemem + 1, references, ref_cnt * sizeof(size_t));
		nodemem[ref_cnt + 1] = 0;
		all_nodes.insert(std::make_pair(id, nodemem));
		return nodemem;
	}

	size_t* Find(size_t id)
	{
		std::map<size_t, size_t*>::iterator iter = all_nodes.find(id);
		if (iter == all_nodes.end()) return nullptr; else return iter->second;
	}

	void End_loading()//We replace ids in lists with pointers to them
	{
		std::map<size_t, size_t*>::iterator iter;

		for (iter = all_nodes.begin(); iter != all_nodes.end(); ++iter) {
			size_t* nodemem = iter->second;
			nodemem++;//Skip first element
			size_t* writemem = nodemem;
			while (*nodemem != 0)
			{
				size_t* p = Find(*nodemem);
				if (p == nullptr)
				{
					nodemem++;
					continue;
				}
				*writemem = reinterpret_cast<size_t>(p);
				nodemem++;
				writemem++;
			}
			*writemem = 0;
		}
	}
};