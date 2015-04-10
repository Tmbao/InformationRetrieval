#pragma once

#include "configure.h"
#include "string_processing.h"
#include "stop_words.h"

namespace twenty_newsgroups {

	const int query_length = 10;

	void build_query(string class_name, string class_path) {
		map <string, int> dictionary;
		vector< pair <int, string>> freq;
		char c_term[2048];

		DIR *dir = opendir(class_path.c_str());
		while (dirent *pdir = readdir(dir)) {
			string name = pdir->d_name;

			if (name != "." && name != ".." && name != "desktop.ini") {
				name = class_path + "\\" + name;
				FILE *file = fopen(name.c_str(), "r");

				// Remove header
				while (fscanf(file, "%[a-zA-Z]:", c_term))
					fgets(c_term, 2048, file);

				while (fscanf(file, "%s", c_term) != EOF) {
					string term = c_term;

					vector<string> s_terms = split_tokens(lower_case(term));
					for (auto s_term : s_terms) {
						if (s_term.length() == 0) continue;

						if (stop_words.find(s_term) == stop_words.end()) {
							if (dictionary.count(s_term) == 0) {
								dictionary[s_term] = dictionary.size();
								freq.push_back({ 0, s_term });
							}
							int term_id = dictionary[s_term];
							freq[term_id].first++;
						}
					}
				}

				fclose(file);
			}
		}
		(void)closedir(dir);

		sort(freq.rbegin(), freq.rend());

		string file_name = "Data\\query\\" + class_name;
		FILE *file_query = fopen(file_name.c_str(), "w");

		for (int i = 0; i < min((int)freq.size(), query_length); i++)
			fprintf(file_query, "%s ", freq[i].second.c_str());

		fclose(file_query);
	}

};