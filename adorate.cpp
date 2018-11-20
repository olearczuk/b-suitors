#include <iostream>
#include <set>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <map>
#include <chrono>

#include "blimit.hpp"

using namespace std;

//global variables
int threads_number;
unsigned int method;
vector<int> old_id;
map<int, int> old_new_id;
atomic<int> index{0};
vector<int> *Q = new vector<int>();
vector<int> *R = new vector<int>();
atomic_int *adorating_count;
bool* is_in_r; //ensures that indexes in R are unique
atomic_flag r_flag = ATOMIC_FLAG_INIT; //for vector R
atomic<int>* s_flag; //for operations on S[x]

struct cmp_dec {
    bool operator() (const pair<int, int> &p1,
                     const pair<int, int> &p2) const {
                         if(p1.second == p2.second)
                             return old_id[p1.first] > old_id[p2.first];
                         return p1.second > p2.second;
    }
};

struct cmp_inc {
    bool operator() (const pair<int, int> &p1,
                     const pair<int, int> &p2) const {
                         if(p1.second == p2.second)
                             return old_id[p1.first] < old_id[p2.first];
                         return p1.second < p2.second;
    }
};

struct vertex {
    //sets of {id, weight} sorted in descending order
    set<pair<int, int>, cmp_inc> S;
    set<pair<int, int>, cmp_dec> N;
    set<pair<int, int>>::iterator it;
};

vector<vertex> vertexes;

void s_lock(int id) {
    int aux = 0;
    while(aux == 0) {
        aux = 1;
        s_flag[id].compare_exchange_weak(aux, 0);
    }
}

void s_unlock(int id) {
    s_flag[id] = 1;
}

// {id, weight} of weakest adorator
pair<int, int> requirements(int id) {
    if(vertexes[id].S.size() < bvalue(method, old_id[id]))
        return {0, 0};
    return *vertexes[id].S.begin();
}

//Checks if vertex accepts adoration from {index, weight}
bool can_adore(int x, pair<int, int> p) {
    if(bvalue(method, old_id[x]) == 0)
        return false;
    s_lock(x);

    pair<int, int> req = requirements(x);
    bool isnt_inside = vertexes[x].S.find(p) == vertexes[x].S.end();

    s_unlock(x);
    return isnt_inside && //first condition
           (p.second > req.second || //second condition
            (p.second == req.second && old_id[p.first] > old_id[req.first]));
}

void find_matching() {
    int w, x, y, u, i;
    bool was_found;
    while(true) {
        i = index.fetch_add(1);
        if(i >= Q->size())
            return;
        int u = Q->at(i);
        unsigned int bval = bvalue(method, old_id[u]);
        while(adorating_count[u] < bval) {
            was_found = false;
            //finding candidate to adore
            while(vertexes[u].it != vertexes[u].N.end()) {
                x = vertexes[u].it->first;
                w = vertexes[u].it->second;
                was_found = can_adore(x, {u, w});
                ++vertexes[u].it;
                if(was_found == true)
                    break;
            }
            if(was_found == false)
                break;
            else {
                s_lock(x);
				pair<int, int> req = requirements(x);
                if(w > req.second || (w == req.second && old_id[u] > old_id[req.first])) {
                    adorating_count[u]++;
                    pair<int, int> req = requirements(x);

                    vertexes[x].S.insert({u, w});
                    if(req.second != 0) {
                        y = req.first;
                        adorating_count[y]--;

                        vertexes[x].S.erase(vertexes[x].S.begin());


                        s_unlock(x);
                        while(r_flag.test_and_set(memory_order_acquire)) {;}
                        if(is_in_r[y] == false) {
                            R->push_back(y);
                            is_in_r[y] = true;
                        }
                        r_flag.clear(memory_order_release);
                    } else
                        s_unlock(x);
                }
                else
                    s_unlock(x);
            }
        }
    }
}

atomic<unsigned long> total_sum{0};
void partial_result() {
    int u, i;
    while(true) {
        i = index.fetch_add(1);
        if(i >= Q->size())
            return;
        u = Q->at(i);
        for(auto it = vertexes[u].S.begin(); it != vertexes[u].S.end(); ++it) {
            total_sum += it->second;
        }
    }
}
thread* t;
unsigned int produce_result() {
    total_sum = 0;
    index = 0;
    for(int i = 0; i < vertexes.size(); i++)
        Q->push_back(i);

    // thread t[threads_number];
    for(int i = 0; i < threads_number; i++)
        t[i] = thread{partial_result};
    partial_result();
    for(int i = 0; i < threads_number; i++)
        t[i].join();
    Q->clear();
    return total_sum/2;
}

void b_suitor(atomic_int *adorating_count) {
    //init
    for(int i = 0; i < vertexes.size(); i++) {
        vertexes[i].S.clear();
        adorating_count[i] = 0;
        vertexes[i].it = vertexes[i].N.begin();
        Q->push_back(i);
    }

    // thread t[threads_number];
    while(!Q->empty()) {
        is_in_r = new bool[vertexes.size()];
        for(int i = 0; i < vertexes.size(); i++)
            is_in_r[i] = false;
        index = 0;
        for(int i = 0; i < threads_number; i++)
            t[i] = thread{find_matching};

        find_matching();

        for(int i = 0; i < threads_number; i++)
            t[i].join();
        swap(R, Q);
        R->clear();
        delete [] is_in_r;

    }
}

//FUNCTIONS USED FOR READING FROM A FILE
void set_val(int &u, int &v, int &w, string &line) {
    int i = 0, p;
    int tab[3];
    tab[0] = tab[1] = tab[2] = 0;
    for(char pc : line) {
        if(pc == ' ')
            i++;
        else {
            p = pc;
            p -= 48;
            tab[i] = 10 * tab[i] + p;
        }
    }
    u = tab[0];
    v = tab[1];
    w = tab[2];
}

int get_new_id(int id) {
    auto found = old_new_id.find(id);
    if(found != old_new_id.end())
        return found->second;
    int new_id = vertexes.size();
    old_id.push_back(id);
    old_new_id.insert({id, new_id});
    vertex v;
    vertexes.push_back(v);
    return new_id;
}

int main(int argc, char * argv[]) {
    string file_name = argv[2], line;
    string aux = argv[3];
    int p;
    unsigned int methods = 0;
    for(char pc : aux) {
        p = pc;
        p -= 48;
        methods = 10 * methods + p;
    }
    aux= argv[1];
    for(char pc : aux) {
        p = pc;
        p -= 48;
        threads_number = 10 * threads_number + p;
    }
    threads_number--;
    ifstream file(file_name);
    int u, v, w;
    int new_u, new_v;
    while(file.is_open() && !file.eof()) {
        getline(file, line);
        if(line.size() == 0)
            continue;
        if(line[0] != '#') {
            set_val(u, v, w, line);
            new_u = get_new_id(u);
            new_v = get_new_id(v);

            vertexes[new_u].N.insert({new_v, w});
            vertexes[new_v].N.insert({new_u, w});
        }
    }
    file.close();

    adorating_count = new atomic<int>[vertexes.size()];
    s_flag = new atomic<int>[vertexes.size()];



    t = new thread[threads_number];
    for(int i = 0; i < vertexes.size(); i++)
        s_flag[i] = 1;

    auto time = chrono::high_resolution_clock::now();
    for(method = 0; method <= methods; method++) {
        b_suitor(adorating_count);
        cout << produce_result() << "\n";
    }
    cout << "time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time).count() / (double)1000000000 << "\n";
    delete Q;
    delete R;
    delete [] adorating_count;
    delete [] s_flag;
    delete [] t;
}
