#include <iostream>
#include <string>
using std::string;

#include "VarSet.h"
#include "GenKillTransformer_T.h"
#include "WPDS.h"

using namespace wpds;
/*
Example program to be analyzed

int a, b, c, d;

void main() {
  a = 1;
  b = 2;
  if (...) {
    c = 3;
  }
  else {
    a = 4;
  }
  p();           // call site: main_calls_p
  c = d + b;
  return;
}

void p() {
  if (...) {
    a = c;
    a = 10;
    p();         // call site: p_calls_p1
    d = c
  }
  else if (...) {
    b = 9;
    a = a + d;
    p();         // call site: p_calls_p2
    d = b;
  }
  return;
}
*/

void print_output( const CA< GenKillTransformer_T< VarSet > >::catrans_t& goal )
{
    if (goal.get_ptr() == 0) {
        std::cout << "Goal transition NULL\n";
    }
    else {
        goal->semiring_element()->print( std::cout );
    }
    std::cout << std::endl << std::endl;
}

int main() {

    Semiring< GenKillTransformer_T< VarSet > > s(GenKillTransformer_T< VarSet >::one());
    WPDS< GenKillTransformer_T< VarSet > > wpds(s);
    CA< GenKillTransformer_T< VarSet > > ca0(s);
    std::set< wpds::wpds_key_t > R;
    std::set< wpds::wpds_key_t > B;


    //std::cout << "s.diff(one(),one()) : " << s.diff(s.one(),s.one()) << std::endl;

    /* Variables */
    wpds_key_t q = str2key("q");

    /* Nodes for procedure main */

    wpds_key_t e_main = new_str2key("e_main");B.insert(e_main);
    wpds_key_t n1 = new_str2key("n1");B.insert(n1);
    wpds_key_t n2 = new_str2key("n2");B.insert(n2);
    wpds_key_t n3 = new_str2key("n3");B.insert(n3);
    wpds_key_t n4 = new_str2key("n4");B.insert(n4);
    wpds_key_t n5 = new_str2key("n5");B.insert(n5);
    wpds_key_t n6 = new_str2key("n6");B.insert(n6);
    wpds_key_t n7 = new_str2key("n7");B.insert(n7);
    wpds_key_t n8 = new_str2key("n8");B.insert(n8);
    wpds_key_t x_main = new_str2key("x_main");B.insert(x_main);

    /* Nodes for procedure p */
    wpds_key_t e_p = new_str2key("e_p");B.insert(e_p);
    wpds_key_t n9 = new_str2key("n9");B.insert(n9);
    wpds_key_t n10 = new_str2key("n10");B.insert(n10);
    wpds_key_t n11 = new_str2key("n11");B.insert(n11);
    wpds_key_t n12 = new_str2key("n12");B.insert(n12);
    wpds_key_t n13 = new_str2key("n13");B.insert(n13);
    wpds_key_t n14 = new_str2key("n14");B.insert(n14);
    wpds_key_t n15 = new_str2key("n15");B.insert(n15);
    wpds_key_t n16 = new_str2key("n16");B.insert(n16);
    wpds_key_t n17 = new_str2key("n17");B.insert(n17);
    wpds_key_t n18 = new_str2key("n18");B.insert(n18);
    wpds_key_t n19 = new_str2key("n19");B.insert(n19);
    wpds_key_t x_p = new_str2key("x_p");B.insert(x_p);

    /* States for a configuration automaton */
    wpds_key_t accepting_state = str2key("accepting_state");
    wpds_key_t q1 = str2key("q1");
    wpds_key_t q2 = str2key("q2");
    wpds_key_t q3 = str2key("q3");
    wpds_key_t q4 = str2key("q4");

    // debugging
    ref_ptr< GenKillTransformer_T< VarSet > > rgkn(GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("c"), mkVarSet("d","b")));

    /* Transition to introduce stack bottom */
    wpds_key_t stack_bottom = str2key("stack_bottom");

    wpds.add_rule(q,stack_bottom,q,x_main,stack_bottom,GenKillTransformer_T< VarSet >::one());

    /* Transitions (edges) for procedure main */
    // pds_add_rule(pds, s, q,     x_main,    q,    n8,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("a","b","c","d"),VarSet::EmptySet()));
    wpds.add_rule(q,     x_main,    q,    n8,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(q,     n8,        q,    n7,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("c"), mkVarSet("d","b")));

    /******************* FOR DEBUGGING, I REMOVED P() *********************/
    //wpds.add_rule(q,  n7,         q,      n6,     GenKillTransformer_T< VarSet >::one());
    /******************* FOR DEBUGGING, I REMOVED P() *********************/
    wpds.add_rule(q,     n6,        q,    n4,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(q,     n4,        q,    n3,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("c"), VarSet::EmptySet()));

    wpds.add_rule(q,     n6,        q,    n5,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(q,     n5,        q,    n3,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("a"), VarSet::EmptySet()));

    wpds.add_rule(q,     n3,        q,    n2,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(q,     n2,        q,    n1,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("b"), VarSet::EmptySet()));
    wpds.add_rule(q,     n1,        q,    e_main, GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("a"), VarSet::EmptySet()));
    /* Transitions (edges) for procedure p */
    wpds.add_rule(q,     x_p,       q,     n9,    GenKillTransformer_T< VarSet >::one());

    wpds.add_rule(q,     x_p,       q,    n14,    GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(q,     n14,       q,    n13,    GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("d"), mkVarSet("c")));

    wpds.add_rule(q,     x_p,       q,    n19,    GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(q,     n19,       q,    n18,    GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("d"), mkVarSet("b")));

    wpds.add_rule(q,     n12,       q,    n11,    GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(q,     n11,       q,    n10,    GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("a"), VarSet::EmptySet()));
    wpds.add_rule(q,     n10,       q,     n9,    GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("a"), mkVarSet("c")));

    wpds.add_rule(q,     n17,       q,    n16,    GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(q,     n16,       q,    n15,    GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("a"), mkVarSet("a","d")));
    wpds.add_rule(q,     n15,       q,     n9,    GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("b"), VarSet::EmptySet()));

    wpds.add_rule(q,     n9,        q,    e_p,    GenKillTransformer_T< VarSet >::one());

    /* Transitions (linkage edges) for main-calls-p and return */
    wpds.add_rule(q,     n7,        q,    x_p,   n6, GenKillTransformer_T< VarSet >::one());

    /* Transitions (linkage edges) for p-calls-p1 and return */
    wpds.add_rule(q,     n13,       q,    x_p,  n12, GenKillTransformer_T< VarSet >::one());

    /* Transitions (linkage edges) for p-calls-p2 and return */
    wpds.add_rule(q,     n18,      q,     x_p,  n17, GenKillTransformer_T< VarSet >::one());

    /* Transitions (linkage edges) for returning from p */
    wpds.add_rule(q,     e_p,       q,     GenKillTransformer_T< VarSet >::one());

