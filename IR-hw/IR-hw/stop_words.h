#pragma once

#include "configure.h"
#include "string_processing.h"

set<string> stop_words;

void init_stop_words() {
	FILE *file = fopen(SW_path, "r");

    char word[1024];
    while (fscanf(file, "%s", word) != EOF)
        stop_words.insert(lower_case(word));

    fclose(file);
}
