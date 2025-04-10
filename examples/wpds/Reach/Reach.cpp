/*!
 * $Id: Reach.cpp,v 1.4 2005-09-06 16:11:34 radu Exp $
 *
 * @author Nick Kidd
 * @version $Revision: 1.4 $
 */

#include "Reach.h"

Reach::Reach( bool b ) :
    isreached(b),count(0) 
{
}

Reach* Reach::one() const
{
    return new Reach(true);
}

Reach* Reach::zero() const
{
    return new Reach(false);
}

// zero is the annihilator for extend
Reach* Reach::extend( const Reach* rhs ) const
{
    if( isreached && rhs->isreached )
        return one();
    else // this and rhs are one()
        return zero();
}

// zero is neutral for combine
Reach* Reach::combine( const Reach* rhs ) const
{
    if( isreached || rhs->isreached )
        return one();
    else
        return zero();
}

bool Reach::equal( const Reach* rhs ) const
{
    return ( isreached == rhs->isreached );
}

Reach* Reach::quasiOne() const
{
    return one();
}

std::ostream & Reach::print( std::ostream & o ) const
{
    return (isreached) ? o << "ONE" : o << "ZERO";
}

/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
 */

