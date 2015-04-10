#pragma once
#include "configure.h"
#include "category.h"
#include "query.h"
#include "document.h"
using namespace reuters21578;

namespace k_nearest_neighbor {

	category cat;

	string classify(vector<string> terms, int k = 10) {
		if (cat.size() == 0) 
			cat = category(CATEGORY_path);
		
		map <string, int> voting;

		auto rank_list = query(terms);
		for (int i = 0; i < min(k, terms.size()); i++) 
			voting[cat[rank_list[i].second]]++;

		string ret = "";
		int max_voting = 0;
		for (auto it = voting.begin(); it != voting.end(); it++) {
			if (max_voting < it->second) {
				ret = it->first;
				max_voting = it->second;
			}
		}

		return ret;
	}
};

namespace naive_bayes {
	category cat;
	map < string,  map <int, int>> c_freq;
	map <string, int> c_terms;
	int n_terms;

	bool is_initialized = false;
	void initialize() {
		// Load Docs
		FILE *file = fopen(INDEX_path, "rb");
		n_terms;
		for (Index index; fread(&index, sizeof(Index), 1, file);) {
			string cate = cat[doc_id[index.doc_id]];
			c_freq[cate][index.term_id] += index.freq;
			c_terms[cate] += index.freq;
			n_terms += index.freq;
		}
		fclose(file);

		is_initialized = true;
	}

	string classify(vector<string> terms) {
		if (cat.size() == 0)
			cat = category(CATEGORY_path);
		if (!is_initialized)
			initialize();
		
		string ret = "";
		double s_ret;
		for (auto it = c_freq.begin(); it != c_freq.end(); it++) {
			double score = log(c_terms[it->first] / double(n_terms));
			for (auto term : terms) 
				score += log((c_freq[it->first][dictionary[term]] + 1.0) / (c_terms[it->first] + dictionary.size()));

			if (ret == "" || s_ret < score) {
				ret = it->first;
				s_ret = score;
			}
		}

		return ret;				
	}
};
