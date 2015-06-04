#include "stdafx.h"

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

// interlocked_increase
// atomowo zwiększa wartość `value` o `what`
// zwraca starą wartość `value` przed inkrementacją
double interlocked_increase(std::atomic<double>& value, double what)
{
	double stare = value.load();
	double nowe;
	do
	{
		nowe = stare + what;
	} while(!value.compare_exchange_weak(stare, nowe));
	return stare;
}

double obecny_poziom_feromonu(double zawartosc, int nr_tury)
{
	return zawartosc * std::pow(0.999, nr_tury);
}

typedef boost::adjacency_list<boost::vecS,
	boost::vecS,
	boost::undirectedS,
	boost::no_property,
	WlasnosciKrawedzi> GrafZFeromonami;

std::map<GrafZFeromonami::vertex_descriptor, std::string> namemap;

template<typename Zbior, typename Element>
bool jest_w_zbiorze(const Zbior& zb, const Element& el)
{
	return zb.find(el) != zb.end();
}

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
		auto v = do_odwiedzenia.front();
		do_odwiedzenia.pop_front();

		auto krawedzie = boost::out_edges(v, graf);
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
	
template<typename Derived, typename RandomNumberGenerator>
class MrowkaBase
{
private:
	GrafZFeromonami::vertex_descriptor stara_pozycja_;
	GrafZFeromonami::vertex_descriptor pozycja_;
	GrafZFeromonami* graf_;
	RandomNumberGenerator ran_;

	int nr_tury_;

	void ustaw_nowa_pozycje(GrafZFeromonami::vertex_descriptor nowa_pozycja)
	{
		stara_pozycja_ = pozycja_;
		pozycja_ = nowa_pozycja;
	}

protected:
	const Derived& derived() const
	{
		return *static_cast<const Derived*>(this);
	}
	Derived& derived()
	{
		return *static_cast<Derived*>(this);
	}
public:
	MrowkaBase(GrafZFeromonami& graf, GrafZFeromonami::vertex_descriptor pozycja, RandomNumberGenerator ran) :
		stara_pozycja_(pozycja),
		pozycja_(pozycja),
		graf_(&graf),
		ran_(ran),
		nr_tury_(0)
	{

	}

	bool nastepny_ruch()
	{
		auto& zmienialny_graf = *graf_;
		const auto& graf = zmienialny_graf;

		std::vector<GrafZFeromonami::edge_descriptor> edges;
		{
			auto list = boost::out_edges(pozycja_, graf);
			std::copy(list.first, list.second, std::back_inserter(edges));
		}

		if(edges.empty())
			return false;

		std::vector<double> wartosci_feromonow;
		std::transform(edges.begin(), edges.end(), std::back_inserter(wartosci_feromonow), [&](GrafZFeromonami::edge_descriptor e)
		{
			if(boost::target(e, graf) == stara_pozycja_)
				return 0.0;

			double poziom_feromonu = zmienialny_graf[e].feromony.load();
			poziom_feromonu = obecny_poziom_feromonu(poziom_feromonu, nr_tury_);
			poziom_feromonu = std::pow(poziom_feromonu, 1.1) + 1;
			return poziom_feromonu;
		});

		// wyląduj na docelowym wierzchołku w zależności od wartości feromonu
		boost::random::discrete_distribution<> dist(wartosci_feromonow.begin(), wartosci_feromonow.end());
		GrafZFeromonami::edge_descriptor wylosowana_krawedz = edges[dist(ran_)];

		ustaw_nowa_pozycje(boost::target(wylosowana_krawedz, graf));

		std::vector<GrafZFeromonami::vertex_descriptor> docelowe;
		{
			auto list = boost::out_edges(pozycja_, graf);
			std::transform(list.first, list.second, std::back_inserter(docelowe), [&graf](GrafZFeromonami::edge_descriptor e)
			{
				return boost::target(e, graf);
			});
		}

		interlocked_increase(zmienialny_graf[wylosowana_krawedz].feromony, derived().ocen_wierzcholek(pozycja_, docelowe));

		++nr_tury_;
		return true;
	}

	int wykonano_ruchow() const
	{
		return nr_tury_;
	}

	typedef RandomNumberGenerator random_number_generator;

