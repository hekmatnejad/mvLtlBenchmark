/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: mamad
 *
 * Created on January 17, 2017, 2:39 PM
 */

#include <cstdlib>
#include <iostream>
#include "MvLtlModel.h"
#include "Util.h"
#include "ModelGenerator.h"
#include "KripkeModel.h"
#include <random>

//#include "mvtwaproduct.h"

#include <mutex>
#include <iomanip>

//#include <spot/twa/formula2bdd.hh>
#include <spot/mv/version.hh>
#include <spot/taalgos/dot.hh>
#include <spot/tl/dot.hh>
#include <spot/twaalgos/word.hh>
#include <spot/ta/taproduct.hh>
#include <spot/twa/taatgba.hh>
#include <valarray>
#include <spot/ta/tgtaproduct.hh>
#include <spot/twaalgos/product.hh>
//#include <spot/tl/dot.hh>
//#include <spot/taalgos/dot.hh>
//#include "secondary.h"
//#include "mv_interval.h"
using namespace std;

#define PRINT_DEBUG_DATA 1
float CERTAINTY_THREASHOLD = 1;

void model_4(string formula);
void dfs_twa_graph(spot::const_twa_graph_ptr aut, bdd query);
void dfs_twa(spot::const_twa_ptr aut);
//void dfs_twa(spot::twa* aut);
spot::twa* my_twa;
int get_random(int low, int high);
int get_weighted_random();
static spot::parsed_aut_ptr read_model_aut;
static spot::kripke_graph_ptr kg_model;
static spot::twa_graph_ptr aut_model;

std::map<int, geo_pos*>* geo_locations;  
std::map<int, int>* rand_intervals;
string benchmark_name = "benchmark/benchmark_model_";
int MAX_GEO_X = 16;
int MAX_GEO_Y = 16;
float MAX_GEO_DIST;
int MAX_INTERVAL_NUM = 6;
int NUM_INTERVALS = 2;
void test();

///spot::twa_graph_ptr shared_formula_graph;

/*
 * 
 */
int main(int argc, char** argv) {
    
    std::cout << "started...\n";
    cout << mvspot::getVersion() << "\n" << mvspot::getBuild() << "\n";

    mvspot::test_intervals();
    benchmark_name += std::to_string(MAX_GEO_X)+"_"+std::to_string(MAX_GEO_Y);
    string model_filename = benchmark_name+".hoa";
    srand(time(NULL));
    read_model_aut = Util::readAutFromFile(model_filename, false, shared_dict);
    if (!read_model_aut || read_model_aut->errors.size() > 0) {
        cout << "could not read the model from file!"<<model_filename<<endl;
        exit(0);
    } else {
        cout << "model loaded from: " << model_filename << endl;
        Util::write2File(benchmark_name+"_graph.dot", read_model_aut->aut, "k");

    }
    kg_model = read_model_aut->ks;
    aut_model = read_model_aut->aut;
    
    MAX_GEO_DIST = std::sqrt(MAX_GEO_X*MAX_GEO_X + MAX_GEO_Y*MAX_GEO_Y);
    std::cout << "MAX_GEO_DIST: " << MAX_GEO_DIST  << endl;
    geo_locations = new std::map<int, geo_pos*>();
    rand_intervals = new std::map<int, int>();
    ifstream in_file;
    in_file.open(benchmark_name+".txt");
    cout << "read geometric locations from "+benchmark_name+".txt\n";
    int s,x,y;
    
    //----------- Calculating the geometric locations and Random certainty generator
  std::default_random_engine generator;
  std::discrete_distribution<int> distribution {0,1,2,3,15,30};
    while(in_file >> s >> x >> y){
        cout << s <<" : " << x <<" , "<< y << endl;
        (*geo_locations)[s] = new geo_pos(x,y);
        //(*rand_intervals)[s] = get_random(MAX_INTERVAL_NUM-NUM_INTERVALS,MAX_INTERVAL_NUM);
        (*rand_intervals)[s] = distribution(generator);
        cout << "random index: " <<(*rand_intervals)[s] << endl;
    }

  model_4("");

    cout << "done!\n";
    return 0;
}

int get_random(int low, int high) {
    return (int) (low + (float) rand() / RAND_MAX * (high - low));
}

