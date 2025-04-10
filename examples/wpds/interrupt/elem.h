#ifndef MY_SEMIRING_ELEMENT_H_
#define MY_SEMIRING_ELEMENT_H_
#include <iostream>
#include "common.h"

class Elem {
    friend std::ostream& operator<<( std::ostream& o,const Elem& e )
    {
        return e.print( o );
    }

    public:
        enum elem_state {ZERO,ONE,ENABLED,DISABLED};

        Elem( elem_state s_ = ENABLED,unsigned int u = 0 );

        static Elem* WPDS_CALL one();
        static Elem* WPDS_CALL zero();
        Elem * extend( Elem * ) const;
        Elem * combine( Elem * ) const;
        Elem * quasiOne() const;
        bool equal  ( Elem * ) const;
        std::ostream& print(std::ostream&) const;
    private:
        elem_state s;
        static Elem one_,zero_;
    public:
        unsigned int count;

};

#endif
