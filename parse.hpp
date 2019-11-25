
#ifndef _PARSE_HPP
#define _PARSE_HPP

#include <string>
#include <vector>
#include <utility>

int parse_define_vertices(std::string params);
std::vector<std::pair<int, int>> parse_define_edges(std::string params);
std::pair<int, int> parse_query_shortest_path(std::string params);

#endif