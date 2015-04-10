#include "configure.h"
#include "document.h"
#include "query.h"

using namespace reuters21578;

namespace HAC {
	
	set<int> list;
	vector<int> count;
	vector<int> dsu;
	int DSU(int u) {
		return dsu[u] == u ? u : dsu[u] = DSU(dsu[u]);
	}

	vector<vector<double>> tfidf;
	void initialize() {
		count.resize(number_of_documents);
		dsu.resize(number_of_documents);
		for (int i = 0; i < number_of_documents; i++) {
			count[i] = 1;
			dsu[i] = i;
			list.insert(i);
		}

		tfidf.resize(number_of_documents, vector<double>());
		for (int i = 0; i < number_of_documents; i++)
			tfidf[i].resize(number_of_terms, 0);

		FILE *file = fopen(INDEX_path, "rb");

		for (Index index; fread(&index, sizeof(Index), 1, file);) {
			double tf = 0;
			if (index.freq > 0) tf = 1 + log(index.freq);

			int d_id = doc_id[index.doc_id];
			tfidf[d_id][index.term_id] = tf * idf[index.term_id];
		}

		fclose(file);
	}

	set<tuple<double, int, int>> edge;

	double sim(int i, int j) {
		double nume = 0;
		double deno = 0;
		for (int k = 0; k < number_of_terms; k++)
			nume += (tfidf[i][k] + tfidf[j][k]) * (tfidf[i][k] + tfidf[j][k]);
		nume -= count[i] + count[j];
		deno = (count[i] + count[j]) * (count[i] + count[j] - 1.0);
		return nume / deno;
	}

	void join(int i, int j) {
		i = DSU(i);
		j = DSU(j);
		dsu[j] = i;

		count[i] += count[j];
		for (int k = 0; k < number_of_terms; k++)
			tfidf[i][k] += tfidf[j][k];

		for (auto i : list) {
			if (i == j) continue;

			int u = i;
			int v = j;
			if (u > v) swap(u, v);

			auto it = edge.lower_bound(make_tuple(sim(u, v) - eps, u, v));
			if (it != edge.end() && get<1>(*it) == u && get<2>(*it) == v)
				edge.erase(it);
		}

		tfidf[j].clear();

		list.erase(j);
		for (auto j : list) {
			if (i == j) continue;
			
			int u = i;
			int v = j;
			if (u > v) swap(u, v);

			edge.insert(make_tuple(sim(u, v), u, v));
		}
	}

	vector<int> cluster(int n_cluster = 20) {
		cerr << "	Initializing terms ..." << endl;
		initialize();

		edge.clear();
		cerr << "	Initializing similarity ..." << endl;
		for (auto i : list) {
			cerr << "		(" << i << ", ... )" << endl;
			for (auto j : list) {
				if (i >= j) continue;
				
				edge.insert(make_tuple(sim(i, j), i, j));
			}
		}

		while (list.size() > n_cluster) {
			cerr << "	Processing ... " << edge.size() << " cluster(s) remaining" << endl;
			auto top = *edge.begin();
			join(get<1>(top), get<2>(top));
		}

		vector<int> ret;
		map<int, int> id;
		for (int i = 0; i < number_of_documents; i++) {
			int x = DSU(i);
			if (id.count(x) == 0)
				id[x] = id.size();

			ret.push_back(id[x]);
		}
		return ret;
	}
};