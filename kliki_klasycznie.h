#ifndef KLIKI_KLASYCZNIE_H
#define KLIKI_KLASYCZNIE_H
#include <boost/graph/bron_kerbosch_all_cliques.hpp>
#include "graf.h"
#include "graf_utils.h"

inline void kliki_klasycznie(const GrafZFeromonami& graf, std::ostream& output)
{
	clique_printer<std::ostream> print(output, 10);
	clique_printer_as_comment<decltype(print)> unprint(print);
	boost::bron_kerbosch_all_cliques(graf, unprint);
}

#endif