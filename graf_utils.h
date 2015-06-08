#ifndef GRAF_UTILS_H
#define GRAF_UTILS_H

#include "graf.h"

template <typename OutputStream>
struct clique_printer
{
	clique_printer(OutputStream& stream, int minsize_)
		: output(stream), minsize(minsize_)
	{ }

	template <typename Clique, typename Graph>
	void clique(const Clique& c, const Graph& g)
	{
		if(c.size() < minsize) return;
		// Iterate over the clique and print each vertex within it.
		typename Clique::const_iterator i, end = c.end();
		for(i = c.begin(); i != end; ++i) {
			output << *i << " ";
		}
		output << std::endl;
	}
	OutputStream& output;
	int minsize;
};

template<typename UnderlyingCliquePrinter>
struct clique_printer_as_comment
{
	UnderlyingCliquePrinter printer;

	explicit clique_printer_as_comment(UnderlyingCliquePrinter unprinter)
		: printer(unprinter)
	{ }

	template <typename Clique, typename Graph>
	void clique(const Clique& c, const Graph& g)
	{
		if(c.size() < printer.minsize) return;
		// Iterate over the clique and print each vertex within it.
		printer.output << "// ";
		printer.clique(c, g);
	}
};

template<typename OutputStream, typename NameMap>
struct clique_printer_with_name_map
{
	clique_printer_with_name_map(OutputStream& stream, int minsize_, NameMap namemap_)
		: namemap(namemap_), output(stream), minsize(minsize_)
	{ }

	template <typename Clique, typename Graph>
	void clique(const Clique& kl, const Graph& g)
	{
		if(kl.size() < minsize) return;
		// Iterate over the clique and print each vertex within it.
		auto kt = kl.cbegin();
		while(kt != kl.cend())
		{
			output << namemap[*kt] << " ";
			++kt;
		}
		output << std::endl;

		Clique k2 = kl;//klika_plus_sasiedzi(g, kl);
		if (kl.size() < 100)
		{
			std::vector<GrafZFeromonami::edge_descriptor> edges = wierzcholki_kliki(g, k2);
			serializuj_do_dot(output, g, k2.begin(), k2.end(), edges.begin(), edges.end(),-1.0, -1.0, namemap);
		}
	}
	NameMap namemap;
	OutputStream& output;
	int minsize;
};

struct serializator_klik
{
	serializator_klik(std::ostream& output, int minimalny_rozmiar) :
		output(output),
		minimalny_rozmiar(minimalny_rozmiar)
	{ }

	template <typename Clique, typename Graph>
	void clique(const Clique& c, const Graph& g)
	{
		if(c.size() < minimalny_rozmiar)
			return;

		auto edges_source = boost::edges(g);
		std::vector<typename Graph::edge_descriptor> edges;
		std::unordered_set<typename Graph::vertex_descriptor> clique(c.begin(), c.end());
		std::copy_if(edges_source.first, edges_source.second, std::back_inserter(edges), [&](typename Graph::edge_descriptor e)
		{
			return clique.find(boost::target(e, g)) != clique.end() && clique.find(boost::source(e, g)) != clique.end();
		});
		serializuj_do_dot(output, g, c.begin(), c.end(), edges.begin(), edges.end());
	}
	std::ostream& output;
	int minimalny_rozmiar;
};

// zgodnie z sugesti¹
template<typename Clique>
Clique klika_plus_sasiedzi(const GrafZFeromonami& graf, Clique klika)
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

template<typename Clique>
std::vector<GrafZFeromonami::edge_descriptor> wierzcholki_kliki(const GrafZFeromonami& graf, Clique klika)
{
	std::vector<GrafZFeromonami::edge_descriptor> wierzcholki;
	for (auto& el : klika)
	{
		auto sasiednie = boost::out_edges(el, graf);
		auto src = sasiednie.first;
		auto dst = sasiednie.second;
		while (src != dst)
		{
			if (std::find(klika.begin(), klika.end(), boost::target(*src, graf)) != klika.end())
			{
				wierzcholki.push_back(*src);
			}
			src++;
		}
	}
	return wierzcholki;
}
#endif