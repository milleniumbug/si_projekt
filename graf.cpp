#include "stdafx.h"
#include "graf.h"

void serializuj_do_dot(std::ostream& os, const GrafZFeromonami& graf)
{
	auto vertices = boost::vertices(graf);
	auto edges = boost::edges(graf);
	serializuj_do_dot(os, graf, vertices.first, vertices.second, edges.first, edges.second);
}