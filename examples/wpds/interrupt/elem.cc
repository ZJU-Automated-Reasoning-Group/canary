#include "elem.h"

Elem::Elem( elem_state s_, unsigned int u )
    : s(s_),count(u) {}

Elem Elem::one_( ONE,1 );
Elem Elem::zero_( ZERO,1 );

Elem * Elem::one()
{
    return &Elem::one_;
}

Elem * Elem::zero()
{
    return &Elem::zero_;
}

Elem * Elem::extend ( Elem *e ) const
{
    //std::cerr << *this << "extend( " << *e << " )\t";
    if( equal(zero()) || e->equal(zero()) ) {
        //std::cerr << "A\n";
        return zero();
    }
    else if( equal(e) || e->equal(one()) ) {
        //std::cerr << "B\n";
        return new Elem( s );
    }
    else {
        //std::cerr << "C\n";
        return new Elem( e->s );
    }
}

Elem * Elem::combine( Elem *e ) const
{
    if( equal(e) || e->equal(one()) || e->equal(zero()) )
        return new Elem(s);
    else if( equal(one()) || equal(zero()) )
        return new Elem(e->s);
    else
        // they are not equal and neither is 1/0, so it == disabled
        return new Elem(DISABLED);
}

Elem * Elem::quasiOne() const
{
    return one();
}

bool Elem::equal( Elem *e ) const
{
    return (s == e->s);
}

std::ostream& Elem::print( std::ostream& o ) const
{
    switch(s) {
        case DISABLED:
            o << "DISABLED";break;
        case ENABLED:
            o << "ENABLED";break;
        case ONE:
            o << "???{ONE}";break;
        case ZERO:
            o << "???{ZERO}";break;
    }
    return o;
}
