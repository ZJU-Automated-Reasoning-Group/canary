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

wpds::wpds_key_t p, A, B, C, D, lambda, accept;

using namespace wpds;

class LocalRuleExtender : public RuleExtender < StringWrapper > {
public:
    void add_rules(WPDS< StringWrapper >&wpds, 
                   wpds_key_t sym, wpds_key_t stack)
    {
        static int done = 0;
        if (sym == p && !done) {
            wpds.add_rule(p,lambda,p,A,new StringWrapper("a"));
            wpds.add_rule(p,A,p,B,new StringWrapper("b"));
            wpds.add_rule(p,B,p,C,new StringWrapper("c"));
            wpds.add_rule(p,C,p,D,new StringWrapper("d"));
            wpds.add_rule(p,A,p,A,B,new StringWrapper("w"));
        }
        done = 1;
    }
};

int main() {

    wpds::Semiring< StringWrapper > s(StringWrapper::one());
    wpds::WPDS< StringWrapper > wpds(s, new LocalRuleExtender);
    wpds::CA< StringWrapper > ca(s);
    wpds::CA< StringWrapper > ca2(s);

    p = str2key("p");
    A = str2key("A");
    B = str2key("B");
    C = str2key("C");
    D = str2key("D");
    lambda = str2key("lambda");
    accept = str2key("accept");

    ca.add(p,lambda,accept,StringWrapper::one());

    std::cout << wpds << std::endl <<std::endl;
    std::cout << "*** BEFORE ***\n" << ca << std::endl;
    ca2 = wpds::poststar< StringWrapper >( wpds,ca,s );
    std::cout << "*** AFTER  ***\n" << ca2 << std::endl;
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
