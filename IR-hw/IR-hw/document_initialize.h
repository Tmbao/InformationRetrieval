#pragma once

#include "string_processing.h"
#include "configure.h"
#include "index.h"
#include "topic.h"
#include "stop_words.h"

namespace twenty_newsgroups {
	int number_of_terms, number_of_documents;
	map<string, int> dictionary;
	vector <int> doc_id, doc_uid;

	void init_dictionary_file() {
		FILE *dict_file = fopen(DICT_path, "w");

		fprintf(dict_file, "%d %d\n", number_of_terms, number_of_documents);
		for (auto it = dictionary.begin(); it != dictionary.end(); it++)
			fprintf(dict_file, "%s %d\n", it->first.c_str(), it->second);

		fclose(dict_file);
	}

	void load_dictionary_file() {
		FILE *dict_file = fopen(DICT_path, "r");
		
		fscanf(dict_file, "%d%d", &number_of_terms, &number_of_documents);
		dictionary.clear();
		char c_term[2048];
		int id;
		for (int i = 0; i < number_of_terms; i++) {
			fscanf(dict_file, "%s%d", c_term, &id);
			dictionary[c_term] = id;
		}

		for (int i = 0; i < number_of_documents; i++) {
			doc_id.push_back(i);
			doc_uid.push_back(i);
		}

		fclose(dict_file);
	}

	vector <string> read_file(const string &file_name) {
		vector <string> ret;
		FILE *file = fopen(file_name.c_str(), "r");
		char c_term[2048];

		// Remove header
		while (fscanf(file, "%[a-zA-Z]:", c_term))
			fgets(c_term, 2048, file);

		while (fscanf(file, "%s", c_term) != EOF) {
			string term = c_term;

			vector <string> s_terms = split_tokens(lower_case(term));
			for (auto s_term : s_terms) {
				if (s_term.length() == 0) continue;
				if (stop_words.find(s_term) == stop_words.end()) 
					ret.push_back(s_term);
			}
		}

		fclose(file);

		return ret;
	}

	void parse_to_RAW_file(const vector <string> &file_names, string raw_path) {
		number_of_terms = 0;
		number_of_documents = file_names.size();
		FILE *raw_file = fopen(raw_path.c_str(), "wb");

		char c_term[2048];
		ID *ids = new ID[4194304];

		for (int doc_id = 0; doc_id < file_names.size(); doc_id++) {
			int n_id = 0;
			vector <string> terms = read_file(file_names[doc_id]);

			for (auto term : terms) {
				if (term.length() == 0) continue;

				if (stop_words.find(term) == stop_words.end()) {
					if (dictionary.count(term) == 0)
						dictionary[term] = number_of_terms++;
					int term_id = dictionary[term];

					if (n_id == 4194304) {
						fwrite(ids, sizeof(ID), n_id, raw_file);
						n_id = 0;
					}
					ids[n_id++] = ID(term_id, doc_id);
				}
			}
			
			if (n_id)
				fwrite(ids, sizeof(ID), n_id, raw_file);
		}

		delete ids;
		fclose(raw_file);
	}

};

namespace ohsu_trec {

	int number_of_terms, number_of_documents;
	map <string, int> dictionary;
	map <int, int> doc_id;
	vector <int> doc_uid;

	void init_dictionary_file() {
		FILE *dict_file = fopen(DICT_path, "w");

		fprintf(dict_file, "%d\n", number_of_terms);
		for (auto it = dictionary.begin(); it != dictionary.end(); it++) 
			fprintf(dict_file, "%s %d\n", it->first.c_str(), it->second);
		fprintf(dict_file, "%d\n", number_of_documents);
		for (map <int, int> ::iterator it = doc_id.begin(); it != doc_id.end(); it++)
			fprintf(dict_file, "%d %d\n", it->first, it->second);

		fclose(dict_file);
	}

	void load_dictionary_file() {
		FILE *dict_file = fopen(DICT_path, "r");
		dictionary.clear();
		doc_id.clear();
		doc_uid.clear();

		fscanf(dict_file, "%d\n", &number_of_terms);
		for (int i = 0; i < number_of_terms; i++) {
			char term[1024];
			int id;
			fscanf(dict_file, "%s%d\n", term, &id);
			dictionary[term] = id;
		}
		fscanf(dict_file, "%d\n", &number_of_documents);
		doc_uid = vector <int>(number_of_documents);
		for (int i = 0; i < number_of_documents; i++) {
			int u_id, id;
			fscanf(dict_file, "%d%d", &u_id, &id);
			doc_id[u_id] = id;
			doc_uid[id] = u_id;
			//doc_uid.push_back(u_id);
		}
		//sort(doc_uid.begin(), doc_uid.end());
		fclose(dict_file);
		
	}

