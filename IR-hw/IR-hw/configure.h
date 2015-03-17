#pragma once

#include "dirent.h"

#include <cstdlib>
#include <cstring>
#include <cmath>

#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <functional>
using namespace std;

#define sqr(x) (x) * (x)

#define SW_path "Data\stopwords_en.txt"
#define RAW_path "Data\RAW.bin"
#define DICT_path "Data\DICT.txt"
#define INDEX_path "Data\INDEX.bin"
#define SORTED_path "Data\SORTED.bin"

const int maximum_of_terms = 200000;