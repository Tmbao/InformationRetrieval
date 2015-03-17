#pragma once

#include "configure.h"
#include "index.h"

void verify_index_file() {
	FILE *file = fopen(INDEX_path, "rb");

	fseek(file, 0, SEEK_END);
	int file_size = ftell(file);
	int n_tokens = file_size / sizeof(Index);

	Index *tokens = new Index[n_tokens];

	fseek(file, 0, SEEK_SET);
	fread(tokens, sizeof(Index), n_tokens, file);

	int total_freq = tokens[0].freq;

	for (int i = 1; i < n_tokens; i++) {
		total_freq += tokens[i].freq;
		if (!(tokens[i-1] < tokens[i])) {
			cerr << tokens[i-1].term_id << " " << tokens[i-1].doc_id << "\n";
			cerr << tokens[i].term_id << " " << tokens[i].doc_id << "\n";
			cerr << "FAIL";
			break;
		}
	}

	delete tokens;

	fclose(file);
}