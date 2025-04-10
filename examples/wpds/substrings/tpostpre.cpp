#include <iostream>
#include <string>
#include <stdio.h>
#include "stringwrapper.h"
#ifdef OLD_WPDS_INDICES
#   include "wpds.h"
#else
#   include "WPDS.h"
#endif

int main() {
    wpds::Semiring< StringWrapper > s(StringWrapper::one());
    wpds::Semiring< StringWrapper > sR(StringWrapper::one(),false);

    wpds::WPDS< StringWrapper > wpds(s);
    wpds::WPDS< StringWrapper > wpds2(s);
    wpds::CA< StringWrapper > ca(s);
    wpds::CA< StringWrapper > ca2(s);

    /* wpds vars */
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

    /* wpds2 vars */
    wpds::wpds_key_t pprime = str2key("p'");
    wpds::wpds_key_t x = str2key("x");
    wpds::wpds_key_t xprime = str2key("x'");
    wpds::wpds_key_t x2prime = str2key("x''");

    /* wpds2 rules */
    wpds2.add_rule( pprime, x,      p,      A,  StringWrapper::one());
    wpds2.add_rule( pprime, xprime, pprime, x,  B,  StringWrapper::one());
    wpds2.add_rule( pprime, x2prime,pprime, xprime, B,  StringWrapper::one());

    ca.add(p,lambda,accept,StringWrapper::one());
    std::cout << "accept's weight is ";
    //ca.state_weight( accept )->print(std::cout) << std::endl;

    std::cout << wpds << std::endl <<std::endl;
    std::cout << "*** BEFORE ***\n" << ca << std::endl;
    ca2 = wpds::poststar< StringWrapper >( wpds,ca,s );
    std::cout << "*** AFTER  ***\n" << ca2 << std::endl;

    wpds::CA< StringWrapper > ca3 =
        wpds::prestar< StringWrapper >( wpds2,ca2,sR );

    wpds::CA< StringWrapper >::catrans_t goal;
    goal = ca3.find( pprime,x2prime,accept );

    std::cout << "\n\n AFTER PRESTAR ON WPDS2\n" << ca3 << std::endl;
    //std::cout << goal << std::endl;
    goal->print(std::cout) << std::endl;

#if 1
    ca3.add_final_state( accept );
    std::cout << "Calling path_summary...\n";
    std::set< wpds::wpds_key_t > R;
    R.insert( B );
    //std::cout << "accept's weight is " << ca3.state_weight( accept ) << std::endl;
    ca3.path_summary( R );
    std::cout << "accept's weight is ";
    ca3.state_weight( accept )->print(std::cout) << std::endl;
    //std::cout << "p's weight is " << ca3.state_weight( p ) << std::endl;
    std::cout << "p's weight is ";
    ca3.state_weight( p )->print(std::cout) << std::endl;
#endif
    return 0;
}
/* Yo, Emacs!
;;; Local Variables: ***
;;; tab-width: 4 ***
;;; End: ***
*/
