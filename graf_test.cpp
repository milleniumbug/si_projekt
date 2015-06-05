﻿#include "stdafx.h"
#include "mapa_kolorow.h"
#include "graf.h"
#include "mrowki.h"
#include "utils.h"
#include "graf_utils.h"
#include "kliki_klasycznie.h"

void usun_klike(std::vector<GrafZFeromonami::vertex_descriptor> vertexlist, GrafZFeromonami& graf)
{
	for(GrafZFeromonami::vertex_descriptor vertex : vertexlist)
	{
		clear_vertex(vertex, graf);
		remove_vertex(vertex, graf);
	}
}

std::vector<GrafZFeromonami::vertex_descriptor> znajdz_klike_w_punkcie(const GrafZFeromonami& graf, GrafZFeromonami::vertex_descriptor v, double threshold)
{
	std::unordered_set<GrafZFeromonami::vertex_descriptor> klika;
	std::deque<GrafZFeromonami::vertex_descriptor> do_odwiedzenia;
	klika.insert(v);
	do_odwiedzenia.push_back(v);

	while(!do_odwiedzenia.empty())
	{
		auto nast = do_odwiedzenia.front();
		do_odwiedzenia.pop_front();

		auto krawedzie = boost::out_edges(nast, graf);
		std::for_each(krawedzie.first, krawedzie.second, [&](GrafZFeromonami::edge_descriptor e)
		{
			double ilosc = graf[e].feromony.load();
			auto t = boost::target(e, graf);
			if(klika.find(t) == klika.end() && ilosc >= threshold)
			{
				do_odwiedzenia.push_back(t);
				klika.insert(t);
			}
		});
	}
	std::vector<GrafZFeromonami::vertex_descriptor> k(klika.begin(), klika.end());
	std::sort(k.begin(), k.end());
	return k;
}

NameMap zaladujgraf(GrafZFeromonami& graf, std::string filename, std::string mapfile)
{
	std::ifstream fin;
	std::vector<GrafZFeromonami::vertex_descriptor> vertex_list;

	fin.open(filename.c_str());
	assert(fin);
	std::string s1;
	while (std::getline(fin, s1))
	{
		std::stringstream tmp(s1);
		std::string s2;
		std::getline(tmp, s2, ' ');
		int i1 = atoi(s2.c_str());
		while (vertex_list.size() <= i1)
		{
			GrafZFeromonami::vertex_descriptor v = boost::add_vertex(graf);
			vertex_list.push_back(v);
		}
		while (std::getline(tmp, s2,' '))
		{
			int i2 = atoi(s2.c_str());
			while (vertex_list.size() <= i2)
			{
				GrafZFeromonami::vertex_descriptor v = boost::add_vertex(graf);
				vertex_list.push_back(v);
			}
			boost::add_edge(vertex_list[i1], vertex_list[i2], graf);
		}
	}
	fin.close();
	fin.open(mapfile.c_str());
	NameMap namemap;
	while (std::getline(fin, s1))
	{
		std::stringstream tmp(s1);
		std::string s2;
		std::getline(tmp, s2, '	');
		std::string s3;
		std::getline(tmp, s3, '	');
		int i = atoi(s3.c_str());
		namemap.insert(std::pair<GrafZFeromonami::vertex_descriptor, std::string>(vertex_list[i], s2));
	}
	return namemap;
}

template<typename RandomNumberGenerator, typename CliqueVisitor>
void jackson_milleniumbug_all_cliques(GrafZFeromonami& graf, RandomNumberGenerator& mt, CliqueVisitor visit, const double threshold_ratio)
{
	assert(threshold_ratio >= 0 && threshold_ratio <= 1);
	mrowki(graf, mt, 50);

	auto edges = boost::edges(graf);
	std::vector<GrafZFeromonami::edge_descriptor> sorted_edges(edges.first, edges.second);
	std::sort(sorted_edges.begin(), sorted_edges.end(), [&](GrafZFeromonami::edge_descriptor lhs, GrafZFeromonami::edge_descriptor rhs)
	{
		return graf[lhs].feromony.load() > graf[rhs].feromony.load();
	});

	const double threshold = threshold_ratio*graf[sorted_edges.front()].feromony.load();

	std::unordered_set<GrafZFeromonami::vertex_descriptor> elementy_klik_oznaczone;
	for(auto edge : sorted_edges)
	{
		auto s = boost::source(edge, graf);
		auto t = boost::target(edge, graf);

		auto znajdz_i_wypisz_klike = [&visit, &threshold, &elementy_klik_oznaczone](const GrafZFeromonami& g, GrafZFeromonami::vertex_descriptor v)
		{
			auto kl = znajdz_klike_w_punkcie(g, v, threshold);
			elementy_klik_oznaczone.insert(kl.begin(), kl.end());
			visit.clique(kl, g);
		};

		if(graf[edge].feromony.load() < threshold)
			break;

		if(!jest_w_zbiorze(elementy_klik_oznaczone, s))
			znajdz_i_wypisz_klike(graf, s);

		if(!jest_w_zbiorze(elementy_klik_oznaczone, t))
			znajdz_i_wypisz_klike(graf, s);
	}
}

