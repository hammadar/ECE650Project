// Compile with c++ ece650-a5cpp -std=c++11 -o ece65 
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include <array>
#include <vector>
#include <utility>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "parse.hpp"
#include "graph.hpp"
#include "cover.hpp"

const int CNF_SAT_VC    = 0;
const int APPROX_VC_1   = 1;
const int APPROX_VC_2   = 2;
std::string ALGO[] = { "CNF-SAT-VC", "APPROX-VC-1", "APPROX-VC-2" };

void *      cnf_sat_vc_wrapper    (void *arg);
std::pair<std::vector<int>, double> cnf_sat_vc_impl(Graph& g, int k);

void *     approx_vc_1_wrapper    (void *arg);
std::pair<std::vector<int>, double> approx_vc_1_impl(Graph& g, int k);

void *     approx_vc_2_wrapper    (void *arg);
std::pair<std::vector<int>, double> approx_vc_2_impl(Graph& g, int k);

Graph                  read_in    ();
std::array<std::pair<std::vector<int>, double>, 3> process_in_parallel(const Graph& g);
void parse_arguments(int argc, char* argv[], bool&);

int main(int argc, char** argv) {

    bool benchmark_mode = false;
    parse_arguments(argc, argv, benchmark_mode);
    
    while(!std::cin.eof()) {
        
        Graph input = read_in();
        std::array<std::pair<std::vector<int>, double>, 3> output = process_in_parallel(input);
        
        for(size_t i = 0; i < 3; i++) {
            if(benchmark_mode) {
                std::cout << std::fixed << std::setprecision(2) << output[i].second;
                
                if(i < 2)
                    std::cout << ",";
                if(i == 2)
                    std::cout << std::endl;
            } else {
                std::cout << ALGO[i] << ": ";
                std::sort(output[i].first.begin(), output[i].first.end());
                for(size_t j = 0; j < output[i].first.size(); j++) {
                    
                    std::cout << output[i].first[j];

                    if (j < output[i].first.size() - 1)
                        std::cout << ",";
                }
                std::cout << std::endl;
            }
        }
    }
}

void parse_arguments(int argc, char* argv[], bool& benchmark_mode) {
    char opt;
    while((opt = getopt(argc, argv, "bo:")) != -1) {
        switch(opt) {
            case 'b':
                benchmark_mode = true;
                break;
            default:
                break;
        }
    }
}

Graph read_in () {

    Graph g;
    std::string line;
    while(!g.initialized() && std::getline(std::cin, line)) {

        if(line.length() <= 0) {
            std::cerr << "Error: no command entered." << std::endl;
            continue;
        }

        if(line.length() < 3) {
            std::cerr << "Error: incorrect command." << std::endl;
            continue;
        }

        int cs = 1;
        std::string cmd = line.substr(0, 1);
        std::string params = line.substr(2, line.length() - 1);
        
        switch(cmd[0]) {
            case 'V' : {
                            int vertices = parse_define_vertices(params);
                            if(vertices < 0) {
                                std::cerr << "Error: must define graph with at least one vertex." << std::endl;
                                continue;
                            }

                            g.set_vertices(vertices);
                    
                            cs = 1;
                            break;
                       }
            case 'E' : {
                            if (cs > 1) {
                                std::cerr << "Error: cannot redefine the graph edges." << std::endl;
                                continue;
                            }

                            std::vector<std::pair<int, int>> edges = parse_define_edges(params);
                            for(auto const e: edges) {
                                if(e.first < 0 || e.first >= g.vs() || e.second < 0 || e.second >= g.vs()) {
                                    std::cerr << "Error: invalid edge (" << e.first << ", " << e.second << "). " 
                                              << "Vertex does not exist."
                                              << std::endl;
                                    continue;
                                }

                                if(e.first == e.second) {
                                    std::cerr << "Error: invalid edge (" << e.first << ", " << e.second << "). " 
                                              << "Self-loops are not allowed."
                                              << std::endl;
                                    continue;
                                }
                            }

                            g.set_edges(edges);
                            cs = 2;
                            
                            break;
                       }
            default: 
                std::cerr << "Error: command unknown." << std::endl;
        }
    }

    return g;
}

std::array<std::pair<std::vector<int>, double>, 3> process_in_parallel(const Graph& g) {

    pthread_t  threads[3];
    void* (*thread_run[])(void*) = { cnf_sat_vc_wrapper, approx_vc_1_wrapper, approx_vc_2_wrapper };
    
    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, thread_run[i], (void *)&g);
    }

    std::array<std::pair<std::vector<int>, double>, 3> output = {};
    for (int i = 0; i < 3; i++) {
        
        void * retptr;
        pthread_join(threads[i], &retptr);
        
        std::pair<std::vector<int>, double> *resultptr = static_cast<std::pair<std::vector<int>, double>*>(retptr);
        output[i] = *resultptr;
           
        delete resultptr;
    }

    return output;
}

double tdiff(struct timespec& t2, struct timespec& t1) {
    double diff = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec)/1E9f;
    return diff;
}

std::vector<int> cnf_sat_vc_impl(Graph& g) {
    
    for(int k = 1; k <= g.vs(); k++) {
        VCSolver solver;
        auto result = solver.vc_cnf_sat(g, k);
        if(result.first)
            return result.second;
    }

    return std::vector<int>{};
}

void * cnf_sat_vc_wrapper (void *arg) {

    Graph *g = (Graph *)arg;
    std::pair<std::vector<int>, double> * output = new std::pair<std::vector<int>, double>{};
    
    clockid_t cid;
    pthread_getcpuclockid(pthread_self(), &cid);
    
    struct timespec t1, t2;
    clock_gettime(cid, &t1);

    output->first  = cnf_sat_vc_impl(*g);
    
    clock_gettime(cid, &t2);

    output->second = tdiff(t2, t1);

    return output;
}


std::vector<int> approx_vc_1_impl(Graph& g) {
    
    VCSolver s;
    std::pair<bool, std::vector<int>> result = s.vc_approx_1(g);

    if(result.first)
        return result.second;
    else
        return std::vector<int>{};
}

void * approx_vc_1_wrapper (void *arg) {

    Graph *g = (Graph *)arg;
    std::pair<std::vector<int>, double> * output = new std::pair<std::vector<int>, double>{};
    
    clockid_t cid;
    pthread_getcpuclockid(pthread_self(), &cid);
    
    struct timespec t1, t2;
    clock_gettime(cid, &t1);

    output->first  = approx_vc_1_impl(*g);
    
    clock_gettime(cid, &t2);

    output->second = tdiff(t2, t1);

    pthread_exit(output);
}


std::vector<int> approx_vc_2_impl(Graph& g) {

    VCSolver s;
    std::pair<bool, std::vector<int>> result = s.vc_approx_2(g);

    if(result.first)
        return result.second;
    else
        return std::vector<int>{};
}

void * approx_vc_2_wrapper (void *arg) {
    
    Graph *g = (Graph *)arg;
    std::pair<std::vector<int>, double> * output = new std::pair<std::vector<int>, double>{};
    
    clockid_t cid;
    pthread_getcpuclockid(pthread_self(), &cid);
    
    struct timespec t1, t2;
    clock_gettime(cid, &t1);

    output->first = approx_vc_2_impl(*g);
    
    clock_gettime(cid, &t2);
    
    output->second = tdiff(t2, t1);
    pthread_exit(output);
}

