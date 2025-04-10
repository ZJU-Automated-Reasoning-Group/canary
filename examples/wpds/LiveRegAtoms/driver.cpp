#include "common.h"
// inclusion of WPDS.h must come before inclusion of
// RegAtomSet.hpp b/c RegAtomSet.hpp defines some macros
// that overlap with variable names in WPDS. RegAtomSet should
// be changed to use an enum.
#include "WPDS.h"
#include "RegAtomSet.hpp"
#include "GenKillTransformer_T.h"

using namespace wpds;

int main( int argc, char **argv )
{
  Semiring< GenKillTransformer_T< RegAtomSet > > s(GenKillTransformer_T< RegAtomSet >::one());
  CA< GenKillTransformer_T< RegAtomSet > > ca(s);
  ca.set_query(Query::poststar());
  wpds_key_t cfg_exit_node = str2key("accepting_state");
  ca.add_final_state(cfg_exit_node);

  wpds_key_t node1 = str2key("node1");
  wpds_key_t node2 = str2key("node2");
  wpds_key_t node3 = str2key("node3");
  wpds_key_t node4 = str2key("node4");
  wpds_key_t node5 = str2key("node5");
  wpds_key_t node6 = str2key("node6");

  wpds_key_t e12 = str2key("e12");
  wpds_key_t e23 = str2key("e23");
  wpds_key_t e34 = str2key("e34");
  wpds_key_t e45 = str2key("e45");
  wpds_key_t e56 = str2key("e56");
  wpds_key_t e6exit = str2key("e6exit");

  std::cout << "**************************************************\n";
  std::cout << "  Test:\n";
  GenKillTransformer_T< RegAtomSet >* l1 = GenKillTransformer_T< RegAtomSet >::makeGenKillTransformer_T(eax,cx);
  GenKillTransformer_T< RegAtomSet >* l2 = GenKillTransformer_T< RegAtomSet >::one();
  GenKillTransformer_T< RegAtomSet >* l1xl2 = l1->extend(l2);
  GenKillTransformer_T< RegAtomSet >::one()->print( std::cout << "ONE:\t" ) << std::endl;
  l1->print( std::cout << "L1:\t" ) << std::endl;
  l1xl2->print( std::cout << "L1 x ONE:\t" ) << std::endl;
  if( l1xl2->equal( GenKillTransformer_T< RegAtomSet >::one() ) ) {
      std::cout << "ERROR\n";
  }
  RegAtomSet eaxRAS( eax );
  eaxRAS.print( std::cout << "eax:\t" ) << std::endl;
  RegAtomSet cxRAS( cx );
  RegAtomSet::Diff( eaxRAS,cxRAS ).print( std::cout << "Diff(eax,cx):\t" ) << std::endl;

  std::cout << "**************************************************\n";
  ca.add(node1,e12, node2, GenKillTransformer_T< RegAtomSet >::makeGenKillTransformer_T(eax,RegAtomSet::EmptySet()));
  ca.add(node2,e23, node3, GenKillTransformer_T< RegAtomSet >::makeGenKillTransformer_T(ax,cx));
  ca.add(node3,e34, node4, GenKillTransformer_T< RegAtomSet >::makeGenKillTransformer_T(bx,ax));
  ca.add(node4,e45, node5, GenKillTransformer_T< RegAtomSet >::makeGenKillTransformer_T(dh,al));
  ca.add(node5,e56, node6, GenKillTransformer_T< RegAtomSet >::makeGenKillTransformer_T(eax,RegAtomSet::EmptySet()) );
  ca.add(node6,e6exit, cfg_exit_node, GenKillTransformer_T< RegAtomSet >::one());

  // Solve the set of equations. The weights on the state's of the CA will
  // contain the transformers of the fixpoint solution.
  ca.path_summary();
  std::set< wpds_key_t >::const_iterator it = ca.states().begin();
  std::set< wpds_key_t >::const_iterator itEND = ca.states().end();
  for( ; it != itEND ; it++) {
      std::cout << "\t";
      printkey(*it,std::cout) << ": ";
      ca.state_weight(*it)->print( std::cout ) << std::endl;
  }

  return 0;
}

/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
 */
