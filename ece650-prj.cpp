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

const int WATCHDOG      = 0;
const int CNF_SAT_VC    = 1;
const int APPROX_VC_1   = 2;
const int APPROX_VC_2   = 3;

struct thread_context { 
    // Context
    pthread_t             thread[4] = {};
    pthread_mutex_t         *mutex; 
    int                      count;
    // Input
    uint                   timeout;
    Graph                        g;
};

std::string ALGO[] = { "CNF-SAT-VC", "APPROX-VC-1", "APPROX-VC-2" };

void        cleanup_vc_thread(void* arg);
void *        watchdog_thread(void *arg);
void *      cnf_sat_vc_thread(void *arg);
void *     approx_vc_1_thread(void *arg);
void *     approx_vc_2_thread(void *arg);

std::pair<std::vector<int>, double>  cnf_sat_vc_impl(Graph& g, int k);
std::pair<std::vector<int>, double> approx_vc_1_impl(Graph& g, int k);
std::pair<std::vector<int>, double> approx_vc_2_impl(Graph& g, int k);

Graph read_in();
void parse_arguments(int argc, char* argv[], bool&, int& timeout);
std::array<std::pair<std::vector<int>, double>, 3> process_in_parallel(const Graph& g, int timeout);

int main(int argc, char** argv) {

    bool benchmark_mode = false;
    
    int  timeout_seconds = 120;
    parse_arguments(argc, argv, benchmark_mode, timeout_seconds);
    
    while(!std::cin.eof()) {
        
        Graph g = read_in();
        std::array<std::pair<std::vector<int>, double>, 3> output = process_in_parallel(g, timeout_seconds);
        
        for(size_t i = 0; i < 3; i++) {
            if(benchmark_mode) {
                if(output[i].second != -1)
                    std::cout << std::fixed << std::setprecision(4) << output[i].second;
                else
                    std::cout << "timeout";
                
                if(i < 2)
                    std::cout << ",";
                if(i == 2)
                    std::cout << std::endl;
            } else {
                std::cout << ALGO[i] << ": ";
                std::sort(output[i].first.begin(), output[i].first.end());
                for(size_t j = 0; j < output[i].first.size(); j++) {
                    
                    if(output[i].second >= 0)
                        std::cout << output[i].first[j];
                    else
                        std::cout << "timeout";

                    if (j < output[i].first.size() - 1)
                        std::cout << ",";
                }
                std::cout << std::endl;
            }
        }
    }
}

void parse_arguments(int argc, char* argv[], bool& benchmark_mode, int& timeout_seconds) {
    char opt;
    while((opt = getopt(argc, argv, "bo:t:")) != -1) {
        switch(opt) {
            case 'b':
                benchmark_mode = true;
                break;
            case 't':
                timeout_seconds = std::stoi(optarg);
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

std::array<std::pair<std::vector<int>, double>, 3> process_in_parallel(const Graph& g, int timeout) {

    pthread_mutex_t mutex  = PTHREAD_MUTEX_INITIALIZER;

    struct thread_context ctx;
    ctx.mutex = &mutex;
    ctx.timeout = timeout;
    ctx.g = g;
    
    void* (*thread_run[])(void*) = { watchdog_thread, cnf_sat_vc_thread, approx_vc_1_thread, approx_vc_2_thread };
    for (int i = 0; i < 4; i++) {
        pthread_create(&ctx.thread[i], NULL, thread_run[i], (void *)&ctx);
    }

    std::array<std::pair<std::vector<int>, double>, 3> output = {};
    for (int i = 0; i < 4; i++) {
        
        void * retptr;
        pthread_join(ctx.thread[i], &retptr);
        
        if(i >= 1 && retptr == PTHREAD_CANCELED) {
            output[i - 1] = std::make_pair(std::vector<int>(), -1); 
        }
        else {
            std::pair<std::vector<int>, double> *resultptr = static_cast<std::pair<std::vector<int>, double>*>(retptr);
            output[i - 1] = *resultptr;
            delete resultptr;
        }
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

void * cnf_sat_vc_thread (void *arg) {

    struct thread_context *ctx = (struct thread_context *)arg;
    std::pair<std::vector<int>, double> * output = new std::pair<std::vector<int>, double>{};
    pthread_cleanup_push(cleanup_vc_thread, output);
    
    clockid_t cid;
    struct timespec t1, t2;
    pthread_getcpuclockid(pthread_self(), &cid);
    
    clock_gettime(cid, &t1);
    output->first  = cnf_sat_vc_impl(ctx->g);
    clock_gettime(cid, &t2);
    output->second = tdiff(t2, t1);

    pthread_mutex_lock(ctx->mutex);
    ctx->count += 1;
    if(ctx->count == 3) {
        pthread_cancel(ctx->thread[WATCHDOG]);
    }
    pthread_mutex_unlock(ctx->mutex);

    pthread_cleanup_pop(0);
    pthread_exit(output);
}


std::vector<int> approx_vc_1_impl(Graph& g) {
    
    VCSolver s;
    std::pair<bool, std::vector<int>> result = s.vc_approx_1(g);

    if(result.first)
        return result.second;
    else
        return std::vector<int>{};
}

void * approx_vc_1_thread (void *arg) {

    struct thread_context *ctx = (struct thread_context*)arg;
    std::pair<std::vector<int>, double> * output = new std::pair<std::vector<int>, double>{};
    
    pthread_cleanup_push(cleanup_vc_thread, output);

    clockid_t cid;
    struct timespec t1, t2;
    pthread_getcpuclockid(pthread_self(), &cid);
    
    clock_gettime(cid, &t1);
    output->first  = approx_vc_1_impl(ctx->g);
    clock_gettime(cid, &t2);
    output->second = tdiff(t2, t1);

    pthread_mutex_lock(ctx->mutex);
    ctx->count += 1;
    if(ctx->count == 3) {
        pthread_cancel(ctx->thread[WATCHDOG]);
    }
    pthread_mutex_unlock(ctx->mutex);
    
    pthread_cleanup_pop(0);
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

void * approx_vc_2_thread (void *arg) {
    
    struct thread_context *ctx = (struct thread_context*)arg;
    std::pair<std::vector<int>, double> * output = new std::pair<std::vector<int>, double>{};
    pthread_cleanup_push(cleanup_vc_thread, output);

    clockid_t cid;
    struct timespec t1, t2;
    pthread_getcpuclockid(pthread_self(), &cid);
    
    clock_gettime(cid, &t1);
    output->first = approx_vc_2_impl(ctx->g);
    clock_gettime(cid, &t2);
    output->second = tdiff(t2, t1);

    pthread_mutex_lock(ctx->mutex);
    ctx->count += 1;
    if(ctx->count == 3) {
        pthread_cancel(ctx->thread[WATCHDOG]);
    }
    pthread_mutex_unlock(ctx->mutex);

    pthread_cleanup_pop(0);
    pthread_exit(output);
}

void * watchdog_thread (void *arg) {
    
    struct thread_context *ctx = (struct thread_context *)arg;
    sleep(ctx->timeout);

    pthread_mutex_lock(ctx->mutex);
    for(int i = 1; i < 4; i++)
        pthread_cancel(ctx->thread[i]);
    pthread_mutex_unlock(ctx->mutex);
    
    pthread_exit(NULL);
}

void cleanup_vc_thread(void *arg) {
     
    std::pair<std::vector<int>, double> *resultptr = 
        static_cast<std::pair<std::vector<int>, double>*>(arg);

    if(resultptr)
        delete resultptr;
}