#if 0
    /* PDS transitions for "consuming" the language (n12 + n17 + n6)*  */
    /* (one set for each CFG node)                                     */
    wpds_key_t c_e_main = str2key("c_e_main");
    wpds.add_rule(c_e_main,     n12,      c_e_main,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_e_main,     n17,      c_e_main,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_e_main,      n6,      c_e_main,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     e_main,      c_e_main,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n1 = str2key("c_n1");
    wpds.add_rule(c_n1,     n12,      c_n1,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n1,     n17,      c_n1,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n1,      n6,      c_n1,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n1,      c_n1,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n2 = str2key("c_n2");
    wpds.add_rule(c_n2,     n12,      c_n2,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n2,     n17,      c_n2,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n2,      n6,      c_n2,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n2,      c_n2,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n3 = str2key("c_n3");
    wpds.add_rule(c_n3,     n12,      c_n3,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n3,     n17,      c_n3,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n3,      n6,      c_n3,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n3,      c_n3,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n4 = str2key("c_n4");
    wpds.add_rule(c_n4,     n12,      c_n4,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n4,     n17,      c_n4,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n4,      n6,      c_n4,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n4,      c_n4,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n5 = str2key("c_n5");
    wpds.add_rule(c_n5,     n12,      c_n5,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n5,     n17,      c_n5,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n5,      n6,      c_n5,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n5,      c_n5,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n6 = str2key("c_n6");
    wpds.add_rule(c_n6,     n12,      c_n6,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n6,     n17,      c_n6,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n6,      n6,      c_n6,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n6,      c_n6,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n7 = str2key("c_n7");
    wpds.add_rule(c_n7,     n12,      c_n7,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n7,     n17,      c_n7,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n7,      n6,      c_n7,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n7,      c_n7,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n8 = str2key("c_n8");
    wpds.add_rule(c_n8,     n12,      c_n8,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n8,     n17,      c_n8,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n8,      n6,      c_n8,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n8,      c_n8,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_x_main = str2key("c_x_main");
    wpds.add_rule(c_x_main,     n12,      c_x_main,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_x_main,     n17,      c_x_main,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_x_main,      n6,      c_x_main,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     x_main,      c_x_main,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_e_p = str2key("c_e_p");
    wpds.add_rule(c_e_p,     n12,      c_e_p,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_e_p,     n17,      c_e_p,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_e_p,      n6,      c_e_p,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     e_p,      c_e_p,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n9 = str2key("c_n9");
    wpds.add_rule(c_n9,     n12,      c_n9,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n9,     n17,      c_n9,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n9,      n6,      c_n9,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n9,      c_n9,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n10 = str2key("c_n10");
    wpds.add_rule(c_n10,     n12,      c_n10,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n10,     n17,      c_n10,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n10,      n6,      c_n10,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n10,      c_n10,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n11 = str2key("c_n11");
    wpds.add_rule(c_n11,     n12,      c_n11,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n11,     n17,      c_n11,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n11,      n6,      c_n11,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n11,      c_n11,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n12 = str2key("c_n12");
    wpds.add_rule(c_n12,     n12,      c_n12,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n12,     n17,      c_n12,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n12,      n6,      c_n12,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n12,      c_n12,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n13 = str2key("c_n13");
    wpds.add_rule(c_n13,     n12,      c_n13,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n13,     n17,      c_n13,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n13,      n6,      c_n13,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n13,      c_n13,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n14 = str2key("c_n14");
    wpds.add_rule(c_n14,     n12,      c_n14,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n14,     n17,      c_n14,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n14,      n6,      c_n14,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n14,      c_n14,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n15 = str2key("c_n15");
    wpds.add_rule(c_n15,     n12,      c_n15,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n15,     n17,      c_n15,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n15,      n6,      c_n15,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n15,      c_n15,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n16 = str2key("c_n16");
    wpds.add_rule(c_n16,     n12,      c_n16,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n16,     n17,      c_n16,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n16,      n6,      c_n16,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n16,      c_n16,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n17 = str2key("c_n17");
    wpds.add_rule(c_n17,     n12,      c_n17,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n17,     n17,      c_n17,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n17,      n6,      c_n17,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n17,      c_n17,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n18 = str2key("c_n18");
    wpds.add_rule(c_n18,     n12,      c_n18,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n18,     n17,      c_n18,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n18,      n6,      c_n18,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n18,      c_n18,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_n19 = str2key("c_n19");
    wpds.add_rule(c_n19,     n12,      c_n19,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n19,     n17,      c_n19,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_n19,      n6,      c_n19,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     n19,      c_n19,     GenKillTransformer_T< VarSet >::one());

    wpds_key_t c_x_p = str2key("c_x_p");
    wpds.add_rule(c_x_p,     n12,      c_x_p,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_x_p,     n17,      c_x_p,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(c_x_p,      n6,      c_x_p,     GenKillTransformer_T< VarSet >::one());
    wpds.add_rule(    q,     x_p,      c_x_p,     GenKillTransformer_T< VarSet >::one());
#endif



    /* pre* of <q, x_p (n12 + n17)* n6> */
    /*  printf("pre* of <q, x_p (n12 + n17)* n6>\n\n");
        ca0 = ca_create();
        ca_add_trans(ca0,s, q, x_p, q1, GenKillTransformer_T< VarSet >::one());
        ca_add_trans(ca0,s, q1, n12, q1, GenKillTransformer_T< VarSet >::one());
        ca_add_trans(ca0,s, q1, n17, q1, GenKillTransformer_T< VarSet >::one());
        ca_add_trans(ca0,s, q1, n6, accepting_state, GenKillTransformer_T< VarSet >::one());
        print_automaton(ca0,s,"before");
        ca0 = ca_prestar(pds,ca0,s,WITNESS_TRACE_OFF);
        print_automaton(ca0,s,"after");
        goal = ca_find(ca0,s,q,x_main,accepting_state);
        print_output(ca0, s, goal);
     */

    /* pre* of <q, x_p (n12 + n17)* n17 n6> */
    /*  printf("pre* of <q, x_p (n12 + n17)* n17 n6>\n\n");
        ca1 = ca_create();
        ca_add_trans(ca1,s, q, x_p, q1, GenKillTransformer_T< VarSet >::one());
        ca_add_trans(ca1,s, q1, n12, q1, GenKillTransformer_T< VarSet >::one());
        ca_add_trans(ca1,s, q1, n17, q1, GenKillTransformer_T< VarSet >::one());
        ca_add_trans(ca1,s, q1, n17, q2, GenKillTransformer_T< VarSet >::one());
        ca_add_trans(ca1,s, q2, n6, accepting_state, GenKillTransformer_T< VarSet >::one());
        print_automaton(ca1,s,"before");
        ca1 = ca_prestar(pds,ca1,s,WITNESS_TRACE_OFF);
        print_automaton(ca1,s,"after");
        goal = ca_find(ca1,s,q,x_main,accepting_state);
        print_output(ca1, s, goal);
     */

    /* post* of <q, stack_bottom> */

    std::cout << "WPDS... \n" << wpds << "\n-------------------------\n";
    printf("post* of <q, stack_bottom>\n\n");
    CA< GenKillTransformer_T< VarSet > > ca2(s);
    ca2.add(q, stack_bottom, accepting_state, GenKillTransformer_T< VarSet >::one());
    ca2.add_final_state( accepting_state );
    std::cout << "Before ...\n" << ca2 << std::endl;

    CA< GenKillTransformer_T< VarSet > > ca3 = poststar< GenKillTransformer_T< VarSet > >(wpds,ca2,s);
    std::cout << "After ...\n" << ca3 << std::endl;

    /* PDS transitions for "consuming" the language (n12 + n17 + n6)*  */
    R.insert( n6 );
    R.insert( n12 );
    R.insert( n17 );
    R.insert( stack_bottom );
    std::cout << "\n\n--- Calling path_summary ---\n";
#if 1
    ca3.path_summary( R );
    std::cout << "--- Done path_summary ---\n";

    // for each b element of B, print (q,b,q') and its weight
    for(std::set< wpds::wpds_key_t >::iterator iter = B.begin();
            iter != B.end(); iter++ ) {
        wpds::wpds_key_t b = *iter;
        //wpds::wpds_key_t b = n18;
        wpds::CA< GenKillTransformer_T< VarSet > >::TransSetIterPair pair = ca3.match( q,b );
        wpds::CA< GenKillTransformer_T< VarSet > >::TransSetIter titer = pair.first;
        wpds::Semiring< GenKillTransformer_T< VarSet > >::SemiringElement V_b(s.zero());
        //std::cout << "BEGINNING " << key2str(b) << "---------------\n";
        for( ; titer != pair.second; titer++ ) {
            wpds::CA< GenKillTransformer_T< VarSet > >::catrans_t t(*titer);

            //std::cout << "extending:\n\t" << ca3.state_weight(t->to_state()) << "\n\t" << t->semiring_element() << std::endl;

            wpds::Semiring< GenKillTransformer_T< VarSet > >::SemiringElement tmp(
                    s.extend( ca3.state_weight(t->to_state()),t->semiring_element() ) );

            //std::cout << "combining:\n\t" << V_b << "\n\t" << tmp << std::endl;
            V_b = s.combine(V_b,tmp);
        }
        V_b->print( printkey(b, std::cout) << ": " ) << "\n\n";
        //std::cout << "ENDING " << key2str(b) << "---------------\n\n";
    }

#endif
#if 0
    CA< GenKillTransformer_T< VarSet > >::catrans_t goal = ca3.find(c_e_main,stack_bottom,accepting_state);
    cout << "e_main: ";
    print_output( goal);
    goal = ca3.find(c_n1,stack_bottom,accepting_state);
    cout << "n1: ";
    print_output( goal);
    goal = ca3.find(c_n2,stack_bottom,accepting_state);
    cout << "n2: ";
    print_output( goal);
    goal = ca3.find(c_n3,stack_bottom,accepting_state);
    cout << "n3: ";
    print_output( goal);
    goal = ca3.find(c_n4,stack_bottom,accepting_state);
    cout << "n4: ";
    print_output( goal);
    goal = ca3.find(c_n5,stack_bottom,accepting_state);
    cout << "n5: ";
    print_output( goal);
    goal = ca3.find(c_n6,stack_bottom,accepting_state);
    cout << "n6: ";
    print_output( goal);
    goal = ca3.find(c_n7,stack_bottom,accepting_state);
    cout << "n7: ";
    print_output( goal);
    goal = ca3.find(c_n8,stack_bottom,accepting_state);
    cout << "n8: ";
    print_output( goal);
    goal = ca3.find(c_x_main,stack_bottom,accepting_state);
    cout << "x_main: ";
    print_output( goal);

    goal = ca3.find(c_e_p,stack_bottom,accepting_state);
    cout << "e_p: ";
    print_output( goal);
    goal = ca3.find(c_n9,stack_bottom,accepting_state);
    cout << "n9: ";
    print_output( goal);
    goal = ca3.find(c_n10,stack_bottom,accepting_state);
    cout << "n10: ";
    print_output( goal);
    goal = ca3.find(c_n11,stack_bottom,accepting_state);
    cout << "n11: ";
    print_output( goal);
    goal = ca3.find(c_n12,stack_bottom,accepting_state);
    cout << "n12: ";
    print_output( goal);
    goal = ca3.find(c_n13,stack_bottom,accepting_state);
    cout << "n13: ";
    print_output( goal);
    goal = ca3.find(c_n14,stack_bottom,accepting_state);
    cout << "n14: ";
    print_output( goal);
    goal = ca3.find(c_n15,stack_bottom,accepting_state);
    cout << "n15: ";
    print_output( goal);
    goal = ca3.find(c_n16,stack_bottom,accepting_state);
    cout << "n16: ";
    print_output( goal);
    goal = ca3.find(c_n17,stack_bottom,accepting_state);
    cout << "n17: ";
    print_output( goal);
    goal = ca3.find(c_n18,stack_bottom,accepting_state);
    cout << "n18: ";
    print_output( goal);
    goal = ca3.find(c_n19,stack_bottom,accepting_state);
    cout << "n19: ";
    print_output( goal);
    goal = ca3.find(c_x_p,stack_bottom,accepting_state);
    cout << "x_p: ";
    print_output( goal);
#endif

    return 0;
} /** end of main **/
/* Yo, Emacs!
;;; Local Variables: ***
;;; tab-width: 4 ***
;;; End: ***
*/