void test(GrafZFeromonami& graf, boost::random::mt19937& mt, double threshold_ratio, bool continous = false, std::ostream& output = std::cout, NameMap namemap = NameMap())
{
	clique_printer<std::ostream> print(output, 10);
	clique_printer_as_comment<decltype(print)> unprint(print);

	clique_printer_with_name_map<std::ostream, NameMap> name_print(output, 10, namemap);
	clique_printer_as_comment<decltype(name_print)> name_unprint(name_print);
	do
	{
		if(namemap.empty())
			jackson_milleniumbug_all_cliques(graf, mt, unprint, threshold_ratio);
		else
			jackson_milleniumbug_all_cliques(graf, mt, name_unprint, threshold_ratio);

		auto edges = boost::edges(graf);
		std::vector<GrafZFeromonami::edge_descriptor> sorted_edges(edges.first, edges.second);
		std::sort(sorted_edges.begin(), sorted_edges.end(), [&](GrafZFeromonami::edge_descriptor lhs, GrafZFeromonami::edge_descriptor rhs)
		{
			return graf[lhs].feromony.load() > graf[rhs].feromony.load();
		});
		auto vertices = boost::vertices(graf);
		auto koniec_krawedzi_niezerowych = std::stable_partition(sorted_edges.begin(), sorted_edges.end(), [&](GrafZFeromonami::edge_descriptor edge)
		{
			return graf[edge].feromony.load() > 0.0;
		});
		const double max_feromonu = graf[sorted_edges.front()].feromony.load();
		const double threshold = threshold_ratio*graf[sorted_edges.front()].feromony.load();
		// // bez wierzchołków, z krawędziami o niezerowej ilości feromonu
		serializuj_do_dot(output, graf, vertices.first, vertices.first, sorted_edges.begin(), koniec_krawedzi_niezerowych, max_feromonu, threshold, namemap);
		// // wszystkie krawędzie
		// serializuj_do_dot(output, graf, vertices.first, vertices.second, sorted_edges.begin(), sorted_edges.end(), max_feromonu, threshold, namemap);

		output << "\n\n";
		if (continous) std::cout << "Continuing...\n";
	} while (continous);
	
	kliki_klasycznie(graf, output);
	output << "\n\n";
}

void testuj_kolejne(unsigned int seed)
{
	const double threshold = 0.0001;
	std::cout << "SEED: " << seed << "\n";
	std::cout << "THRESHOLD: " << threshold << "\n";
	std::cout << "\n";
	{
		boost::random::mt19937 mt(seed);
		GrafZFeromonami graf;
		wygeneruj_graf_z_klika(graf, 100, 150, 30, mt);
		std::ofstream output("out1.gv");
		test(graf, mt, threshold, false, output);
	}
	{
		boost::random::mt19937 mt(seed);
		GrafZFeromonami graf;
		static const int ile_grafow = 5;
		std::array<GrafZFeromonami, ile_grafow> grafy_pomocnicze;
		for(auto& graf_pomocniczy : grafy_pomocnicze)
		{
			wygeneruj_graf_z_klika(graf_pomocniczy, 100, 150, 30, mt);
		}

		// source: http://stackoverflow.com/a/24332273/1012936
		typedef GrafZFeromonami graph_t;
		typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_t;
		typedef boost::property_map<graph_t, boost::vertex_index_t>::type index_map_t;

		typedef boost::iterator_property_map<std::vector<vertex_t>::iterator,
			index_map_t, vertex_t, vertex_t&> IsoMap;

		std::array<std::vector<vertex_t>, ile_grafow> isoValues;
		std::vector<IsoMap> mapV;
		for(int i = 0; i < ile_grafow; ++i)
		{
			isoValues[i].resize(boost::num_vertices(grafy_pomocnicze[i]));
			mapV.push_back(IsoMap(isoValues[i].begin()));
			boost::copy_graph(grafy_pomocnicze[i], graf, boost::orig_to_copy(mapV[i]));
		}

		// połączeń między grafami jest n*(n-1)/2
		static const int wsp1 = (ile_grafow % 2 == 0) ? ile_grafow / 2 : ile_grafow;
		static const int wsp2 = ((ile_grafow - 1) % 2 == 0) ? (ile_grafow - 1) / 2 : (ile_grafow - 1);
		for(int i = 0; i < wsp1; ++i)
		{
			for(int j = 0; j < wsp2; ++j)
			{
				auto lewy = boost::random_vertex(grafy_pomocnicze[i], mt);
				auto prawy = boost::random_vertex(grafy_pomocnicze[j], mt);
				boost::add_edge(mapV[i][lewy], mapV[j][prawy], graf);
			}
		}
		std::ofstream output("out2.gv");
		test(graf, mt, threshold, false, output);
	}
	{
		boost::random::mt19937 mt(seed);
		GrafZFeromonami graf;
		NameMap namemap = zaladujgraf(graf, "temp-po_linkach-lista-simple-20120104_feed.txt", "temp-po_linkach-feature_dict-simple-20120104");
		std::ofstream output("out3.gv");
		test(graf, mt, threshold, false, output, namemap);
	}
	{
		boost::random::mt19937 mt(seed);
		GrafZFeromonami graf;
		NameMap namemap = zaladujgraf(graf, "temp-po_linkach-lista-simple-20120104_feed.txt", "temp-po_linkach-feature_dict-simple-20120104");
		std::ofstream output("out4.gv");
		test(graf, mt, threshold, true, output, namemap);
	}
}

void testuj_kolejne()
{
	std::random_device rd;
	auto seed = rd();
	testuj_kolejne(seed);
}