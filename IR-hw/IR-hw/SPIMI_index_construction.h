#pragma once

#include "index.h"
#include "configure.h"

class SPIMI {

private:

	priority_queue < pair <ID, int>, vector< pair <ID, int>>, greater < pair <ID, int>> > q;

	vector<int> posting_list[maximum_of_terms];
	vector<int> record_position;

	pair <FILE*, int> *blocks;

	void add_next_tokens(int block_id) {
		if (blocks[block_id].second) {
			ID id;
			fread(&id, sizeof(ID), 1, blocks[block_id].first);
			blocks[block_id].second--;

			q.push({id, block_id});
		} else 
			fclose(blocks[block_id].first);
	}

	int write_block_to_disk(FILE *&sorted_file) {
		for (int i = 0; i < maximum_of_terms; i++) {
			sort(posting_list[i].begin(), posting_list[i].end());
			for (int j = 0; j < posting_list[i].size(); j++) {
				ID id(i, posting_list[i][j]);
				fwrite(&id, sizeof(ID), 1, sorted_file);
			}
			posting_list[i].clear();
		}
		return ftell(sorted_file);
	}

public:

	void index_construction(string raw_path = RAW_path, string sorted_path = SORTED_path, string index_path = INDEX_path, int maximum_of_spaces = 200000) {

		// Sort Blocks
		FILE *raw_file = fopen(raw_path.c_str(), "rb");
		FILE *sorted_file = fopen(sorted_path.c_str(), "wb");

		int n_spaces = 0;

		record_position.clear();
		record_position.push_back(0);
		for (ID id; fread(&id, sizeof(ID), 1, raw_file); ) {
			n_spaces -= posting_list[id.term_id].capacity();
			posting_list[id.term_id].push_back(id.doc_id);
			n_spaces += posting_list[id.term_id].capacity();

			if (n_spaces >= maximum_of_spaces) {
				record_position.push_back(write_block_to_disk(sorted_file));
				n_spaces = 0;
			}
		}

		if (n_spaces) {
			record_position.push_back(write_block_to_disk(sorted_file));
			n_spaces = 0;
		}

		fclose(raw_file);
		fclose(sorted_file);

		// Merge Blocks

		FILE *index_file = fopen(index_path.c_str(), "wb");

		int n_blocks = record_position.size() - 1;
		blocks = new pair <FILE *, int>[n_blocks];


		for (int block_id = 0; block_id < n_blocks; block_id++) {
			blocks[block_id] = {fopen(sorted_path.c_str(), "rb"), (record_position[block_id + 1] - record_position[block_id]) / sizeof(ID)};
			fseek(blocks[block_id].first, record_position[block_id], SEEK_SET);
		}

		while (!q.empty()) q.pop();

		for (int i = 0; i < n_blocks; i++)
			add_next_tokens(i);

		ID prev(-1, -1);
		int freq = 0;
		while (!q.empty()) {
			pair <ID, int> cur = q.top(); q.pop();
			add_next_tokens(cur.second);
			if (prev != cur.first) {
				if (freq) {
					fwrite(&prev, sizeof(ID), 1, index_file);
					fwrite(&freq, sizeof(int), 1, index_file);
				}
				prev = cur.first;
				freq = 1;
			} else 
				freq++;
		}

		if (freq) {
			fwrite(&prev, sizeof(ID), 1, index_file);
			fwrite(&freq, sizeof(int), 1, index_file);
		}

		delete blocks;

		fclose(index_file);
	}

};