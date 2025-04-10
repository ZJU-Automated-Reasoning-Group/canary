#include <iostream>
#include <string>
#include "WPDS.h"
#include "elem.h"

/* We are going to check for a path in a function that
 * has a return path w/ interrupts still disabled.
 *
 * A:
 * --------------------------------------------------------
 * void f() {
 *  cli;
 *  if( ... )
 *      return -EBUG;
 *  // do stuff
 *  sti;
 *  }
 *
 * B:
 * --------------------------------------------------------
 * void f() {
 *  cli;
 *  if( ... ) {
 *      sti;
 *      return -EBUG;
 *  }
 *  // do stuff
 *  sti;
 *  }
 */

#define DO_A //undef to change WPDS rules to represent B
//#undef DO_A

int main() {

    wpds::Semiring< Elem > s(Elem::one());
    wpds::WPDS< Elem > wpds( s );
    wpds::CA< Elem > ca( s );
    wpds::wpds_key_t p = str2key("p");
    wpds::wpds_key_t y = str2key("y");
    wpds::wpds_key_t q = str2key("q");
    wpds::wpds_key_t e_f = str2key("e_f");
    wpds::wpds_key_t x_f = str2key("x_f");
    wpds::wpds_key_t n1 = str2key("n1");
    wpds::wpds_key_t n2 = str2key("n2");
    wpds::wpds_key_t n3 = str2key("n3");
    wpds::wpds_key_t n4 = str2key("n4");
    wpds::wpds_key_t accept = str2key("accept");
    wpds::wpds_key_t temp = str2key("temp");


    /* Entry points to f */
    wpds.add_rule(  p,e_f,  p,n1,   Elem::one()); // enter f

    /* Body of f */
    wpds.add_rule(  p,n1,   p,n2,   new Elem(Elem::DISABLED)); // cli
#ifdef DO_A
    /* This represents an incorrect program */
    wpds.add_rule(  p,n2,   p,n3,   Elem::one());  // if
#else
    /* This represents a correct program */
    wpds.add_rule(  p,n2,   p,temp, Elem::one());  // if
    wpds.add_rule(  p,temp, p,n3,   new Elem(Elem::ENABLED));
#endif

    wpds.add_rule(  p,n2,   p,n4,   new Elem(Elem::ENABLED)); // do stuff...;sti;

    /* Return points from f */
    wpds.add_rule(  p,n3,   p,x_f,  Elem::one()); // return
    wpds.add_rule(  p,n4,   p,x_f,  Elem::one()); // return
    wpds.add_rule(  p,x_f,  p,      Elem::one()); // return from f

    ca.add( p,e_f,accept,Elem::one());
    std::cout << ca << std::endl;
    wpds::CA< Elem > ca2 = 
        wpds::poststar< Elem >( wpds,ca,s );
    std::cout << ca2 << std::endl << std::endl;

    wpds::CA< Elem >::catrans_t t = ca2.find(p,x_f,accept);
    std::cout << "Interupts were ";
    //t->semiring_element()->print(std::cout) << std::endl;
    std::cout << t << std::endl;
    return 0;
}
