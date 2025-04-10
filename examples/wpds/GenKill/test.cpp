#include "VarSet.h"
using std::cout;
using std::cerr;
using std::endl;
using std::string;

int main() {
  VarSet v, w, x, y;

  cout << VarSet::UniverseSet() << endl;
  cout << VarSet::EmptySet() << endl;

  v = VarSet::EmptySet();
  w = VarSet::EmptySet();
  cout << VarSet::Diff(v,w) << endl;
  cout << VarSet::Diff(w,v) << endl;
  cout << VarSet::Union(v,w) << endl;
  cout << VarSet::Union(w,v) << endl;
  cout << VarSet::Intersect(v,w) << endl;
  cout << VarSet::Intersect(w,v) << endl;
  cout << v << endl;
  cout << w << endl;
  cout << VarSet::Eq(v,w) << endl;
  x = VarSet::Union(VarSet::Diff(v,w), VarSet::Union(VarSet::Diff(w,v), VarSet::Intersect(v,w)));
  y = VarSet::Union(v,w);
  cout << x << " == " << y << ": " << VarSet::Eq(x,y) << endl;


  v.Insert("a" );
  v.Insert("b" );
  v.Insert("c" );
  v.Insert("d" );
  w = mkVarSet("c", "d", "e", "f");
  cout << VarSet::Diff(v,w) << endl;
  cout << VarSet::Diff(w,v) << endl;
  cout << VarSet::Union(v,w) << endl;
  cout << VarSet::Union(w,v) << endl;
  cout << VarSet::Intersect(v,w) << endl;
  cout << VarSet::Intersect(w,v) << endl;
  cout << v << endl;
  cout << w << endl;
  cout << VarSet::Eq(v,w) << endl;
  x = VarSet::Union(VarSet::Diff(v,w), VarSet::Union(VarSet::Diff(w,v), VarSet::Intersect(v,w)));
  y = VarSet::Union(v,w);
  cout << x << " == " << y << ": " << VarSet::Eq(x,y) << endl;

  return(0);
}
