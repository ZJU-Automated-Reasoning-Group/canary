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
    wpds::wpds_key_t B = str2key("B");
    wpds::wpds_key_t C = str2key("C");
    wpds::wpds_key_t D = str2key("D");
    wpds::wpds_key_t lambda = str2key("lambda");
    wpds::wpds_key_t accept = str2key("accept");

    wpds.add_rule(p,lambda,p,A,new StringWrapper("a"));
    wpds.add_rule(p,A,p,B,new StringWrapper("b"));
    wpds.add_rule(p,B,p,C,new StringWrapper("c"));
    wpds.add_rule(p,C,p,D,new StringWrapper("d"));
    wpds.add_rule(p,A,p,A,B,new StringWrapper("w"));

    ca.add(p,lambda,accept,StringWrapper::one());

    std::cout << wpds << std::endl <<std::endl;
    std::cout << "*** BEFORE ***\n" << ca << std::endl;
    ca2 = wpds::poststar< StringWrapper >( wpds,ca,s );
    std::cout << "*** AFTER  ***\n" << ca2 << std::endl;

    std::set< wpds::wpds_key_t > Rset;
    std::set< wpds::wpds_key_t > Bset;
    Bset.insert( A );
    Bset.insert( B );
    Bset.insert( C );
    Bset.insert( D );
    Rset.insert( B );
    Rset.insert( lambda );
    ca2.add_final_state( accept );
    ca2.path_summary( Rset );

    // for each b element of Bset, print (p,b,q) and its weight
    for(std::set< wpds::wpds_key_t >::iterator iter = Bset.begin();
            iter != Bset.end(); iter++ ) {
        wpds::wpds_key_t b = *iter;
        //wpds::wpds_key_t b = n18;
        CA< StringWrapper >::TransListIterPair pair = ca2.match(p,b);
        wpds::Semiring< StringWrapper >::SemiringElement V_b(s.zero());
        //std::cout << "BEGINNING " << key2str(b) << "---------------\n";
        CA< StringWrapper >::TransListIter tsiter = pair.first;
        for( ; tsiter != pair.second; tsiter++ ) {
            wpds::CA< StringWrapper >::catrans_t t( *tsiter );

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

    return 0;
}
/* Yo, Emacs!
;;; Local Variables: ***
;;; tab-width: 4 ***
;;; End: ***
*/
