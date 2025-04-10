#ifndef VARSET_GUARD
#define VARSET_GUARD 1

/*!
 * @author Nick Kidd
 *
 * This class is the VarSet interface;
 */

#include <string>
#include <iostream>
#include "dictionary_t.h"

class VarSet {
    public:
        typedef Dictionary_T<std::string,bool>::iterator iterator;

    public: // static methods
        static bool Eq( const VarSet& x, const VarSet& y );
        static VarSet Diff( const VarSet& x, const VarSet& y );
        static VarSet Union( const VarSet& x, const VarSet& y );
        static VarSet Intersect( const VarSet& x, const VarSet& y );
        static const VarSet& UniverseSet();
        static const VarSet& EmptySet();

    public:
        VarSet() {}
        VarSet( const VarSet& vs );
        VarSet& operator=( const VarSet& vs );
        iterator begin();
        void begin( VarSet::iterator& it );
        void Insert( std::string s );
        void Delete( std::string s );
        std::ostream& print( std::ostream& ) const;

    private:
        Dictionary_T< std::string,bool > setImpl;
};

std::ostream& operator<<( std::ostream& o , const VarSet& vs );
VarSet mkVarSet( std::string s);
VarSet mkVarSet( std::string s1,  std::string s2);
VarSet mkVarSet( std::string s1,  std::string s2,  std::string s3);
VarSet mkVarSet( std::string s1,  std::string s2,  std::string s3,  std::string s4);

#endif  // VARSET_GUARD

/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
*/

