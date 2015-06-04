#ifndef GRAF_UTILS_H
#define GRAF_UTILS_H

template <typename OutputStream>
struct clique_printer
{
	clique_printer(OutputStream& stream, int minsize_)
		: os(stream), minsize(minsize_)
	{ }

	template <typename Clique, typename Graph>
	void clique(const Clique& c, const Graph& g)
	{
		if(c.size() < minsize) return;
		// Iterate over the clique and print each vertex within it.
		typename Clique::const_iterator i, end = c.end();
		for(i = c.begin(); i != end; ++i) {
			os << *i << " ";
		}
		os << std::endl;
	}
	OutputStream& os;
	int minsize;
};

template <typename OutputStream>
struct clique_printer_as_comment
{
	clique_printer<OutputStream> printer;

	clique_printer_as_comment(OutputStream& stream, int minsize_)
		: printer(stream, minsize_)
	{ }

	template <typename Clique, typename Graph>
	void clique(const Clique& c, const Graph& g)
	{
		if(c.size() < printer.minsize) return;
		// Iterate over the clique and print each vertex within it.
		printer.os << "// ";
		printer.clique(c, g);
	}
};

struct serializator_klik
{
	serializator_klik(std::ostream& os, int minimalny_rozmiar) :
		os(os),
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
		serializuj_do_dot(os, g, c.begin(), c.end(), edges.begin(), edges.end());
	}
	std::ostream& os;
	int minimalny_rozmiar;
};

#endif