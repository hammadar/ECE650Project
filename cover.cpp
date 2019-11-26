
#include <algorithm>
#include <memory>
#include <vector>
#include <map>

#include "cover.hpp"
#include "minisat/core/SolverTypes.h"
#include "minisat/core/Solver.h"

VCSolver::VCSolver() {
 
}

VCSolver::~VCSolver() {

}

std::pair<bool, std::vector<int>> VCSolver::vc_approx_1(Graph g) {

    std::vector<int> cover;

    // vector of (vertex, degree) pairs ranked descendingly by degree
    std::vector<std::pair<int, int>> edges = g.get_edges();
    while(!edges.empty()) {   
        
        std::vector<std::pair<int, int>> ranked_vertices = g.get_ranked_vertices();
        std::pair<int, int> v = ranked_vertices[0];
        cover.push_back(v.first);

       for(auto const& e: edges) {
          int v1 = e.first;
          int v2 = e.second;
                
          if(v.first == v1 || v.first == v2) {
              g.remove_edge(e);
          }
        }

        edges = g.get_edges();
    };
       
    return std::make_pair(true, cover);
}

std::pair<bool, std::vector<int>> VCSolver::vc_approx_2(Graph g) {

   std::vector<int> cover;
   std::map<std::pair<int, int>, bool> edges;
   for(auto e: g.get_edges()) {
        edges[e] = false;
   }

   for(auto it = edges.begin(); it != edges.end(); ++it) {
       
       auto edge = it->first;
       if(edges[edge] == false) {

            int v1 = edge.first;
            int v2 = edge.second;

            cover.push_back(v1);
            cover.push_back(v2);

            for(auto it2 = edges.begin(); it2 != edges.end(); ++it2) {
                int v3 = it2->first.first;
                int v4 = it2->first.second;

                if(v3 == v1 || v3 == v2 || v4 == v1 || v4 == v2)
                    edges[it2->first] = true;
            }

            edges[edge] = true;
       }
   }

   return std::make_pair(true, cover);
}

std::pair<bool, std::vector<int>> VCSolver::vc_cnf_sat(Graph g, int k) {
    
    size_t N = (size_t)g.vs();
    Minisat::Lit **lit = new Minisat::Lit* [N+1];
    for(size_t i = 1; i <= N; i++)
        lit[i] = new Minisat::Lit[k+1];

    std::unique_ptr<Minisat::Solver> solver(new Minisat::Solver());
    int nclauses = 0;
    
    for(size_t i = 1; i <= N; i++) {
	    for(size_t j = 1; j <= (size_t)k; j++) {
	        lit[i][j] = Minisat::mkLit(solver->newVar());
	    }
    }

    // Clauses for: "at least one vertex is the i-th vertex in the vertex cover"
    // i in [1, k] -> (x[1][i] v x[2][i] v ... v x[n][i] 

    for(size_t i = 1; i <= (size_t)k; i++) {
        Minisat::vec<Minisat::Lit> clause;
        for(size_t m = 1; m <= N; m++)
            clause.push(lit[m][i]);

        solver->addClause(clause);
        nclauses += 1;
    }
	
    // Clauses for: "no one vertex can appear twice in a vertex cover"
    // m in [1, n], p, q in [1, k] with p < q -> ~x[m][p] v ~x[m][q]
    //
    for(size_t m = 1; m <= N; m++) {
        for(size_t p = 1; p <= (size_t)k; p++) {
            for(size_t q = 1; q <= (size_t)k; q++) {
                if(p < q) {
                    solver->addClause(~lit[m][p], ~lit[m][q]);
                    nclauses += 1;
                }
            }
        }
    }
    
    // Clauses for: "no one vertex appears in the m-th position of thvertex cover"
    // m in [1, k], p, q in [1, N], p < q -> ~x[p][m] v ~x[q][m]
    //
    for(size_t m = 1; m <= (size_t)k; m++) {
        for(size_t p = 1; p <= N; p++) {
            for(size_t q = 1; q <= N; q++) {
                if(p < q) {
                    solver->addClause(~lit[p][m], ~lit[q][m]); 
                    nclauses += 1;
                }
            }
        }
    }

    // Clauses for: "every edge is incident to at least one vertex in the vertex cover"
    // <i, j> in edges(g) -> x[i][1] v x[i][2] v ... v x[i][k] v x[j][1] v x[j][2] v ... x[j][k]
    //
    int **edges = g.adjmat();
    for(size_t i = 1; i <= N; i++) {
        for(size_t j = 1; j <= N; j++) {
            
            if(i < j && edges[i-1][j-1] == 1) {
                
                Minisat::vec<Minisat::Lit> clause;
                for(size_t m = 1; m <= (size_t)k; m++) {
                    clause.push(lit[i][m]);
                    clause.push(lit[j][m]);
                }

                solver->addClause(clause);
                nclauses += 1;
            }
        }
    }
    
    // Collect model
    //
    bool res = solver->solve();
    std::vector<int> cover;
    if(res) {
        for(size_t i = 1; i <= N; i++) {
            for(int j = 1; j <= k; j++) {
                if (Minisat::toInt(solver->modelValue(lit[i][j])) == 0) {
                    cover.push_back(i-1);
                }
            }
        }
    }

    solver.reset(new Minisat::Solver());

    for(size_t i = 1; i <= N; i++)
        delete[] lit[i];
    delete[] lit;
    
    return std::make_pair(res, cover);
}
