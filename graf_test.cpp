#include <iostream>
#include <ostream>
#include <iomanip>
#include <atomic>
#include <deque>
#include <future>
#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>
#include <random>
#include <boost/random.hpp>

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

template<typename Derived, typename RandomNumberGenerator>
class MrowkaBase
{
private:
	GrafZFeromonami::vertex_descriptor stara_pozycja_;
	GrafZFeromonami::vertex_descriptor pozycja_;
	GrafZFeromonami* graf_;
	RandomNumberGenerator ran_;

	int nr_tury_;

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

		auto list = boost::out_edges(pozycja_, graf);
		if(list.first == list.second)
			return false;

		std::vector<GrafZFeromonami::vertex_descriptor> docelowe;
		std::vector<GrafZFeromonami::edge_descriptor> edges;
		std::vector<double> wartosci_feromonow;
		std::for_each(list.first, list.second, [&](GrafZFeromonami::edge_descriptor e)
		{
			auto docelowy = boost::target(e, graf);
			if(docelowy == stara_pozycja_)
				return;

			docelowe.push_back(docelowy);
			edges.push_back(e);
			// UWAGA - tu dotykamy danych w grafie
			double poziom_feromonu = zmienialny_graf[e].feromony.load();
			poziom_feromonu = obecny_poziom_feromonu(poziom_feromonu, nr_tury_);
			poziom_feromonu = std::pow(poziom_feromonu, 1.1) + 1;
			wartosci_feromonow.push_back(poziom_feromonu);
		});

		// wyląduj na docelowym wierzchołku w zależności od wartości feromonu
		boost::random::discrete_distribution<> dist(wartosci_feromonow.begin(), wartosci_feromonow.end());
		int wylosowany_indeks_wierzcholka = dist(ran_);
		stara_pozycja_ = pozycja_;
		pozycja_ = docelowe[wylosowany_indeks_wierzcholka];

		auto list2 = boost::out_edges(pozycja_, graf);
		docelowe.clear();
		std::for_each(list2.first, list2.second, [&](GrafZFeromonami::edge_descriptor e)
		{
			auto docelowy = boost::target(e, graf);
			docelowe.push_back(docelowy);
		});

		interlocked_increase(zmienialny_graf[edges[wylosowany_indeks_wierzcholka]].feromony, derived().ocen_wierzcholek(pozycja_, docelowe));

		++nr_tury_;
		return true;
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

void mrowki(GrafZFeromonami& graf, const int ilosc_watkow = std::thread::hardware_concurrency())
{
	std::random_device rd;
	boost::random::mt19937 mt(rd());
	typedef MrowkaKlika Mrowka;
	static const int ilosc_ruchow = 5000;
	static const int ilosc_mrowek_na_watek = 1;

	std::vector<std::vector<Mrowka>> mrowiska(ilosc_watkow);
	for(int i = 0; i < ilosc_watkow; ++i)
		for(int j = 0; j < ilosc_mrowek_na_watek; ++j)
			mrowiska[i].push_back(Mrowka(graf, boost::random_vertex(graf, mt), Mrowka::random_number_generator(rd())));
	std::vector<std::future<void>> wyniki(ilosc_watkow);
	for(int i = 0; i < ilosc_watkow; ++i)
	{
		wyniki[i] = std::async(std::launch::async, [](std::vector<Mrowka>& mrowisko)
		{
			for(int i = 0; i < ilosc_ruchow; ++i)
				for(auto& mrowka : mrowisko)
					mrowka.nastepny_ruch();
		}, std::ref(mrowiska[i]));
	}

	for(auto& wynik : wyniki)
		wynik.wait();

	auto edges = boost::edges(graf);
	std::for_each(edges.first, edges.second, [&](GrafZFeromonami::edge_descriptor v)
	{
		double x = graf[v].feromony.load();
		x = obecny_poziom_feromonu(x, ilosc_ruchow);
		graf[v].feromony.store(x);
	});
}

void kliki_klasycznie(GrafZFeromonami& graf);

template<typename VertexIterator, typename EdgeIterator>
void serializuj_do_dot(std::ostream& os, const GrafZFeromonami& graf, VertexIterator vb, VertexIterator ve, EdgeIterator eb, EdgeIterator ee)
{
	os << "strict graph {\n";
	std::for_each(vb, ve, [&](GrafZFeromonami::vertex_descriptor vertex)
	{
		os << vertex << "\n";
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

void test()
{
	GrafZFeromonami graf;
	std::random_device rd;
	boost::random::mt19937 mt(rd());

	wygeneruj_graf_z_klika(graf, 100, 150, 30, mt);
	mrowki(graf, 1);


	auto edges = boost::edges(graf);
	std::vector<GrafZFeromonami::edge_descriptor> sorted_edges(edges.first, edges.second);
	std::sort(sorted_edges.begin(), sorted_edges.end(), [&](GrafZFeromonami::edge_descriptor lhs, GrafZFeromonami::edge_descriptor rhs)
	{
		return graf[lhs].feromony.load() > graf[rhs].feromony.load();
	});
	auto vertices = boost::vertices(graf);
	serializuj_do_dot(std::cout, graf, vertices.first, vertices.second, sorted_edges.begin(), sorted_edges.begin()+10);
	std::cout << "\n\n";

	kliki_klasycznie(graf);
	std::cout << "\n\n";
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

void kliki_klasycznie(GrafZFeromonami& graf)
{
	std::size_t size = 0;
	clique_printer<std::ostream> vis(std::cout, 10);
	boost::bron_kerbosch_all_cliques(graf, vis);
}