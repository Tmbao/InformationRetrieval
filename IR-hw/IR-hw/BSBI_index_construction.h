#pragma once

#include "index.h"
#include "configure.h"

class BSBI {

private:

	priority_queue < pair <ID, int>, vector < pair <ID, int> >, greater < pair <ID, int> > > q;

	pair <FILE *, int> *partitions;

	void add_next_tokens(int partition_id) {
		if (partitions[partition_id].second) {
			ID id;
			fread(&id, sizeof(ID), 1, partitions[partition_id].first);
			partitions[partition_id].second--;

			q.push({id, partition_id});
		} else 
			fclose(partitions[partition_id].first);
	}

	void sort_partition(FILE *&raw_file, FILE *&sorted_file, int n_tokens) {
		ID *tokens = new ID[n_tokens];

		fread(tokens, sizeof(ID), n_tokens, raw_file);
		sort(tokens, tokens + n_tokens);
		fwrite(tokens, sizeof(ID), n_tokens, sorted_file);

		delete tokens;
	}

public:

	void index_construction(int n_partitions = 10) {

		// Partition and Sort
		FILE *raw_file = fopen(RAW_path, "rb");
		FILE *sorted_file = fopen(SORTED_path, "wb");

		fseek(raw_file, 0, SEEK_END);
		int file_size = ftell(raw_file);
		int n_tokens = file_size / sizeof(ID);
		int tokens_per_partition = (n_tokens - 1) / n_partitions + 1;

		fseek(raw_file, 0, SEEK_SET);
		for (int partition_id = 0, remain_tokens = n_tokens; partition_id < n_partitions; partition_id++) {
			sort_partition(raw_file, sorted_file, min(remain_tokens, tokens_per_partition));
			remain_tokens -= tokens_per_partition;
		}

		fclose(raw_file);
		fclose(sorted_file);

		// Merge Partitions
		FILE *index_file = fopen(INDEX_path, "wb");

		partitions = new pair <FILE *, int>[n_partitions];

		for (int partition_id = 0, remain_tokens = n_tokens; partition_id < n_partitions; partition_id++) {
			partitions[partition_id] = {fopen(SORTED_path, "rb"), min(remain_tokens, tokens_per_partition)};
			fseek(partitions[partition_id].first, partition_id * tokens_per_partition * sizeof(ID), SEEK_SET);
			remain_tokens -= tokens_per_partition;
		}

		while (!q.empty()) q.pop();

		for (int i = 0; i < n_partitions; i++)
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

		delete partitions;

		fclose(index_file);
	}

};