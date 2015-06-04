#include "stdafx.h"
#include "graf.h"

std::vector<GrafZFeromonami::vertex_descriptor> klika_plus_sasiedzi(const GrafZFeromonami& graf, std::vector<GrafZFeromonami::vertex_descriptor> klika)
{
	std::unordered_set<GrafZFeromonami::vertex_descriptor> sasiedzi;
	for(auto& el : klika)
	{
		auto sasiedztwo_wierzcholka = boost::out_edges(el, graf);
		std::transform(sasiedztwo_wierzcholka.first, sasiedztwo_wierzcholka.second, std::inserter(sasiedzi, sasiedzi.begin()), [&](GrafZFeromonami::edge_descriptor e)
		{
			return boost::target(e, graf);
		});
	}
	klika.insert(klika.end(), sasiedzi.begin(), sasiedzi.end());
	std::sort(klika.begin(), klika.end());
	return klika;
}