/*!
 * @author Nick Kidd
 */

#include "VarSet.h"

std::ostream& operator<<( std::ostream& o , const VarSet& vs )
{
    vs.print(o);
    return o;
}

bool VarSet::Eq( const VarSet& x, const VarSet& y )
{
    VarSet ans;
    bool temp;

    VarSet::iterator beg;
    for ( x.setImpl.begin(beg); !beg.end() ; beg++ ) {
        if (y.setImpl.Lookup(beg.Key(), temp) == DICT_NOT_FOUND) {
            return false;
        }
    }
    for (y.setImpl.begin(beg); !beg.end(); beg++ ) {
        if (x.setImpl.Lookup(beg.Key(), temp) == DICT_NOT_FOUND) {
            return false;
        }
    }
    return true;
}

VarSet VarSet::Diff( const VarSet& x, const VarSet& y )
{
    VarSet ans = x;

    VarSet::iterator beg;
    for (y.setImpl.begin(beg); !beg.end(); beg++ ) {
        ans.Delete(beg.Key());
    }
    return ans;
}

VarSet VarSet::Union( const VarSet& x, const VarSet& y )
{
    VarSet ans = x;

    VarSet::iterator beg;
    for (y.setImpl.begin(beg); !beg.end(); beg++ ) {
        ans.Insert(beg.Key());
    }
    return ans;
}

VarSet VarSet::Intersect( const VarSet& x, const VarSet& y )
{
    VarSet ans;
    bool temp;

    VarSet::iterator beg;
    for (x.setImpl.begin(beg); !beg.end(); beg++) {
        if (y.setImpl.Lookup(beg.Key(), temp) == DICT_FOUND) {
            ans.Insert(beg.Key());
        }
    }
    return ans;
}

VarSet* makeUniverseVarSet()
{
    VarSet* vs = new VarSet();
    std::string s;
    for (int i = 0; i < 26; i++) {
        s = 'a' + i;
        vs->Insert(s);
    }
    return vs;
}

const VarSet& VarSet::UniverseSet()
{
    static VarSet* UNIVERSE_SET = makeUniverseVarSet();
    return *UNIVERSE_SET;
}

const VarSet& VarSet::EmptySet()
{
    static VarSet* EMPTY_SET = new VarSet();
    return *EMPTY_SET;
}

VarSet::VarSet( const VarSet& vs ) :
    setImpl(vs.setImpl)
{
}

VarSet& VarSet::operator=( const VarSet& vs )
{
    setImpl = vs.setImpl;
    return *this;
}

VarSet::iterator VarSet::begin()
{
    VarSet::iterator it;
    setImpl.begin(it);
    return it;
}

void VarSet::begin( VarSet::iterator& it )
{
    setImpl.begin(it);
}

void VarSet::Insert( std::string s )
{
    setImpl = setImpl.Insert(s,true);
}

void VarSet::Delete( std::string s )
{
    setImpl = setImpl.Delete(s);
}

std::ostream& VarSet::print( std::ostream& o ) const
{
    setImpl.Print(o);
    return o;
}

VarSet mkVarSet(std::string s)
{
  VarSet ans;
  ans.Insert(s);
  return ans;
}

VarSet mkVarSet(std::string s1, std::string s2)
{
  VarSet ans;
  ans.Insert(s1);
  ans.Insert(s2);
  return ans;
}

VarSet mkVarSet(std::string s1, std::string s2, std::string s3)
{
  VarSet ans;
  ans.Insert(s1);
  ans.Insert(s2);
  ans.Insert(s3);
  return ans;
}

VarSet mkVarSet(std::string s1, std::string s2, std::string s3, std::string s4)
{
  VarSet ans;
  ans.Insert(s1);
  ans.Insert(s2);
  ans.Insert(s3);
  ans.Insert(s4);
  return ans;
}
/* Yo, Emacs!
   ;;; Local Variables: ***
   ;;; tab-width: 4 ***
   ;;; End: ***
 */
