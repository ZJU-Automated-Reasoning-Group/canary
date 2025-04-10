/*!
 * @author Nick Kidd
 */

/*

// Pseudo Code //
x = 0
y = 0

fun f()
    n0: <$ f enter node $>
    n1: if( x = 0 )
    n2:     then y := 1
    n3:     else y := 2
    n4: g()
    n5: <$ f exit node $>

fun g()
    n6: <$ g enter node $>
    n7: y := 4
    n8: x := 60
    n9: <$ g exit node $>

 */
#include "WPDS.h"
#include "Reach.h"
#include <string>
#include <sstream>

int main()
{
    Reach* reachOne = new Reach(true);
    wpds::Semiring< Reach > s( reachOne );
    wpds::WPDS< Reach > myWpds(s);
    wpds::wpds_key_t p = str2key("p");
    wpds::wpds_key_t accept = str2key("accept");
    wpds::wpds_key_t n[10];
    for( int i=0 ; i < 10 ; i++ ) {
        std::stringstream ss;
        ss << "n" << i;
        n[i] = str2key( ss.str() );
    }

    // f intraprocedural
    myWpds.add_rule( p, n[0], p, n[1], new Reach(false)); // test add rule twice
    myWpds.add_rule( p, n[0], p, n[1], reachOne);
    myWpds.add_rule( p, n[1], p, n[2], reachOne);
    myWpds.add_rule( p, n[1], p, n[3], reachOne);
    myWpds.add_rule( p, n[2], p, n[4], reachOne);
    myWpds.add_rule( p, n[3], p, n[4], reachOne);

    // g intraprocedural
    myWpds.add_rule( p, n[6], p, n[7], reachOne);
    myWpds.add_rule( p, n[7], p, n[8], reachOne);
    myWpds.add_rule( p, n[8], p, n[9], reachOne);

    // f call g
    myWpds.add_rule( p, n[4], p, n[6], n[5], reachOne);

    // f return
    myWpds.add_rule( p, n[5] , p , reachOne);

    // g return
    myWpds.add_rule( p, n[9] , p , reachOne);

    // Print the WPDS
    myWpds.print( std::cout ) << std::endl;

    // Perform poststar query
    wpds::CA< Reach > query( s );
    query.add( p, n[0], accept, reachOne );
    query.add_initial_state( p );
    query.add_final_state( accept );
    query.print( std::cout << "BEFORE\n" ) << std::endl;
    wpds::CA< Reach > answer = wpds::poststar< Reach >(myWpds,query,s);
    answer.print( std::cout << "\nAFTER\n" ) << std::endl;

    // Perform path summary query
    answer.path_summary();
    ref_ptr< Reach > pWeight = answer.state_weight( p );
    std::cout << "Weight on state \"p\": ";
    pWeight->print( std::cout ) << std::endl;

    wpds::CA< Reach > reglang( s );
    Reach *ignored = new Reach( true );
    //manually add transitions of the regular expression
    reglang.add( str2key("t1") , n[9] , str2key("t2") , ignored);
    reglang.add( str2key("t2") , n[5] , str2key("t3") , ignored);
    reglang.add_initial_state( str2key("t1") );
    reglang.add_final_state( str2key("t3") );

    // answer is the same CA from the earlier poststar example
    ref_ptr< Reach > reglangWeight = answer.reglang_query( reglang );
    std::cout << "Result of reglang_query: ";
    reglangWeight->print( std::cout ) << std::endl;

    return 0;
}
/* Yo, Emacs!
;;; Local Variables: ***
;;; tab-width: 4 ***
;;; End: ***
*/

