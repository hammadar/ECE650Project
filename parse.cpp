#include <iostream>
#include <sstream>
#include <array>
#include <vector>
#include <algorithm>
#include <utility>

#include "parse.hpp"

int parse_define_vertices(std::string params) {
    int vertices = 0;

    vertices = std::stoi(params);

    return vertices;
}

std::vector<std::pair<int, int>> parse_define_edges(std::string params) {

    std::array<char, 5> to_remove = {'{', '}', '<', '>', ' '};
    std::vector<std::pair<int,int>> in_edges = {};
    
    for(std::array<char, 4>::size_type i = 0; i < to_remove.size(); i++)
        params.erase(std::remove(params.begin(), params.end(), to_remove[i]), params.end());
                
    std::istringstream tmp(params);
    std::vector<int> vertex_sequence = {};
    for(std::string vertex; std::getline(tmp, vertex, ','); vertex_sequence.push_back(std::stoi(vertex)));

    for(std::vector<int>::size_type i = 0; i < vertex_sequence.size(); i += 2) {
        in_edges.push_back(std::make_pair(vertex_sequence[i], vertex_sequence[i+1]));
    }

    return in_edges;
}

std::pair<int, int> parse_query_shortest_path(std::string params) {

    int pos = params.find(' ');
    
    int v1 = std::stoi(params.substr(0, pos));
    int v2 = std::stoi(params.substr(pos + 1, params.length() - pos));

    std::pair<int, int> vv(v1, v2);
    return vv;
}