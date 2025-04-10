#include <iostream>
#include <string>
#include <stdio.h>
#include "stringwrapper.h"
#include <set>
#ifdef OLD_WPDS_INDICES
#   include "wpds.h"
#else
#   include "WPDS.h"
#endif

int main() {

    wpds::Semiring< StringWrapper > s(StringWrapper::one());
    wpds::WPDS< StringWrapper > wpds(s);
    wpds::CA< StringWrapper > ca(s);
    wpds::CA< StringWrapper > ca2(s);

    wpds::wpds_key_t p = str2key("p");
    wpds::wpds_key_t A = str2key("A");
    wpds::wpds_key_t A1 = str2key("A1");
    wpds::wpds_key_t foo = str2key("foo");
    wpds::wpds_key_t B = str2key("B");
    wpds::wpds_key_t C = str2key("C");
    wpds::wpds_key_t D = str2key("D");
    wpds::wpds_key_t lambda = str2key("lambda");
    wpds::wpds_key_t accept = str2key("accept");

    wpds.add_rule(p,lambda,p,A1,new StringWrapper("a"));
    wpds.add_rule(p,A1,p,foo,A,StringWrapper::one());
    wpds.add_rule(p,foo,p,StringWrapper::one());
    wpds.add_rule(p,A,p,B,new StringWrapper("b"));
    wpds.add_rule(p,A,p,C,new StringWrapper("e"));
    wpds.add_rule(p,B,p,D,new StringWrapper("c"));
    wpds.add_rule(p,C,p,D,new StringWrapper("d"));
    wpds.add_rule(p,A,p,A,B,new StringWrapper("w"));

    ca.add(p,lambda,accept,StringWrapper::one());

    std::cout << wpds << std::endl <<std::endl;
    std::cout << "*** BEFORE ***\n" << ca << std::endl;
    ca2 = wpds::poststar< StringWrapper >( wpds,ca,s );
    std::cout << "*** AFTER  ***\n" << ca2 << std::endl;



    {   // find p --D--> accept; ask for trace back.
      wpds::CA<StringWrapper>::catrans_t t;
      wpds::CAPathOfWitness<StringWrapper>::witness_path_t wp;
      wpds::DAGWitnessForPath<StringWrapper>::witness_dag_t dag;

      if( ca2.find(p,D,accept,t) )
      {
          wp = new wpds::CAPathOfWitness<StringWrapper>(t->witness(),0);
          dag = wpds::DAGWitnessForPath<StringWrapper>::createFromCAPathOfWitness(wp,Query::poststar());
          std::cout << *dag << std::endl;
      }
    }     


#if 0
    std::set< wpds::wpds_key_t > Rset;
    std::set< wpds::wpds_key_t > Bset;
    Bset.insert( A );
    Bset.insert( B );
    Bset.insert( C );
    Bset.insert( D );
    Rset.insert( B );
    Rset.insert( lambda );
    ca2.path_summary( Rset );

    // for each b element of Bset, print (p,b,q) and its weight
    for(std::set< wpds::wpds_key_t >::iterator iter = Bset.begin();
            iter != Bset.end(); iter++ ) {
        wpds::wpds_key_t b = *iter;
        //wpds::wpds_key_t b = n18;
        wpds::ConfigIterator< StringWrapper > *ci = ca2.match_f(p,b);
        wpds::Semiring< StringWrapper >::SemiringElement V_b(s.zero());
        //std::cout << "BEGINNING " << key2str(b) << "---------------\n";
        while( ci->has_more() ) {
            wpds::CA< StringWrapper >::catrans_t t(ci->next());

            //std::cout << "extending:\n\t" << ca2.state_weight(t->to_state()) << "\n\t" << t->semiring_element() << std::endl;

            wpds::Semiring< StringWrapper >::SemiringElement tmp(
                    s.extend( ca2.state_weight(t->to_state()),t->semiring_element() ) );

            //std::cout << "combining:\n\t" << V_b << "\n\t" << tmp << std::endl;
            V_b = s.combine(V_b,tmp);
        }
        printkey(b, std::cout) << ": ";
        V_b->print(std::cout) << "\n\n";
        //std::cout << "ENDING " << key2str(b) << "---------------\n\n";
    }
#endif
    return 0;
}
/* Yo, Emacs!
;;; Local Variables: ***
;;; tab-width: 4 ***
;;; End: ***
*/