	// GŁUPOTA VISUAL STUDIO
	// to powinno działać, bo C++11, ale to VS2013, jeszcze nie dojrzał na tyle.
	//MrowkaBase(const MrowkaBase&) = delete;
	//MrowkaBase& operator=(const MrowkaBase&) = delete;
	//MrowkaBase(MrowkaBase&&) = default;
	//MrowkaBase& operator=(MrowkaBase&&) = default;
};

class MrowkaKlika : public MrowkaBase<MrowkaKlika, boost::random::minstd_rand>
{
private:
	std::vector<GrafZFeromonami::vertex_descriptor> poprzedni_zbior;
public:
	MrowkaKlika(GrafZFeromonami& graf, GrafZFeromonami::vertex_descriptor pozycja, random_number_generator ran) :
		MrowkaBase(graf, pozycja, ran)
	{

	}

	double ocen_wierzcholek(GrafZFeromonami::vertex_descriptor ten_wierzcholek, const std::vector<GrafZFeromonami::vertex_descriptor>& sasiedzi)
	{
		assert(!sasiedzi.empty());

		double ile_wspolnych = std::count_if(sasiedzi.begin(), sasiedzi.end(), [&](GrafZFeromonami::vertex_descriptor v)
		{
			return std::find(poprzedni_zbior.begin(), poprzedni_zbior.end(), v) != poprzedni_zbior.end();
		});
		double ilosc_sasiadow = sasiedzi.size();
		poprzedni_zbior = sasiedzi;
		return (ile_wspolnych / ilosc_sasiadow) * 200 * std::pow(ilosc_sasiadow, 0.75);
	}
};

template<typename RandomNumberGenerator>
void mrowki(GrafZFeromonami& graf, RandomNumberGenerator& rng, const int ilosc_mrowek, const int ilosc_watkow = std::thread::hardware_concurrency())
{
	typedef MrowkaKlika Mrowka;
	const int ilosc_ruchow = 5000;
	assert(ilosc_watkow > 0);

	std::vector<std::vector<Mrowka>> mrowiska(ilosc_watkow);

	auto wypeln_mrowiska = [&mrowiska, &graf, &rng, &ilosc_mrowek]()
	{
		for(int i = 0; i < ilosc_mrowek; ++i)
		{
			const int obecne_mrowisko = i % mrowiska.size();
			mrowiska[obecne_mrowisko].push_back(Mrowka(graf, boost::random_vertex(graf, rng), Mrowka::random_number_generator(rng())));
		}
	};
	wypeln_mrowiska();

	auto symuluj_akcje_mrowek = [&graf, &mrowiska, &ilosc_watkow, &ilosc_ruchow]()
	{
		std::vector<std::future<void>> wyniki(ilosc_watkow);
		for(int i = 0; i < ilosc_watkow; ++i)
		{
			wyniki[i] = std::async(std::launch::async, [](std::vector<Mrowka>& mrowisko, const int ilosc_ruchow)
			{
				for(int i = 0; i < ilosc_ruchow; ++i)
					for(auto& mrowka : mrowisko)
						mrowka.nastepny_ruch();
			}, std::ref(mrowiska[i]), ilosc_ruchow);
		}
		for(auto& wynik : wyniki)
			wynik.wait();
	};
	symuluj_akcje_mrowek();

	auto ustaw_oczekiwane_wartosci_feromonu = [&graf, &ilosc_ruchow]()
	{
		auto edges = boost::edges(graf);
		std::for_each(edges.first, edges.second, [&](GrafZFeromonami::edge_descriptor v)
		{
			double x = graf[v].feromony.load();
			x = obecny_poziom_feromonu(x, ilosc_ruchow);
			graf[v].feromony.store(x);
		});
	};
	ustaw_oczekiwane_wartosci_feromonu();
}

void kliki_klasycznie(const GrafZFeromonami& graf);

