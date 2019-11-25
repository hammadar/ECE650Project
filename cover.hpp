
#ifndef _COVER_HPP
#define _COVER_HPP

#include <vector>

#include "graph.hpp"

class VCSolver {
private:

    public:
	VCSolver();
   ~VCSolver();

   std::pair<bool, std::vector<int>> vc_cnf_sat(Graph g, int k);
   std::pair<bool, std::vector<int>> vc_approx_1(Graph g);
   std::pair<bool, std::vector<int>> vc_approx_2(Graph g);
};

#endif
