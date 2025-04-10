#ifndef REACH_GUARD
#define REACH_GUARD 1

/*!
 * $Id: Reach.h,v 1.4 2006-01-13 20:00:00 kidd Exp $
 *
 * @author Nick Kidd
 * @version $Revision: 1.4 $
 */

#include "ref_ptr.h"
#include "common.h"

class Reach
{

    bool isreached;
    public:

        ref_ptr<Reach>::count_t count;

        Reach( bool b );

        Reach* one() const;

        Reach* zero() const;

        // zero is the annihilator for extend
        Reach* extend( const Reach* rhs ) const;
        
        // zero is neutral for combine
        Reach* combine( const Reach* rhs ) const;
        
        bool equal( const Reach* rhs ) const;

        Reach* quasiOne() const;

        std::ostream & print( std::ostream & o ) const;
};

#endif  // REACH_GUARD

/* Yo, Emacs!
;;; Local Variables: ***
;;; tab-width: 4 ***
;;; End: ***
*/

