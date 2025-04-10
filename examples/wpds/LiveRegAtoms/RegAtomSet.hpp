#ifndef REGATOMSET_GUARD
#define REGATOMSET_GUARD 1

//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006, Thomas Reps 
// University of Wisconsin, Madison.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in
//      the documentation and/or other materials provided with the
//      distribution.
//
//   3. Neither the name of the University of Wisconsin, Madison nor the
//      names of its contributors may be used to endorse or promote
//      products derived from this software without specific prior
//      written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// e-mail: reps@cs.wisc.edu
//////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <limits.h>
#include "common.h"

// RegAtomSet ----------------------------------------

// Definitions of the bits used to represent individual register atoms
#define al   (1 << 0)
#define ah   (1 << 1)
#define uax  (1 << 2)
#define ax   (al | ah)
#define eax  (uax | ax)

#define bl   (1 << 3)
#define bh   (1 << 4)
#define ubx  (1 << 5)
#define bx   (bl | bh)
#define ebx  (ubx | bx)

#define cl   (1 << 6)
#define ch   (1 << 7)
#define ucx  (1 << 8)
#define cx   (cl | ch)
#define ecx  (ucx | cx)

#define dl   (1 << 9)
#define dh   (1 << 10)
#define udx  (1 << 11)
#define dx   (dl | dh)
#define edx  (udx | dx)

#define di   (1 << 12)
#define udi  (1 << 13)
#define edi  (udi | di)

#define si   (1 << 14)
#define usi  (1 << 15)
#define esi  (usi | si)

#define bp   (1 << 16)
#define ubp  (1 << 17)
#define ebp  (ubp | bp)

#define sp   (1 << 18)
#define usp  (1 << 19)
#define esp  (usp | sp)

#define EmptyRegAtomSet     0
#define UniverseRegAtomSet  (eax | ebx | ecx | edx | edi | esi | ebp | esp)

// bool Eq(RegAtomSet x, RegAtomSet y);
//#define Eq(x,y) ((x) == (y))

// RegAtomSet Diff(RegAtomSet x, RegAtomSet y);
//#define Diff(x,y) ((x) & (~(y) & UniverseRegAtomSet))

// RegAtomSet Union(RegAtomSet x, RegAtomSet y);
//#define Union(x,y) ((x) | (y))

// RegAtomSet Intersect(RegAtomSet x, RegAtomSet y);
//#define Intersect(x,y) ((x) & (y))

// LiveRegAtomsTransformer: gen/kill functions over RegAtomSets -------------

class RegAtomSet
{

    public: // static vars
        static const std::string regAtomNames[20];

    public: // static methods
        static bool WPDS_CALL Eq( const RegAtomSet& x, const RegAtomSet& y );
        static RegAtomSet WPDS_CALL Diff( const RegAtomSet& x, const RegAtomSet& y );
        static RegAtomSet WPDS_CALL Union( const RegAtomSet& x, const RegAtomSet& y );
        static RegAtomSet WPDS_CALL Intersect( const RegAtomSet& x, const RegAtomSet& y );
        static const RegAtomSet& WPDS_CALL UniverseSet();
        static const RegAtomSet& WPDS_CALL EmptySet();

    public: // methods
        RegAtomSet();
        RegAtomSet( unsigned int bv );
        RegAtomSet( const RegAtomSet& rhs );
        RegAtomSet& operator=( const RegAtomSet& rhs );
        std::ostream& print( std::ostream& o ) const;

    protected:
        unsigned int bitVector;


}; // class RegAtomSet

std::ostream& operator<<( std::ostream& o , const RegAtomSet& ras );

#endif // REGATOMSET_GUARD
