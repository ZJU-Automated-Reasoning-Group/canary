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
    wpds::WPDS< StringWrapper > wpds(s);
    wpds::CA< StringWrapper > ca(s);
    wpds::CA< StringWrapper > ca2(s);

    wpds::WPDSKey p = str2key("p");
    wpds::WPDSKey A = str2key("A");
    wpds::WPDSKey B = str2key("B");
    wpds::WPDSKey q1 = str2key("q1");
    wpds::WPDSKey q2 = str2key("q2");
    wpds::WPDSKey accept = str2key("accept");

    wpds.add_rule(p,A,p,B,new StringWrapper("a"));
    wpds.add_rule(p,A,p,A,B,new StringWrapper("b"));

    std::cout << wpds << std::endl;

    ca.add(p,A,q1,new StringWrapper("a"));
    ca.add(q1,B,q2,StringWrapper::one());
    ca.add(q2,B,accept,StringWrapper::one());

    std::cout << "*** BEFORE ***\n" << ca << std::endl;
    ca2 = wpds::prestar< StringWrapper >( wpds,ca,s );
    std::cout << "done prestar!\n";
    std::cout << "*** AFTER  ***\n" << ca2 << std::endl;

    // path summary
    /*
    std::set< wpds::wpds_key_t > R;
    R.insert( B );
    ca2.path_summary( R );
    std::cout << "q2's weight is ";
    ca2.state_weight( q2 )->print(std::cout) << std::endl;
    */
    return 0;
}
