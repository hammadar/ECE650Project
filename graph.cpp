
#include <assert.h>

#include <stdexcept>
#include <algorithm>
#include <map>
#include <queue>
#include <iostream>
#include <memory>
#include <utility>

#include "graph.hpp"

Graph::Graph() {
    this->vertices = std::vector<int>{};
    this->m = nullptr;
    this->init_complete = false;
}

Graph::Graph(int vertices): Graph() {

    this->set_vertices(vertices);
}

Graph::Graph(const Graph& g): Graph(g.vertices.size()) {
    for(size_t x = 0; x < g.vertices.size(); x++)
        for(size_t y = 0; y < g.vertices.size(); y++)
            this->m[x][y] = g.m[x][y];
}

Graph::~Graph() {
    for(size_t i = 0; i < vertices.size(); i++)
        delete[] m[i];
    delete[] m;
}

Graph& Graph::operator=(const Graph& g) {
    this->vertices = g.vertices;
    this->init_complete = g.init_complete;
    
    this->m = new int *[g.vertices.size()];
    for(size_t i = 0; i < g.vertices.size(); i++)
        this->m[i] = new int[g.vertices.size()];

    for(size_t x = 0; x < g.vertices.size(); x++)
        for(size_t y = 0; y < g.vertices.size(); y++)
            this->m[x][y] = g.m[x][y];

    return *this;
}

void Graph::set_vertices(int vertices) {
    
    if(m) {
        for(size_t i = 0; i < this->vertices.size(); i++)
            delete[] m[i];
        delete[] m;
    }
    
    this->vertices.clear();
    for(int i = 0; i < vertices; i++)
        this->vertices.push_back(i);

    m = new int*[vertices];
    for(int i = 0; i < vertices; i++)
        m[i] = new int[vertices];

    for(int x = 0; x < vertices; x++)
        for(int y = 0; y < vertices; y++)
            m[x][y] = 0;

    this->init_complete = false;
}

void Graph::set_edges(std::vector<std::pair<int, int>>& edges) {
    
    for(auto e: edges) {

        int v1 = e.first;
        int v2 = e.second;
        if(m[v1][v2] != 1 && m[v2][v1] != 1) {

            m[v1][v2] = 1;
            m[v2][v1] = 1;
        }
    }
    
    this->init_complete = true;
}

std::vector<int> Graph::get_vertices() {
    
    return this->vertices;
}

std::vector<std::pair<int, int>> Graph::get_ranked_vertices() {
   
    std::vector<std::pair<int, int>> rvert = {};

    for(size_t v1 = 0; v1 < this->vertices.size(); v1++) {
        assert(this->m[v1][v1] == 0);

        int rank = 0;
        for(size_t v2 = 0; v2 < this->vertices.size(); v2++) {
            if(this->m[v1][v2] == 1)
                rank++;
        }

        rvert.push_back(std::make_pair(v1, rank));
    }

    std::sort(rvert.begin(), rvert.end(), [](const std::pair<int, int>& p1, const std::pair<int, int>& p2) { return p1.second > p2.second; });
    return rvert;
}

std::vector<std::pair<int, int>> Graph::get_edges() {
    
    std::vector<std::pair<int, int>> edges = {};

    for(size_t v1 = 0; v1 < this->vertices.size(); v1++)
        for(size_t v2 = v1 + 1; v2 < this->vertices.size(); v2++) {
            if(this->m[v1][v2] == 1)
                edges.push_back(std::make_pair(v1, v2));
        }

    return edges;
}

void Graph::remove_edge(const std::pair<int, int>& edge) {
   assert(edge.first >= 0 && edge.first < (int)this->vertices.size());
   assert(edge.second >= 0 && edge.second < (int)this->vertices.size());
   assert(edge.first != edge.second);

   this->m[edge.first][edge.second] = 0;
   this->m[edge.second][edge.first] = 0;
}

std::vector<int> Graph::get_path(int v1, int v2) const {
    
    std::queue<int> q;
    int pi[this->vertices.size()];

    for(size_t x = 0; x < this->vertices.size(); x++)
        pi[x] = -1;

    pi[v1] = v1;
    q.push(v1);
    while(q.size() > 0) {
        
        int u = q.front();
        q.pop();

        for(size_t v = 0; v < this->vertices.size(); v++) {
            
            // skip neighbor vertex if it has been processed
            if (this->m[u][v] != 1 || pi[v] != -1)
                continue;
      
            pi[v] = u;
            q.push(v);
        }
    }

    int v = v2;
    std::vector<int> path;
    while(pi[v] != -1 && v != pi[v]) {
        path.push_back((int)v);
        v = pi[v];

        if(pi[v] == v)
            path.push_back(v);
    }
   
    std::reverse(path.begin(), path.end());
    return path;
}
