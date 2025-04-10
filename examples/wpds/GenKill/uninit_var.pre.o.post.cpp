#include <iostream>
#include "VarSet.h"
#include "GenKillTransformer_T.h"
#include "WPDS.h"

/*
Example program to be analyzed

int a, b, c, d;

void main() {
  a = 5;
  if (...) {
    b = 3;
  }
  else {
    b = 4;
  }
  p();           // call site: main_calls_p
  return;
}

void p() {
  if (...) {
    c = 8;
    d = 4;
    p();         // call site: p_calls_p1
  }
  else if (...) {
    c = 8;
    p();         // call site: p_calls_p2
    d = 4;
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

int main()
{
    wpds::Semiring< GenKillTransformer_T< VarSet > > s(GenKillTransformer_T< VarSet >::one());
    wpds::Semiring< GenKillTransformer_T< VarSet > > s_R(GenKillTransformer_T< VarSet >::one(),false);
    wpds::WPDS< GenKillTransformer_T< VarSet > > pds( s );
    wpds::WPDS< GenKillTransformer_T< VarSet > > readout( s_R );

#ifdef TRACE
    traceFile = stderr;
#endif
#ifdef REFTRACE
    refTraceFile = stderr;
#endif

    /* PDS States */
    wpds::wpds_key_t q = str2key("q");

    /* Nodes for procedure main */

    wpds::wpds_key_t e_main = str2key("e_main");
    wpds::wpds_key_t n1 = str2key("n1");
    wpds::wpds_key_t n2 = str2key("n2");
    wpds::wpds_key_t n3 = str2key("n3");
    wpds::wpds_key_t n4 = str2key("n4");
    wpds::wpds_key_t n5 = str2key("n5");
    wpds::wpds_key_t n6 = str2key("n6");
    wpds::wpds_key_t x_main = str2key("x_main");
    /* Nodes for procedure p */
    wpds::wpds_key_t e_p = str2key("e_p");
    wpds::wpds_key_t n7 = str2key("n7");
    wpds::wpds_key_t n8 = str2key("n8");
    wpds::wpds_key_t n9 = str2key("n9");
    wpds::wpds_key_t n10 = str2key("n10");
    wpds::wpds_key_t n11 = str2key("n11");
    wpds::wpds_key_t n12 = str2key("n12");
    wpds::wpds_key_t n13 = str2key("n13");
    wpds::wpds_key_t n14 = str2key("n14");
    wpds::wpds_key_t n15 = str2key("n15");
    //wpds::wpds_key_t n16 = str2key("n16");
    wpds::wpds_key_t x_p = str2key("x_p");

    /* Transitions (edges) for procedure main */
    pds.add_rule( q,     e_main,    q,    n1,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(VarSet::EmptySet(), mkVarSet("a","b","c","d")));
    pds.add_rule( q,     n1,        q,    n2,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("a"), VarSet::EmptySet()));

    pds.add_rule( q,     n2,        q,    n3,     GenKillTransformer_T< VarSet >::one());
    pds.add_rule( q,     n3,        q,    n5,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("b"), VarSet::EmptySet()));

    pds.add_rule( q,     n2,        q,    n4,     GenKillTransformer_T< VarSet >::one());
    pds.add_rule( q,     n4,        q,    n5,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("b"), VarSet::EmptySet()));

    pds.add_rule( q,     n6,        q,    x_main, GenKillTransformer_T< VarSet >::one());

    /* Transitions (edges) for procedure p */
    pds.add_rule( q,     e_p,       q,    n7,     GenKillTransformer_T< VarSet >::one());

    /* if (...) */
    pds.add_rule( q,     n7,        q,    n8,     GenKillTransformer_T< VarSet >::one());
    pds.add_rule( q,     n8,        q,    n9,     GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("c"), VarSet::EmptySet()));
    pds.add_rule( q,     n9,        q,    n10,    GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("d"), VarSet::EmptySet()));
    pds.add_rule( q,     n11,       q,    x_p,    GenKillTransformer_T< VarSet >::one());

    /* else if (...) */
    pds.add_rule( q,     n7,        q,    n12,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule( q,     n12,       q,    n13,    GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("c"), VarSet::EmptySet()));
    pds.add_rule( q,     n14,       q,    n15,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule( q,     n15,       q,    x_p,    GenKillTransformer_T< VarSet >::makeGenKillTransformer_T(mkVarSet("d"), VarSet::EmptySet()));

    /* else */
    pds.add_rule( q,     n7,        q,    x_p,    GenKillTransformer_T< VarSet >::one());

    /* Transitions (linkage edges) for main-calls-p and return */
    pds.add_rule( q,     n5,        q,    e_p,   n6, GenKillTransformer_T< VarSet >::one());

    /* Transitions (linkage edges) for p-calls-p1 and return */
    pds.add_rule( q,     n10,       q,    e_p,  n11, GenKillTransformer_T< VarSet >::one());

    /* Transitions (linkage edges) for p-calls-p2 and return */
    pds.add_rule( q,     n13,      q,     e_p,  n14, GenKillTransformer_T< VarSet >::one());

    /* Transitions (linkage edges) for returning from p */
    pds.add_rule( q,     x_p,       q,     GenKillTransformer_T< VarSet >::one());

    /* PDS transitions for "consuming" (backwards) the call graph to enclosing procedure */
    /* (one set for each CFG node)                                                       */
    wpds::wpds_key_t c0 = str2key("c0");

    pds.add_rule(     q, e_main,       c0, e_main,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(     q,     n1,       c0,     n1,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(     q,     n2,       c0,     n2,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(     q,     n3,       c0,     n3,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(     q,     n4,       c0,     n4,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(     q,     n5,       c0,     n5,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(     q,     n6,       c0,     n6,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(     q, x_main,       c0, x_main,    GenKillTransformer_T< VarSet >::one());

    pds.add_rule(      q,     e_p,      c0,     e_p,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,      n7,      c0,      n7,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,      n8,      c0,      n8,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,      n9,      c0,      n9,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,     n10,      c0,     n10,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,     n11,      c0,     n11,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,     n12,      c0,     n12,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,     n13,      c0,     n13,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,     n14,      c0,     n14,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,     n15,      c0,     n15,    GenKillTransformer_T< VarSet >::one());
    pds.add_rule(      q,     x_p,      c0,     x_p,    GenKillTransformer_T< VarSet >::one());
#if 0
#endif

    readout.add_rule(     c0,     e_p,      c0,     e_p,   n6, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     e_p,      c0,     e_p,  n11, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     e_p,      c0,     e_p,  n14, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     e_p,      c0,     e_p,    GenKillTransformer_T< VarSet >::one());

    readout.add_rule(     c0,     n7,      c0,     n7,   n6, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n7,      c0,     n7,  n11, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n7,      c0,     n7,  n14, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n7,      c0,     n7,    GenKillTransformer_T< VarSet >::one());

    readout.add_rule(     c0,     n8,      c0,     n8,   n6, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n8,      c0,     n8,  n11, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n8,      c0,     n8,  n14, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n8,      c0,     n8,    GenKillTransformer_T< VarSet >::one());

    readout.add_rule(     c0,     n9,      c0,     n9,   n6, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n9,      c0,     n9,  n11, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n9,      c0,     n9,  n14, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n9,      c0,     n9,    GenKillTransformer_T< VarSet >::one());

    readout.add_rule(     c0,     n10,      c0,     n10,   n6, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n10,      c0,     n10,  n11, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n10,      c0,     n10,  n14, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n10,      c0,     n10,  GenKillTransformer_T< VarSet >::one());

    readout.add_rule(     c0,     n11,      c0,     n11,   n6, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n11,      c0,     n11,  n11, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n11,      c0,     n11,  n14, GenKillTransformer_T< VarSet >::one());
    readout.add_rule(     c0,     n11,      c0,     n11,  GenKillTransformer_T< VarSet >::one());

    readout.add_rule(     c0,     n12,      c0,     n12,   n6, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n12,      c0,     n12,  n11, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n12,      c0,     n12,  n14, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n12,      c0,     n12, GenKillTransformer_T< VarSet >::one());

     readout.add_rule(     c0,     n13,      c0,     n13,   n6, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n13,      c0,     n13,  n11, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n13,      c0,     n13,  n14, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n13,      c0,     n13, GenKillTransformer_T< VarSet >::one());

     readout.add_rule(     c0,     n14,      c0,     n14,   n6, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n14,      c0,     n14,  n11, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n14,      c0,     n14,  n14, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n14,      c0,     n14, GenKillTransformer_T< VarSet >::one());

     readout.add_rule(     c0,     n15,      c0,     n15,   n6, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n15,      c0,     n15,  n11, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n15,      c0,     n15,  n14, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     n15,      c0,     n15, GenKillTransformer_T< VarSet >::one());

     readout.add_rule(     c0,     x_p,      c0,     x_p,   n6, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     x_p,      c0,     x_p,  n11, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     x_p,      c0,     x_p,  n14, GenKillTransformer_T< VarSet >::one());
     readout.add_rule(     c0,     x_p,      c0,     x_p, GenKillTransformer_T< VarSet >::one());


    /* States for a configuration automaton */
    wpds::wpds_key_t accepting_state = str2key("accepting_state");
    wpds::wpds_key_t q1 = str2key("q1");
    wpds::wpds_key_t q2 = str2key("q2");
    wpds::wpds_key_t q3 = str2key("q3");
    wpds::wpds_key_t q4 = str2key("q4");

    /* debug print wpds */
    std::cout << pds << "\n----------------------------------------\n";
    /* pre* o post* of <q, e_main> */
    printf("pre* o post* of <q, e_main>\n\n");
    wpds::CA< GenKillTransformer_T< VarSet > > ca3( s );
    ca3.add( q, e_main, accepting_state, GenKillTransformer_T< VarSet >::one());
    std::cout << "before\n" << ca3 << std::endl;
    wpds::CA< GenKillTransformer_T< VarSet > > ca1 = wpds::poststar< GenKillTransformer_T< VarSet > >(pds,ca3,s);
    std::cout << "middle\n" << ca1 << std::endl;

    ca1 = wpds::prestar< GenKillTransformer_T< VarSet > >( readout,ca1,s_R );
    //wpds::CA< GenKillTransformer_T< VarSet > > ca1 = wpds::prestar< GenKillTransformer_T< VarSet > >( readout,ca2,s_R );
    std::cout << "after\n" << ca1 << std::endl;

    wpds::CA< GenKillTransformer_T< VarSet > >::catrans_t goal =
        ca1.find(c0,e_main,accepting_state);
    std::cout << "e_main: ";
    print_output(goal);
    goal = ca1.find(c0,n1,accepting_state);
    std::cout << "n1: ";
    print_output(goal);
    goal = ca1.find(c0,n2,accepting_state);
    std::cout << "n2: ";
    print_output(goal);
    goal = ca1.find(c0,n3,accepting_state);
    std::cout << "n3: ";
    print_output(goal);
    goal = ca1.find(c0,n4,accepting_state);
    std::cout << "n4: ";
    print_output(goal);
    goal = ca1.find(c0,n5,accepting_state);
    std::cout << "n5: ";
    print_output(goal);
    goal = ca1.find(c0,n6,accepting_state);
    std::cout << "n6: ";
    print_output(goal);
    goal = ca1.find(c0,x_main,accepting_state);
    std::cout << "x_main: ";
    print_output(goal);

    goal = ca1.find(c0,e_p,accepting_state);
    std::cout << "e_p: ";
    print_output(goal);
    goal = ca1.find(c0,n7,accepting_state);
    std::cout << "n7: ";
    print_output(goal);
    goal = ca1.find(c0,n8,accepting_state);
    std::cout << "n8: ";
    print_output(goal);
    goal = ca1.find(c0,n9,accepting_state);
    std::cout << "n9: ";
    print_output(goal);
    goal = ca1.find(c0,n10,accepting_state);
    std::cout << "n10: ";
    print_output(goal);
    goal = ca1.find(c0,n11,accepting_state);
    std::cout << "n11: ";
    print_output(goal);
    goal = ca1.find(c0,n12,accepting_state);
    std::cout << "n12: ";
    print_output(goal);
    goal = ca1.find(c0,n13,accepting_state);
    std::cout << "n13: ";
    print_output(goal);
    goal = ca1.find(c0,n14,accepting_state);
    std::cout << "n14: ";
    print_output(goal);
    goal = ca1.find(c0,n15,accepting_state);
    std::cout << "n15: ";
    print_output(goal);
    goal = ca1.find(c0,x_p,accepting_state);
    std::cout << "x_p: ";
    print_output(goal);
#if 0
#endif

    return 0;
} /** end of main **/
