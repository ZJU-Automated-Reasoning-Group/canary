#include <iostream>
#include "WPDS.h"

class PA {

    friend std::ostream& operator<<( std::ostream& o, const PA& pa )
    {
        return pa.print(o);
    }

    public:
        PA( int i_ = 0,unsigned int c = 0 )
            : i(i_),count(c) {}
        PA( const PA& pa )
            : i(pa.i) {}    // leave count alone

        static PA* WPDS_CALL one() { return &one_; }
        static PA* WPDS_CALL zero() { return &one_; }
        PA * extend( PA * right ) const { return PA::one(); }
        PA * combine( PA * right ) const { return PA::one(); }
        PA * quasiOne() const { return PA::one(); }
        bool equal( PA * right ) const { return i == right->i; }
        std::ostream& print( std::ostream& o ) const
        {
            return (o << "PA{ i = " << i << " }");
        }
    private:
        int i;
        static PA one_;
    public:
        unsigned int count;

};

PA PA::one_(1,1);

/*
 * void f() {
 *      int *a;
 *      int *b;
 *      int **p;
 *      int i1 =0;
 *      int i2 = 0;
 *      int i3 = 0;
 *      int i4 = 0;
 *      a = &i1;
 *      b = &i2;
 *      b = &i3;
 *      p = &b;
 *      *p = a;
 * }
 */

using namespace wpds;

int main() {
    wpds::Semiring< PA > s(PA::one());
    wpds::WPDS< PA > wpds(s);
    wpds::CA< PA > ca(s);

    wpds_key accept = str2key("accept");
    wpds_key deref = str2key("deref");
    wpds_key a = str2key("a");
    wpds_key b = str2key("b");
    wpds_key p = str2key("p");
    wpds_key i1 = str2key("i1");
    wpds_key i2 = str2key("i2");
    wpds_key i3 = str2key("i3");
    wpds_key i4 = str2key("i4");

    wpds.add_rule(  a,deref,i1,             PA::one() );
    wpds.add_rule(  b,deref,i2,             PA::one() );
    wpds.add_rule(  b,deref,i3,             PA::one() );
    wpds.add_rule(  p,deref,b,              PA::one() );
    wpds.add_schema(p,deref,deref,a,deref,  new PA(4) );

    std::cout << wpds << std::endl;

    wpds::CA< PA > ca2 = wpds::prestar< PA >( wpds,ca,s );

    std::cout << wpds << std::endl;
    std::cout << ca2 << std::endl;

    return 0;
}
