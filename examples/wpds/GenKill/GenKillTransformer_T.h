#ifndef GEN_KILL_TRANSFORMER_GUARD
#define GEN_KILL_TRANSFORMER_GUARD 1

#include <iostream>
#include <climits>
#include <cassert>
#include "common.h"
#include "ref_ptr.h"

/*!
 * @class GenKillTransformer_T
 *
 * GenKillTransformer_T is a templated class that implements the semiring
 * needed for a gen-kill dataflow-analysis problem. The template
 * parameter must implement the "Set" interface defined below
 *

class Set {

    public:
        // not needed by GenKillTransformer_T but a good idea
        Set();

        // Only Set constructor GenKillTransformer_T invokes
        Set( const Set& );

        // not needed by GenKillTransformer_T but a good idea
        Set& operator=( const Set& );

        static bool Eq( const Set& x, const Set& y );

        static Set Diff( const Set& x, const Set& y );

        static Set Union( const Set& x, const Set& y );

        static Set Intersect( const Set& x, const Set& y );

        // Optional (but recommended) constant generator
        static const Set& UniverseSet(); 

        // Optional (but recommended) constant generator
        static const Set& EmptySet();

        std::ostream& print(std::ostream& o);
};

*/

template< typename Set > class GenKillTransformer_T {

    public: // methods

        // A client uses makeGenKillTransformer_T to create a
        // GenKillTransformer_T instead of calling the constructor directly;
        // makeGenKillTransformer_T maintains unique representatives for the
        // special semiring values one and bottom.
        static GenKillTransformer_T* WPDS_CALL makeGenKillTransformer_T(
                const Set& k
                , const Set& g )
        {
            Set k_normalized = Set::Diff(k,g);
            if (Set::Eq(k_normalized, Set::EmptySet())&& 
                    Set::Eq(g, Set::UniverseSet()))
            {
                return GenKillTransformer_T::bottom();
            }
            else if (Set::Eq(k_normalized, Set::EmptySet()) && 
                    Set::Eq(g, Set::EmptySet()))
            {
                return GenKillTransformer_T::one();
            }
            else {
                return new GenKillTransformer_T(k_normalized, g);
            }
        }

        ~GenKillTransformer_T() {}

        //-------------------------------------------------
        // Semiring methods
        //-------------------------------------------------
        static GenKillTransformer_T* WPDS_CALL one()
        {
            // Uses a method-static variable to avoid
            // problems with static-initialization order
            static GenKillTransformer_T* ONE =
                new GenKillTransformer_T(Set::EmptySet(),Set::EmptySet(),1);
            return ONE;
        }

        // Zero is a special value that doesn't map to any gen/kill pair,
        // so all we really want out of this is a unique representative.
        // The gen/kill sets with which it is initialized are arbitrary.
        static GenKillTransformer_T* WPDS_CALL zero()
        {
            // Uses a method-static variable to avoid
            // problems with static-initialization order
            static GenKillTransformer_T* ZERO =
                new GenKillTransformer_T(Set::UniverseSet(),Set::EmptySet(),1);
            return ZERO;
        }

        static GenKillTransformer_T* WPDS_CALL bottom()
        {
            // Uses a method-static variable to avoid
            // problems with static-initialization order
            static GenKillTransformer_T* BOTTOM = 
                new GenKillTransformer_T(Set::EmptySet(),Set::UniverseSet(),1);
            return BOTTOM;
        }

        //
        // extend
        //
        // Return the extend of x (this) and y.
        // Considering x and y as functions, x extend y = y o x,
        // where (g o f)(v) = g(f(v)).
        //
        GenKillTransformer_T* extend( GenKillTransformer_T* y )
        {
            // Handle special case for either argument being zero()
            if( equal(GenKillTransformer_T::zero()) ) {
                return GenKillTransformer_T::zero();
            }
            else if( y->equal(GenKillTransformer_T::zero()) ) {
                return GenKillTransformer_T::zero();
            }
            // Handle special case for either argument being one().
            if( equal(GenKillTransformer_T::one()) ) {
                return y;
            }
            else if( y->equal(GenKillTransformer_T::one()) ) {
                return this;
            }
            // Handle the general case
            Set temp_k( Set::Union( kill, y->kill ) );
            Set temp_g( Set::Union( Set::Diff(gen,y->kill),y->gen) );
            return makeGenKillTransformer_T( temp_k,temp_g );
        }

        GenKillTransformer_T* combine( GenKillTransformer_T* y )
        {
            // Handle special case for either argument being zero()
            if( equal(GenKillTransformer_T::zero()) ) {
                return y;
            }
            else if( y->equal(GenKillTransformer_T::zero()) ) {
                return this;
            }
            // Handle the general case
            Set temp_k( Set::Intersect( kill, y->kill ) );
            Set temp_g( Set::Union( gen, y->gen ) );

            return makeGenKillTransformer_T( temp_k,temp_g );
        }

        GenKillTransformer_T* quasiOne() const
        {
            return one();
        }

        //
        // diff(GenKillTransformer_T* y)
        //
        // Return the difference between x (this) and y.
        //
        // The return value r has two properties:
        // 1. r ]= x,
        //    i.e., equal(combine(x,r), x) == true
        // 2. y combine r = y combine a,
        //    i.e., equal(combine(y,r), combine(y,a)) == true
        //
        GenKillTransformer_T* diff( GenKillTransformer_T* y )
        {
            // Handle special case for either argument being zero
            if( equal(GenKillTransformer_T::zero()) ) {
                return GenKillTransformer_T::zero();
            }
            else if( y->equal(GenKillTransformer_T::zero()) ) {
                return this;
            }

            // Both *this and *y are proper (non-zero) values

            Set temp_k( Set::Diff(Set::UniverseSet(),Set::Diff(y->kill,kill)) );
            Set temp_g( Set::Diff(gen,y->gen) );

            // Test for whether zero should be returned,
            // i.e., if *this >= *y.
            if (Set::Eq(temp_k, Set::UniverseSet()) && 
                Set::Eq(temp_g, Set::EmptySet()))
            {
                return GenKillTransformer_T::zero();
            }

            // Handle the general case
            return makeGenKillTransformer_T(temp_k, temp_g);
        }

        // Zero is a special representative that must be compared
        // by address rather by its contained Gen/Kill sets.
        bool equal(GenKillTransformer_T* y) const
        {
            if ( this == GenKillTransformer_T::zero() )
                return y == GenKillTransformer_T::zero();
            if ( y == GenKillTransformer_T::zero() )
                return this == GenKillTransformer_T::zero();
            return Set::Eq(kill,y->kill) && Set::Eq(gen,y->gen);
        }

        std::ostream& print( std::ostream& o ) const
        {
            o << "<\\S.(S - {" << kill << "}) U {" << gen << "}>";
            return o;
        }

        //-------------------------------------------------
        // Other useful methods
        //-------------------------------------------------

        Set apply( const Set& input ) {
            assert ( this != GenKillTransformer_T::zero() );
            return Set::Union( Set::Diff(input,kill),gen );
        }

        Set& getKill() {
            return kill;
        }

        const Set& getKill() const {
            return kill;
        }

        Set& getGen() {
            return gen;
        }

        const Set& getGen() const {
            return gen;
        }

        static std::ostream& WPDS_CALL print_static_transformers( std::ostream& o )
        {
            o << "ONE\t=\t"    << *one()    << std::endl;
            o << "ZERO\t=\t"   << *zero()   << std::endl;
            o << "BOTTOM\t=\t" << *bottom() << std::endl;
            return o;
        }

    private: // methods -----------------------------------------------------------

        // Constructor
        // The constructor is private to ensure uniqueness of one, zero, and bottom
        GenKillTransformer_T(const Set& k, const Set& g, unsigned int c=0) :
            kill(k), gen(g), count(c)
        {
#if 0
            std::cerr << "GenKillTransformer_T(" << k << ", " << g << ")" << std::endl;
#endif
        }

    private: // members -----------------------------------------------------------
        Set kill, gen;   // Used to represent the function \S.(S - kill) U gen

    public: // members -----------------------------------------------------------
        unsigned int count;
};

template< typename Set >
std::ostream& operator<< (std::ostream& out, const GenKillTransformer_T<Set>& t)
{
  t.print(out);
  return(out);
}

#endif

/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
*/
