
#ifndef _GRAPH_HPP
#define _GRAPH_HPP

#include <vector>
#include <utility>

class Graph {
    
    private:
        
        std::vector<int> vertices;
        std::vector<std::pair<int, int>> edges;
        
        bool init_complete;
        int **m;
        
    public:
        
        Graph();
        Graph(int vertices);
        Graph(const Graph& g);
        ~Graph();

        Graph& operator=(const Graph& g); 

        void set_vertices(int vertices);
        void set_edges(std::vector<std::pair<int,int>>& edges);
        void remove_edge(const std::pair<int, int>& edge);

        std::vector<int> get_path(int v1, int v2) const;

        bool initialized() const {
            return this->init_complete;
        }

        int vs() const {
            return this->vertices.size();
        }

        int es() const {
            return this->edges.size();
        }

        std::vector<int> get_vertices();
        std::vector<std::pair<int, int>> get_ranked_vertices();
        std::vector<std::pair<int, int>> get_edges();

        int ** adjmat() const {
            return this->m;
        }
};

#endif
