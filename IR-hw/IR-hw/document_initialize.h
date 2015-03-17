#pragma once

#include "string_processing.h"
#include "configure.h"
#include "index.h"
#include "stop_words.h"

map<string, int> dictionary;

void init_dictionary_file() {
	FILE *dict_file = fopen(DICT_path, "w");

	fprintf(dict_file, "%d\n", dictionary.size());
	for (auto it = dictionary.begin(); it != dictionary.end(); it++) {
		fprintf(dict_file, "%s %d\n", it->first.c_str(), it->second);
	}

	fclose(dict_file);
}

void load_dictionary_file() {
	FILE *dict_file = fopen(DICT_path, "r");
	dictionary.clear();

	int n;
	fscanf(dict_file, "%d\n", &n);
	for (int i = 0; i < n; i++) {
		char term[100];
		int id;
		fscanf(dict_file, "%s%d\n", term, &id);
		dictionary[term] = id;
	}

	fclose(dict_file);	
}

void parse_to_RAW_file(const vector <string> &file_names) {
	int number_of_terms = 0;
	FILE *raw_file = fopen(RAW_path, "wb");

	char c_term[2048];
	ID *ids = new ID[4194304];

	for (int doc_id = 0; doc_id < file_names.size(); doc_id++) {
		FILE *file = fopen(file_names[doc_id].c_str(), "r");
		int n_id = 0;

		// Remove header
		while (fscanf(file, "%[a-zA-Z]:", c_term)) 
			fgets(c_term, 2048, file);

		while (fscanf(file, "%s", c_term) != EOF) {
			string term = c_term;

			vector <string> s_terms = split_tokens(lower_case(term));
			for (auto s_term: s_terms) {
				if (s_term.length() == 0) continue;

				if (stop_words.find(s_term) == stop_words.end()) {
					if (dictionary.count(s_term) == 0)
						dictionary[s_term] = number_of_terms++;
					int term_id = dictionary[s_term];

					if (n_id == 4194304) {
						fwrite(ids, sizeof(ID), n_id, raw_file);	
						n_id = 0;
					}
					ids[n_id++] = ID(term_id, doc_id);
			 	}
			}
		}
		if (n_id) 
			fwrite(ids, sizeof(ID), n_id, raw_file);	

		fclose(file);
	}

	delete ids;
	fclose(raw_file);
}