void dfs_twa_graph(spot::const_twa_graph_ptr aut, bdd query) {
    std::vector<bool> seen(aut->num_states());
    std::stack<unsigned> todo; // Now storing edges numbers
    auto& gr = aut->get_graph();
    auto push_state = [&](unsigned state) {
        todo.push(gr.state_storage(state).succ);
        seen[state] = true;
    };
    push_state(aut->get_init_state_number());
    while (!todo.empty()) {
        unsigned edge = todo.top();
        todo.pop();
        if (edge == 0U) // No more outgoing edge
            continue;
        auto& e = gr.edge_storage(edge);
        //bdd res = query & e.cond;
        //if(res==bddfalse){
        //gr.out_iteraser(edge).erase();

        //gr.remove_dead_edges_(); 
        // }
        //else
        {
            todo.push(e.next_succ); // Prepare next sibling edge.
            if (!seen[e.dst])
                push_state(e.dst);
            std::cout << e.src << "->" << e.dst << '\n';
        }
    }
}

void dfs_twa(spot::const_twa_ptr aut)
//void dfs_twa(spot::twa* aut)
{
  spot::state_unicity_table seen;
  std::stack<std::pair<const spot::state*,
                       spot::twa_succ_iterator*>> todo;

  // push receives a newly-allocated state and immediately store it in
  // seen.  Therefore any state on todo is already in seen and does
  // not need to be destroyed.
  auto push = [&](const spot::state* s)
    {
       if (seen.is_new(s))
         {
           spot::twa_succ_iterator* it = aut->succ_iter(s);
           if (it->first())
             todo.emplace(s, it);
           else                 // No successor for s
             aut->release_iter(it);
         }
    };
  push(aut->get_init_state());
  while (!todo.empty())
    {
       const spot::state* src = todo.top().first;
       spot::twa_succ_iterator* srcit = todo.top().second;
       const spot::state* dst = srcit->dst();
       std::cout << aut->format_state(src) << "->"
                 << aut->format_state(dst) << '\n';
       //aut->cond(src) = q_bdd;
       // Advance the iterator, and maybe release it.
       
       if (!srcit->next())
         {
//            spot::twa_succ_iterator* it = aut->succ_iter(src);
//            int c = 0;
//            if(it->first())
//                c++;
//            while(it->next())
//                c++;
//            cout << "num edges: " << c << endl;
            aut->release_iter(srcit);
            todo.pop();
         }
       push(dst);
    }
}

