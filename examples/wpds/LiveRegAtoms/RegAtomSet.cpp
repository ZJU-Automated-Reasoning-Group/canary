/*!
 * @author Nick Kidd
 */

#include <iostream>
#include "RegAtomSet.hpp"

const std::string RegAtomSet::regAtomNames[20] = {
    "al",
    "ah",
    "uax",
    "bl",
    "bh",
    "ubx",
    "cl",
    "ch",
    "ucx",
    "dl",
    "dh",
    "udx",
    "di",
    "udi",
    "si",
    "usi",
    "bp",
    "ubp",
    "sp",
    "usp"
};

bool RegAtomSet::Eq( const RegAtomSet& x, const RegAtomSet& y )
{
    return x.bitVector == y.bitVector;
}

RegAtomSet RegAtomSet::Diff( const RegAtomSet& x, const RegAtomSet& y )
{

    return (x.bitVector & (~y.bitVector & UniverseRegAtomSet));
}

RegAtomSet RegAtomSet::Union( const RegAtomSet& x, const RegAtomSet& y )
{
    return x.bitVector | y.bitVector;
}

RegAtomSet RegAtomSet::Intersect( const RegAtomSet& x, const RegAtomSet& y )
{
    return x.bitVector & y.bitVector;
}

const RegAtomSet& RegAtomSet::UniverseSet()
{
    static RegAtomSet* ras = new RegAtomSet( UniverseRegAtomSet );
    return *ras;
}

const RegAtomSet& RegAtomSet::EmptySet()
{
    static RegAtomSet* ras = new RegAtomSet( EmptyRegAtomSet );
    return *ras;
}


RegAtomSet::RegAtomSet() : bitVector( EmptyRegAtomSet )
{
}

RegAtomSet::RegAtomSet( unsigned int bv ) : bitVector(bv)
{
}

RegAtomSet::RegAtomSet( const RegAtomSet& rhs ) : bitVector( rhs.bitVector )
{
}

RegAtomSet& RegAtomSet::operator=( const RegAtomSet& rhs )
{
    bitVector = rhs.bitVector;
    return *this;
}

std::ostream& RegAtomSet::print( std::ostream& o ) const
{
    unsigned int mask = 1;
    bool seenAnElement = false;

    for(size_t i = 0; i < 20; i++) {
        if (bitVector & mask) {
            if (seenAnElement) {
                o << ", ";
            }
            o << regAtomNames[i];
            seenAnElement = true;
        }
        mask = mask << 1;
    }
    return o;
}

std::ostream& operator<<( std::ostream& o , const RegAtomSet& ras )
{
    ras.print(o);
    return o;
}

/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
 */
