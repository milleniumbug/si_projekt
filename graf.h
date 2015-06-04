#ifndef GRAF_H
#define GRAF_H

#include <boost/graph/adjacency_list.hpp>
#include "mapa_kolorow.h"

struct WlasnosciKrawedzi
{
	std::atomic<double> feromony;

	// Stupid workaround for lack support for noncopyable properties in Boost.Graph
	// WARNING, ATTENTION, ALERT, ACHTUNG, UWAGA: NON-ATOMIC
	WlasnosciKrawedzi(const WlasnosciKrawedzi& other)
	{
		this->feromony.store(other.feromony.load());
	}

	// WARNING, ATTENTION, ALERT, ACHTUNG, UWAGA: NON-ATOMIC
	WlasnosciKrawedzi& operator=(const WlasnosciKrawedzi& other)
	{
		this->feromony.store(other.feromony.load());
		return *this;
	}

	WlasnosciKrawedzi() :
		feromony(0.0)
	{

	}
};

typedef boost::adjacency_list<boost::vecS,
	boost::vecS,
	boost::undirectedS,
	boost::no_property,
	WlasnosciKrawedzi> GrafZFeromonami;

typedef std::map<GrafZFeromonami::vertex_descriptor, std::string> NameMap;

template<typename MutableGraph, typename RandomNumberGenerator>
void wygeneruj_graf_z_klika(MutableGraph& graf, int ilosc_wierzcholkow, int ilosc_krawedzi_hint, int ilosc_wierzcholkow_w_klice, RandomNumberGenerator& rng)
{
	boost::generate_random_graph(graf, ilosc_wierzcholkow, ilosc_krawedzi_hint, rng);
	std::unordered_set<typename boost::graph_traits<MutableGraph>::vertex_descriptor> klika;
	while(klika.size() < ilosc_wierzcholkow_w_klice)
	{
		klika.insert(boost::random_vertex(graf, rng));
	}
	for(auto& left : klika)
	{
		boost::remove_out_edge_if(left, [&](typename boost::graph_traits<MutableGraph>::edge_descriptor edge)
		{
			return klika.find(boost::target(edge, graf)) != klika.end();
		}, graf);
		for(auto& right : klika)
		{
			if(left != right)
			{
				boost::add_edge(left, right, graf);
			}
		}
	}
}

template<typename VertexIterator, typename EdgeIterator>
void serializuj_do_dot(std::ostream& os, const GrafZFeromonami& graf, VertexIterator vb, VertexIterator ve, EdgeIterator eb, EdgeIterator ee, const double max = -1, const double threshold = -1, NameMap namemap = NameMap())
{
	os << "strict graph {\n";
	std::for_each(vb, ve, [&](GrafZFeromonami::vertex_descriptor vertex)
	{
		if(!namemap.empty())
		{
			os << vertex << " [ label = " << boost::escape_dot_string(namemap[vertex]) << "]\n";
		}
		else os << vertex << "\n";
	});
	std::for_each(eb, ee, [&](GrafZFeromonami::edge_descriptor edge)
	{
		const double fer = graf[edge].feromony.load();
		std::string kolor_property = (max > 0) ? "color = \"" + as_hex(daj_kolor(0, max, threshold, fer)) + "\";" : "";
		os << boost::source(edge, graf) << " -- " << boost::target(edge, graf) << " [ label = " << std::fixed << boost::escape_dot_string(fer) << ";  " << kolor_property << " ]" << "\n";
	});
	os << "}\n";
}

void serializuj_do_dot(std::ostream& os, const GrafZFeromonami& graf);

#endif