void model_4(string formula) {
    cout << ">>> in model_4\n";

    if(NUM_CARS < 1)
    {
        std::cout << "*** NUM_CARS must be greater than 0.\n";
        exit(0);
    }
    //****************//
    CERTAINTY_THREASHOLD = 0.99;
    unsigned* init_state;
    init_state = new unsigned[NUM_CARS];
    std::fill_n(init_state,NUM_CARS,0);
    //init_state[0] = 5;
    init_state[0] = 10;
    if(NUM_CARS > 1)
    //init_state[1] = 6;
    init_state[1] = 3;
    init_state[2] = 7;
    init_state[3] = 1;
    string** str_loc;
    str_loc = new string*[NUM_CARS];
    str_loc[0] = new string[2];
    str_loc[0][0] = "C1_loc_1";
    str_loc[0][1] = "C1_loc_9";
    if(NUM_CARS > 1){
    str_loc[1] = new string[2];
    str_loc[1][0] = "C2_loc_4";
    str_loc[1][1] = "C2_loc_12";
//    str_loc[2] = new string[1];
//    str_loc[2][0] = "C3_loc_5";
//    str_loc[3] = new string[1];
//    str_loc[3][0] = "C4_loc_0";
    }
    list<string>* lst_loc;
    lst_loc = new list<string>[NUM_CARS];
    lst_loc[0].push_back(str_loc[0][0]);
    lst_loc[0].push_back(str_loc[0][1]);
    if(NUM_CARS > 1){
    lst_loc[1].push_back(str_loc[1][0]);
    lst_loc[1].push_back(str_loc[1][1]);
    }

    //for(int i=2; i<NUM_CARS; i++)
    //    lst_loc[i].push_back(str_loc[i][0]);
    //****************//
    stringstream stream;
    stream << fixed << setprecision(2) << CERTAINTY_THREASHOLD;
    string str_threshold = stream.str();
    string str_certainty_ap = "q > " + str_threshold;

//---------------------------------------------------------------

    formula = "G(\"q=[0.5,1]\") & F(C1_loc_1) & F(C1_loc_9) & ((!C1_loc_1) U C1_loc_9) "
            " & G(!C1_loc_1 | !C1_loc_9) ";
            //" & G(C1_loc_9 -> XG(\"q=[1,1]\"))";
            ////" & G(C1_loc_9 -> GF(\"q=[1,1]\"))";
    formula += " & F(C2_loc_4) & F(C2_loc_12) & ((!C2_loc_12) U C2_loc_4) "
            " & G(!C2_loc_4 | !C2_loc_12)"
            //" & G(C1_loc_4 -> XG(\"q=[1,1]\"))";
            "";
            
    //formula += " & FG(C3_loc_5 & C4_loc_0)";


    if(COLLISION_AVOIDANCE)
        formula += " & G " + collision_symbol;


    cout << ">>> Formula: " << formula << endl;

    spot::parsed_formula pf = spot::parse_infix_psl(formula); //"FG(goal) & G \"c > 0.2\" "
    if (pf.format_errors(std::cerr)) {
        cout << "the formula has error!\n";
        return;
    }
    
    // Translate its negation.
    //spot::formula f = spot::formula::Not(pf.f);
    spot::formula f = pf.f;
    spot::twa_graph_ptr af = spot::translator(shared_dict).run(f);
    //Util::write2File("new_formula_org.dot", af);
    
    //dfs_twa_graph(af,bddtrue);
    //cout <<".............\n";    
    //update intervals on the edges
    mvspot::interval_bdd::simplify_interval_formula_twa(af);
    
    if(true){
    Util::write2File("new_formula.dot", af);
    af->merge_edges();
    af->merge_univ_dests();
    af->purge_dead_states();
    af->purge_unreachable_states();
    }
    shared_formula_graph = af;//***********
    //dfs_twa_graph(af,bddtrue);
    
    
//    mvspot::mv_interval* shared_intervals = mvspot::create_interval_set("certainty", "q", 2);
//    shared_intervals->add_interval("q=[1,1]",1,1);
//    shared_intervals->add_interval("q=[0.5,1]",0.5,1);

    mvspot::mv_interval* shared_intervals = mvspot::create_interval_set("certainty", "q", 5);
    shared_intervals->add_interval("q=[1,1]",1,1);
    shared_intervals->add_interval("q=[0.5,1]",0.5,1);
    shared_intervals->add_interval("q=[0.5,0.5]",0.5,0.5);
    shared_intervals->add_interval("q=[0,0.5]",0,0.5);
    shared_intervals->add_interval("q=[0,0]",0,0);
    shared_intervals->add_interval("q=[0,1]",0,1);

    if(false){
    cout << "\n\nusing TO Lattice:\n" << *shared_intervals->getTo_lattice_() << endl<<endl;
    for(std::pair<string,mvspot::mv_interval*> it : *shared_intervals->getMap_intervals()){
        
        cout << "interval: " << it.first << "\n" << *(it.second->getTo_lattice_()) << endl;
    }
    }

    // Find a run of or marine_robot_kripke that intersects af.
    auto k = std::make_shared<car_robot_kripke>(shared_dict, str_certainty_ap, aut_model,
            init_state, lst_loc, shared_intervals);
   
  //dfs_twa(k);

    //shared_model_kripke = static_cast<marine_robot_kripke*>(k);
    // Convert demo_kripke into an explicit graph
    //spot::twa_graph_ptr kg = spot::make_twa_graph(k, spot::twa::prop_set::all());
    //Util::write2File("merged_model.dot", kg);

    if(false){
    cout << "accepting condition <model>: " << k->acc() << " and formulas:\n";
    for(spot::formula f:  k->ap())
        cout << f <<endl;
    }
    //spot::acc_cond acc = af->acc();
    //af->acc() = spot::acc_cond::fin(acc.all_sets());
    if(false){
    cout << "accepting condition <formula>: " << af->acc()<< " and formulas:\n";
    for(spot::formula f:  af->ap())
        cout << f <<endl;
    }
    //k->mv_intersecting_run(af);
    //if(true) return;
    //auto prd = spot::otf_product(k,af);//do not forget to disable clear_todo_queue
    //Util::write2File("product.dot", prd);
    //return;
    
    if (auto run = k->intersecting_run(af))
        std::cout << "found a plan by the following run:\n" << *run;
    else
        std::cout << "no plan found.\n";

    // mvspot::mvtwaproduct mvtp;
    // mvtp.test_me_again();
}
