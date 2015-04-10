#pragma once

#include "dirent.h"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>

#include <fstream>
#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <map>
#include <functional>
#include <bitset>
#include <string>
using namespace std;

#include "pugixml.hpp"
#include "pugiconfig.hpp"
using namespace pugi;

#define sqr(x) (x) * (x)

#define SW_path "Data\\stopwords_en.txt"
#define RAW_path "Data\\RAW.bin"
#define DICT_path "Data\\DICT.txt"
#define INDEX_path "Data\\INDEX.bin"
#define SORTED_path "Data\\SORTED.bin"
#define QUERIES_path "Data\\QUERIES.txt"
#define CATEGORY_path "Data\\CATEGORY.txt"
#define LISTFILES_path "Data\\LISTFILES.txt"

#define TREC_directory "Data\\t9.filtering\\ohsu-trec"
#define REUTERS_directory "Data\\reuters21578"
#define TWENTYGRPS_directory "Data\\20_newsgroups"


const int maximum_of_terms = 400000;
const double eps = 1e-8;