template<typename VertexIterator, typename EdgeIterator>
void serializuj_do_dot(std::ostream& os, const GrafZFeromonami& graf, VertexIterator vb, VertexIterator ve, EdgeIterator eb, EdgeIterator ee)
{
	os << "strict graph {\n";
	std::for_each(vb, ve, [&](GrafZFeromonami::vertex_descriptor vertex)
	{
		if (!namemap.empty())
		{
			
			os << vertex << " [ name = " << namemap[vertex] << "]\n";
		}
		else os << vertex << "\n";
	});
	std::for_each(eb, ee, [&](GrafZFeromonami::edge_descriptor edge)
	{
		os << boost::source(edge, graf) << " -- " << boost::target(edge, graf) << " [ fer = " << std::fixed << graf[edge].feromony.load() << "; ]" << "\n";
	});
	os << "}\n";
}

void serializuj_do_dot(std::ostream& os, const GrafZFeromonami& graf)
{
	auto vertices = boost::vertices(graf);
	auto edges = boost::edges(graf);
	serializuj_do_dot(os, graf, vertices.first, vertices.second, edges.first, edges.second);
}

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

void zaladujgraf(GrafZFeromonami& graf,std::string filename,std::string mapfile)
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
}

void test(GrafZFeromonami& graf, boost::random::mt19937& mt, double threshold_ratio, bool continous = false)
{
	do
	{
		assert(threshold_ratio >= 0 && threshold_ratio <= 1);
		mrowki(graf, mt, 50, 4);

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
		serializuj_do_dot(std::cout, graf, vertices.first, vertices.first, sorted_edges.begin(), koniec_krawedzi_niezerowych);
		std::cout << "\n\n";

		std::unordered_set<GrafZFeromonami::vertex_descriptor> elementy_klik_oznaczone;
		const double threshold = threshold_ratio*graf[sorted_edges.front()].feromony.load();
		for(auto edge : sorted_edges)
		{
			auto s = boost::source(edge, graf);
			auto t = boost::target(edge, graf);

			auto znajdz_i_wypisz_klike = [&](GrafZFeromonami::vertex_descriptor v)
			{
				auto kl = znajdz_klike_w_punkcie(graf, v, threshold);
				elementy_klik_oznaczone.insert(kl.begin(), kl.end());
				if (!namemap.empty())
				{
					auto kt = kl.cbegin();
					while (kt != kl.end())
					{
						std::cout << namemap[*kt] << " ";
						kt++;
					}
				} else
					std::copy(kl.begin(), kl.end(), std::ostream_iterator<GrafZFeromonami::vertex_descriptor>(std::cout, " "));
				std::cout << "\n";
				//if(continous)
				//	usun_klike(kl, graf);
			};

			if(graf[edge].feromony.load() < threshold)
				break;

			if(!jest_w_zbiorze(elementy_klik_oznaczone, s))
				znajdz_i_wypisz_klike(s);

			if(!jest_w_zbiorze(elementy_klik_oznaczone, t))
				znajdz_i_wypisz_klike(s);
		}
		std::cout << "\n\n";
		if (continous) std::cout << "Continuing...\n";
	} while (continous);
	

	kliki_klasycznie(graf);
	std::cout << "\n\n";
}

void testuj_kolejne(unsigned int seed)
{
	// najmniejsza liczba dodatnia
	const double zeroplus = std::nextafter(0.0, std::numeric_limits<double>::infinity());
	const double threshold = 0.0001;
	std::cout << "SEED: " << seed << "\n";
	std::cout << "THRESHOLD: " << threshold << "\n";
	std::cout << "\n";
	{
		boost::random::mt19937 mt(seed);
		GrafZFeromonami graf;
		wygeneruj_graf_z_klika(graf, 100, 150, 30, mt);
		test(graf, mt, threshold,false);
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
		test(graf, mt, threshold,false);
	}
	{
		boost::random::mt19937 mt(seed);
		GrafZFeromonami graf;
		zaladujgraf(graf, "temp-po_linkach-lista-simple-20120104_feed.txt", "temp-po_linkach-feature_dict-simple-20120104");
		test(graf, mt, threshold,true);
	}
}

void testuj_kolejne()
{
	std::random_device rd;
	auto seed = rd();
	testuj_kolejne(seed);
}

#include <boost/graph/bron_kerbosch_all_cliques.hpp>


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

void kliki_klasycznie(const GrafZFeromonami& graf)
{
	clique_printer<std::ostream> vis(std::cout, 10);
	boost::bron_kerbosch_all_cliques(graf, vis);
}