	void save_topic_to_RAW_file(FILE *&raw_file, const topic &tp) {
		doc_id[tp.u_id] = number_of_documents++;

		ID *ids;
		vector<string> tokens;

		tokens = split_tokens(tp.title, " .,;\"\r\t\n()[]{}&':?+-*/");
		ids = new ID[tokens.size()];
		for (int i = 0; i < tokens.size(); i++) {
			if (stop_words.find(tokens[i]) != stop_words.end())
				continue;
			if (dictionary.count(tokens[i]) == 0)
				dictionary[tokens[i]] = number_of_terms++;
			ids[i] = ID(dictionary[tokens[i]], tp.u_id);
		}
		fwrite(ids, sizeof(ID), tokens.size(), raw_file);
		delete ids;
		
		tokens = split_tokens(tp.abstract, " .,;\"\r\t\n()[]{}&':?+-*/");
		ids = new ID[tokens.size()];
		for (int i = 0; i < tokens.size(); i++) {
			if (stop_words.find(tokens[i]) != stop_words.end())
				continue;
			if (dictionary.count(tokens[i]) == 0)
				dictionary[tokens[i]] = number_of_terms++;
			ids[i] = ID(dictionary[tokens[i]], tp.u_id);
		}
		fwrite(ids, sizeof(ID), tokens.size(), raw_file);
		delete ids;

		/*ids = new ID[tp.terms.size()];
		for (int i = 0; i < tp.terms.size(); i++) {
			if (stop_words.find(tp.terms[i]) != stop_words.end())
				continue;
			if (dictionary.count(tp.terms[i]) == 0)
				dictionary[tp.terms[i]] = number_of_terms++;
			ids[i] = ID(dictionary[tp.terms[i]], tp.u_id);
		}
		fwrite(ids, sizeof(ID), tp.terms.size(), raw_file);
		delete ids;*/
	}

	void parse_to_RAW_file(vector <string> file_names, string raw_path) {
		FILE *raw_file = fopen(raw_path.c_str(), "wb");

		char c_term[8192];
		number_of_terms = 0;
		for (string file_name : file_names) {
			FILE *file = fopen(file_name.c_str(), "r");

			topic *tp = NULL;
			do {
				if (fscanf(file, "%s", c_term) == EOF)
					break;
				
				vector<string> tags;
				switch (c_term[1]) {
				case 'I': // Sequential ID
					if (tp != NULL) {
						save_topic_to_RAW_file(raw_file, *tp);
						delete tp;
					}
					int id;
					fscanf(file, "%d\n", &id);
					tp = new topic(id);
					break;

				case 'U': // Document ID			
					fscanf(file, "%d\n", &tp->u_id);
					break;

				case 'M': // Mesh
					fscanf(file, "\n");
					fgets(c_term, 8192, file);
					tags = split_tokens(c_term, ";.\n");
					for (string tag : tags) {
						vector <string> tokens = split_tokens(tag, " ,\n");
						for (int i = 0; i < tokens.size(); i++)
							tp->terms.push_back(tokens[i]);
					}
					break;

				case 'T': // Title
					fscanf(file, "\n");
					fgets(c_term, 8192, file);
					tp->title = c_term;
					break;

				case 'P': // Publish
					fscanf(file, "\n");
					fgets(c_term, 8192, file);
					tp->pub_type = c_term;
					break;

				case 'W': // Abstract
					fscanf(file, "\n");
					fgets(c_term, 8192, file);
					tp->abstract = c_term;
					break;

				case 'A': // Author
					fscanf(file, "\n");
					fgets(c_term, 8192, file);
					tp->author = c_term;
					break;

				case 'S': // Source
					fscanf(file, "\n");
					fgets(c_term, 8192, file);
					tp->source = c_term;
					break;

				}

			} while (true);
			if (tp != NULL) {
				save_topic_to_RAW_file(raw_file, *tp);
				delete tp;
			}

			fclose(file);
		}

		fclose(raw_file);
	}

};