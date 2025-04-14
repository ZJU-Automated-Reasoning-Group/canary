//===- llvm/ADT/MySparseBitVector.h - Efficient Sparse BitVector -*- C++ -*- ===//
//
//
//
//===----------------------------------------------------------------------===//

//#ifndef LLVM_ADT_SPARSEBITVECTOR_H
//#define LLVM_ADT_SPARSEBITVECTOR_H
#ifndef MY_SPARSEBITVECTOR_H
#define MY_SPARSEBITVECTOR_H

#include <stack>
#include <cassert>
#include <cstring>
#include <algorithm>
#include "llvm/Support/DataTypes.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/ADT/ilist.h"
#include "llvm/Support/raw_ostream.h"
namespace llvm {

/// MySparseBitVector is an implementation of a bitvector that is sparse
/// by only storing the elements that have non-zero bits set.  In
/// order to make this fast for the most common cases, MySparseBitVector
/// is implemented as a linked list of MySparseBitVectorElements.  We
/// maintain a pointer to the last MySparseBitVectorElement accessed (in
/// the form of a list iterator), in order to make multiple in-order
/// test/set constant time after the first one is executed.  Note that
/// using vectors to store MySparseBitVectorElement's does not work out
/// very well because it causes insertion in the middle to take
/// enormous amounts of time with a large amount of bits.  Other
/// structures that have better worst cases for insertion in the
/// middle (various balanced trees, etc) do not perform as well in
/// practice as a linked list with this iterator kept up to date.
/// They are also significantly more memory intensive.

// BENH
//
// OPT: the implementation can be optimized using SSE2+ instructions
// OPT: cap the alloc_stack to limit its growth
// OPT: improve the custom allocator to exploit locality (maybe use boost::pool)

template <unsigned ElementSize = 128>
struct MySparseBitVectorElement {
public:
  using BitWord = unsigned long;
  enum {
    BITWORD_SIZE = sizeof(BitWord) * 8,
    BITWORDS_PER_ELEMENT = (ElementSize + BITWORD_SIZE - 1) / BITWORD_SIZE,
    BITS_PER_ELEMENT = ElementSize,
    BYTES_PER_ELEMENT = sizeof(BitWord) * BITWORDS_PER_ELEMENT
  };

  MySparseBitVectorElement<ElementSize> *getNext() const {
    return Next;
  }
  MySparseBitVectorElement<ElementSize> *getPrev() const {
    return Prev;
  }

  void setNext(MySparseBitVectorElement<ElementSize> *RHS) {
    Next = RHS;
  }
  void setPrev(MySparseBitVectorElement<ElementSize> *RHS) {
    Prev = RHS;
  }

public:
  MySparseBitVectorElement<ElementSize> *Next;
  MySparseBitVectorElement<ElementSize> *Prev;
  // Index of Element in terms of where first bit starts.
  unsigned ElementIndex;
  BitWord Bits[BITWORDS_PER_ELEMENT];
  // Needed for sentinels
  MySparseBitVectorElement() {
    ElementIndex = ~0UL;
    memset(&Bits[0], 0, BYTES_PER_ELEMENT);
  }

  friend struct ilist_traits<MySparseBitVectorElement<ElementSize> >;
public:
  explicit MySparseBitVectorElement(unsigned Idx) {
    ElementIndex = Idx;
    memset(&Bits[0], 0, BYTES_PER_ELEMENT);
  }

  ~MySparseBitVectorElement() {
  }

  // Copy ctor.
  MySparseBitVectorElement(const MySparseBitVectorElement &RHS) {
    ElementIndex = RHS.ElementIndex;
    memcpy(&Bits[0],&RHS.Bits[0],BYTES_PER_ELEMENT);
  }

  // Comparison.
  bool operator==(const MySparseBitVectorElement &RHS) const {
    if (ElementIndex != RHS.ElementIndex)
      return false;
    return !memcmp(&Bits[0],&RHS.Bits[0],BYTES_PER_ELEMENT);
  }

  bool operator!=(const MySparseBitVectorElement &RHS) const {
    return !(*this == RHS);
  }

  // Return the bits that make up word Idx in our element.
  BitWord word(unsigned Idx) const {
    assert (Idx < BITWORDS_PER_ELEMENT);
    return Bits[Idx];
  }

  unsigned index() const {
    return ElementIndex;
  }

  bool empty() const {
    return !memcmp(&Bits[0],&zero[0],BYTES_PER_ELEMENT);
  }

  void set(unsigned Idx) {
    Bits[Idx / BITWORD_SIZE] |= 1L << (Idx % BITWORD_SIZE);
  }

  bool test_and_set (unsigned Idx) {
    bool old = test(Idx);
    if (!old) {
      set(Idx);
      return true;
    }
    return false;
  }

  void reset(unsigned Idx) {
    Bits[Idx / BITWORD_SIZE] &= ~(1L << (Idx % BITWORD_SIZE));
  }

  bool test(unsigned Idx) const {
    return Bits[Idx / BITWORD_SIZE] & (1L << (Idx % BITWORD_SIZE));
  }

  unsigned count() const {
    unsigned NumBits = 0;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i)
      if (sizeof(BitWord) == 4)
        NumBits += llvm::countPopulation(Bits[i]);
      else if (sizeof(BitWord) == 8)
        NumBits += llvm::countPopulation(Bits[i]);
      else
        assert(0 && "Unsupported!");
    return NumBits;
  }

  /// find_first - Returns the index of the first set bit.
  int find_first() const {
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i)
      if (Bits[i] != 0) {
        if (sizeof(BitWord) == 4)
          return i * BITWORD_SIZE + llvm::countTrailingZeros(Bits[i]);
        else if (sizeof(BitWord) == 8)
          return i * BITWORD_SIZE + llvm::countTrailingZeros(Bits[i]);
        else
          assert(0 && "Unsupported!");
      }
    assert(0 && "Illegal empty element");
  }

  /// find_next - Returns the index of the next set bit starting from the
  /// "Curr" bit. Returns -1 if the next set bit is not found.
  int find_next(unsigned Curr) const {
    if (Curr >= BITS_PER_ELEMENT)
      return -1;

    unsigned WordPos = Curr / BITWORD_SIZE;
    unsigned BitPos = Curr % BITWORD_SIZE;
    BitWord Copy = Bits[WordPos];
    assert (WordPos <= BITWORDS_PER_ELEMENT
            && "Word Position outside of element");

    // Mask off previous bits.
    Copy &= ~0L << BitPos;

    if (Copy != 0) {
      if (sizeof(BitWord) == 4)
        return WordPos * BITWORD_SIZE + llvm::countTrailingZeros(Copy);
      else if (sizeof(BitWord) == 8)
        return WordPos * BITWORD_SIZE + llvm::countTrailingZeros(Copy);
      else
        assert(0 && "Unsupported!");
    }

    // Check subsequent words.
    for (unsigned i = WordPos+1; i < BITWORDS_PER_ELEMENT; ++i)
      if (Bits[i] != 0) {
        if (sizeof(BitWord) == 4)
          return i * BITWORD_SIZE + llvm::countTrailingZeros(Bits[i]);
        else if (sizeof(BitWord) == 8)
          return i * BITWORD_SIZE + llvm::countTrailingZeros(Bits[i]);
        else
          assert(0 && "Unsupported!");
      }
    return -1;
  }

  // Union this element with RHS and return true if this one changed.
  bool unionWith(const MySparseBitVectorElement &RHS) {
    bool changed = false;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      BitWord old = changed ? 0 : Bits[i];

      Bits[i] |= RHS.Bits[i];
      if (!changed && old != Bits[i])
        changed = true;
    }
    return changed;
  }

  // Return true if we have any bits in common with RHS
  bool intersects(const MySparseBitVectorElement &RHS) const {
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      if (RHS.Bits[i] & Bits[i])
        return true;
    }
    return false;
  }

  // Intersect this Element with RHS and return true if this one changed.
  // BecameZero is set to true if this element became all-zero bits.
  bool intersectWith(const MySparseBitVectorElement &RHS,
                     bool &BecameZero) {
    bool changed = false;
    bool allzero = true;

    BecameZero = false;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      BitWord old = changed ? 0 : Bits[i];

      Bits[i] &= RHS.Bits[i];
      if (Bits[i] != 0)
        allzero = false;

      if (!changed && old != Bits[i])
        changed = true;
    }
    BecameZero = allzero;
    return changed;
  }
  // Intersect this Element with the complement of RHS and return true if this
  // one changed.  BecameZero is set to true if this element became all-zero
  // bits.
  bool intersectWithComplement(const MySparseBitVectorElement &RHS,
                               bool &BecameZero) {
    bool changed = false;
    bool allzero = true;

    BecameZero = false;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      BitWord old = changed ? 0 : Bits[i];

      Bits[i] &= ~RHS.Bits[i];
      if (Bits[i] != 0)
        allzero = false;

      if (!changed && old != Bits[i])
        changed = true;
    }
    BecameZero = allzero;
    return changed;
  }
  // Three argument version of intersectWithComplement that intersects
  // RHS1 & ~RHS2 into this element
  void intersectWithComplement(const MySparseBitVectorElement &RHS1,
                               const MySparseBitVectorElement &RHS2,
                               bool &BecameZero) {
    bool allzero = true;

    BecameZero = false;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      Bits[i] = RHS1.Bits[i] & ~RHS2.Bits[i];
      if (Bits[i] != 0)
        allzero = false;
    }
    BecameZero = allzero;
  }

  // Get a hash value for this element;
  uint64_t getHashValue() const {
    uint64_t HashVal = 0;
    for (unsigned i = 0; i < BITWORDS_PER_ELEMENT; ++i) {
      HashVal ^= Bits[i];
    }
    return HashVal;
  }

  // return a pointer to the Bits array
  BitWord* getBits() { return &Bits[0]; }

  // increment the ElementIndex by the given amount
  void incIndex(unsigned x) { ElementIndex += x; }

  // initialize the element
  void init(unsigned idx)
  {
    ElementIndex = idx;
    memset(&Bits[0],0,BYTES_PER_ELEMENT);
  }

  // initialize the element
  void init(const MySparseBitVectorElement &RHS)
  {
    ElementIndex = RHS.ElementIndex;
    memcpy(&Bits[0],&RHS.Bits[0],BYTES_PER_ELEMENT);
  }

  // to compare an Element against zero
  static BitWord zero[BITWORDS_PER_ELEMENT];
};

//FIX: can't figure out how to use 'BitWord' instead of 'long unsigned'
template<unsigned ElementSize> long unsigned MySparseBitVectorElement<ElementSize>::zero[] = { 0 };


template <unsigned ElementSize = 128>
class MySparseBitVector {
  using ElementList = ilist<MySparseBitVectorElement<ElementSize> >;
  using ElementListIter = typename ElementList::iterator;
  using ElementListConstIter = typename ElementList::const_iterator;
  using BitWord = typename MySparseBitVectorElement<ElementSize>::BitWord;
  enum {
    BITWORD_SIZE = MySparseBitVectorElement<ElementSize>::BITWORD_SIZE,
    WORDSZ = BITWORD_SIZE,
    NWORDS = MySparseBitVectorElement<ElementSize>::BITWORDS_PER_ELEMENT,
    BYTES_PER_ELEMENT = MySparseBitVectorElement<ElementSize>::BYTES_PER_ELEMENT
  };

  // Pointer to our current Element.
  ElementListIter CurrElementIter;
  ElementList Elements;

  //// BEGIN FREELIST IMPLEMENTATION

  static std::stack<MySparseBitVectorElement<ElementSize>*> alloc_stack;

  MySparseBitVectorElement<ElementSize>* getNewElement(unsigned idx)
  {
    MySparseBitVectorElement<ElementSize> *ret = 0;

    if (alloc_stack.empty()) {
      ret = new MySparseBitVectorElement<ElementSize>(idx);
    }
    else {
      ret = alloc_stack.top(); alloc_stack.pop();
      assert(ret);
      ret->init(idx);
    }

    return ret;
  }

  MySparseBitVectorElement<ElementSize>* getNewElement(const MySparseBitVectorElement<ElementSize>& cpy)
  {
    MySparseBitVectorElement<ElementSize> *ret = 0;

    if (alloc_stack.empty()) {
      ret = new MySparseBitVectorElement<ElementSize>(cpy);
    }
    else {
      ret = alloc_stack.top(); alloc_stack.pop();
      assert(ret);
      ret->init(cpy);
    }

    return ret;
  }

  void eraseElement(MySparseBitVectorElement<ElementSize> *el)
  {
    assert(el);
    alloc_stack.push(el);
  }

  void eraseElement(const ElementListIter &it)
  {
    MySparseBitVectorElement<ElementSize> *el = Elements.remove(it);
    assert(el);
    alloc_stack.push(el);
  }

  void eraseElement(const ElementListIter &bgn, const ElementListIter &end)
  {
    ElementListIter i = bgn, k;
    while (i != end) { k = i; ++i; eraseElement(k); }
  }

  //// END FREELIST IMPLEMENTATION

  // This is like std::lower_bound, except we do linear searching from the
  // current position.
  ElementListIter FindLowerBound(unsigned ElementIndex) {

    if (Elements.empty()) {
      CurrElementIter = Elements.begin();
      return Elements.begin();
    }

    // Make sure our current iterator is valid.
    if (CurrElementIter == Elements.end())
      --CurrElementIter;

    // Search from our current iterator, either backwards or forwards,
    // depending on what element we are looking for.
    ElementListIter ElementIter = CurrElementIter;
    if (CurrElementIter->index() == ElementIndex) {
      return ElementIter;
    } else if (CurrElementIter->index() > ElementIndex) {
      while (ElementIter != Elements.begin()
             && ElementIter->index() > ElementIndex)
        --ElementIter;
    } else {
      while (ElementIter != Elements.end() &&
             ElementIter->index() < ElementIndex)
        ++ElementIter;
    }
    CurrElementIter = ElementIter;
    return ElementIter;
  }

  // Iterator to walk set bits in the bitmap.  This iterator is a lot uglier
  // than it would be, in order to be efficient.
  class MySparseBitVectorIterator {
  private:
    bool AtEnd;

    const MySparseBitVector<ElementSize> *BitVector;

    // Current element inside of bitmap.
    ElementListConstIter Iter;

    // Current bit number inside of our bitmap.
    unsigned BitNumber;

    // Current word number inside of our element.
    unsigned WordNumber;

    // Current bits from the element.
    typename MySparseBitVectorElement<ElementSize>::BitWord Bits;

    // Move our iterator to the first non-zero bit in the bitmap.
    void AdvanceToFirstNonZero() {
      if (AtEnd)
        return;
      if (BitVector->Elements.empty()) {
        AtEnd = true;
        return;
      }
      Iter = BitVector->Elements.begin();
      BitNumber = Iter->index() * ElementSize;
      unsigned BitPos = Iter->find_first();
      BitNumber += BitPos;
      WordNumber = (BitNumber % ElementSize) / BITWORD_SIZE;
      Bits = Iter->word(WordNumber);
      Bits >>= BitPos % BITWORD_SIZE;
    }

    // Move our iterator to the next non-zero bit.
    void AdvanceToNextNonZero() {
      if (AtEnd)
        return;

      while (Bits && !(Bits & 1)) {
        Bits >>= 1;
        BitNumber += 1;
      }

      // See if we ran out of Bits in this word.
      if (!Bits) {
        int NextSetBitNumber = Iter->find_next(BitNumber % ElementSize) ;
        // If we ran out of set bits in this element, move to next element.
        if (NextSetBitNumber == -1 || (BitNumber % ElementSize == 0)) {
          ++Iter;
          WordNumber = 0;

          // We may run out of elements in the bitmap.
          if (Iter == BitVector->Elements.end()) {
            AtEnd = true;
            return;
          }
          // Set up for next non zero word in bitmap.
          BitNumber = Iter->index() * ElementSize;
          NextSetBitNumber = Iter->find_first();
          BitNumber += NextSetBitNumber;
          WordNumber = (BitNumber % ElementSize) / BITWORD_SIZE;
          Bits = Iter->word(WordNumber);
          Bits >>= NextSetBitNumber % BITWORD_SIZE;
        } else {
          WordNumber = (NextSetBitNumber % ElementSize) / BITWORD_SIZE;
          Bits = Iter->word(WordNumber);
          Bits >>= NextSetBitNumber % BITWORD_SIZE;
          BitNumber = Iter->index() * ElementSize;
          BitNumber += NextSetBitNumber;
        }
      }
    }
  public:
    // Preincrement.
    inline MySparseBitVectorIterator& operator++() {
      ++BitNumber;
      Bits >>= 1;
      AdvanceToNextNonZero();
      return *this;
    }

    // Postincrement.
    inline MySparseBitVectorIterator operator++(int) {
      MySparseBitVectorIterator tmp = *this;
      ++*this;
      return tmp;
    }

    // Return the current set bit number.
    unsigned operator*() const {
      return BitNumber;
    }

    bool operator==(const MySparseBitVectorIterator &RHS) const {
      // If they are both at the end, ignore the rest of the fields.
      if (AtEnd && RHS.AtEnd)
        return true;
      // Otherwise they are the same if they have the same bit number and
      // bitmap.
      return AtEnd == RHS.AtEnd && RHS.BitNumber == BitNumber;
    }
    bool operator!=(const MySparseBitVectorIterator &RHS) const {
      return !(*this == RHS);
    }
    MySparseBitVectorIterator(): BitVector(NULL) {
    }


    MySparseBitVectorIterator(const MySparseBitVector<ElementSize> *RHS,
                            bool end = false):BitVector(RHS) {
      Iter = BitVector->Elements.begin();
      BitNumber = 0;
      Bits = 0;
      WordNumber = ~0;
      AtEnd = end;
      AdvanceToFirstNonZero();
    }
  };
public:
  using iterator = MySparseBitVectorIterator;

  MySparseBitVector () {
    CurrElementIter = Elements.begin ();
  }

  ~MySparseBitVector() {
    eraseElement(Elements.begin(),Elements.end());
  }

  // MySparseBitVector copy ctor.
  MySparseBitVector(const MySparseBitVector &RHS) {
    ElementListConstIter ElementIter = RHS.Elements.begin();
    while (ElementIter != RHS.Elements.end()) {
      MySparseBitVectorElement<ElementSize> *el = getNewElement(*ElementIter);
      Elements.push_back(el);
      ++ElementIter;
    }

    CurrElementIter = Elements.begin ();
  }

  // Test, Reset, and Set a bit in the bitmap.
  bool test(unsigned Idx) {
    if (Elements.empty())
      return false;

    unsigned ElementIndex = Idx / ElementSize;
    ElementListIter ElementIter = FindLowerBound(ElementIndex);

    // If we can't find an element that is supposed to contain this bit, there
    // is nothing more to do.
    if (ElementIter == Elements.end() ||
        ElementIter->index() != ElementIndex)
      return false;
    return ElementIter->test(Idx % ElementSize);
  }

  void reset(unsigned Idx) {
    if (Elements.empty())
      return;

    unsigned ElementIndex = Idx / ElementSize;
    ElementListIter ElementIter = FindLowerBound(ElementIndex);

    // If we can't find an element that is supposed to contain this bit, there
    // is nothing more to do.
    if (ElementIter == Elements.end() ||
        ElementIter->index() != ElementIndex)
      return;
    ElementIter->reset(Idx % ElementSize);

    // When the element is zeroed out, delete it.
    if (ElementIter->empty()) {
      ++CurrElementIter;
      eraseElement(ElementIter);
    }
  }

  void set(unsigned Idx) {
    unsigned ElementIndex = Idx / ElementSize;
    MySparseBitVectorElement<ElementSize> *Element;
    ElementListIter ElementIter;
    if (Elements.empty()) {
      Element = getNewElement(ElementIndex);
      ElementIter = Elements.insert(Elements.end(), Element);

    } else {
      ElementIter = FindLowerBound(ElementIndex);

      if (ElementIter == Elements.end() ||
          ElementIter->index() != ElementIndex) {
        Element = getNewElement(ElementIndex);
        // We may have hit the beginning of our MySparseBitVector, in
        // which case, we may need to insert right after this element,
        // which requires moving the current iterator forward one,
        // because insert does insert before.
        if (ElementIter != Elements.end() &&
            ElementIter->index() < ElementIndex)
          ElementIter = Elements.insert(++ElementIter, Element);
        else
          ElementIter = Elements.insert(ElementIter, Element);
      }
    }
    CurrElementIter = ElementIter;

    ElementIter->set(Idx % ElementSize);
  }

  bool test_and_set (unsigned Idx) {
    bool old = test(Idx);
    if (!old) {
      set(Idx);
      return true;
    }
    return false;
  }

  bool operator!=(const MySparseBitVector &RHS) const {
    return !(*this == RHS);
  }

  bool operator==(const MySparseBitVector &RHS) const {
    ElementListConstIter Iter1 = Elements.begin();
    ElementListConstIter Iter2 = RHS.Elements.begin();

    for (; Iter1 != Elements.end() && Iter2 != RHS.Elements.end();
         ++Iter1, ++Iter2) {
      if (*Iter1 != *Iter2)
        return false;
    }
    return Iter1 == Elements.end() && Iter2 == RHS.Elements.end();
  }

  MySparseBitVector& operator=(const MySparseBitVector &RHS)
  {
    clear();

    for (ElementListConstIter it = RHS.Elements.begin(), 
	   end = RHS.Elements.end(); it != end; ++it) {
      Elements.insert(Elements.end(),getNewElement(*it));
    }
    
    return *this;
  }

  // Union our bitmap with the RHS and return true if we changed.
  bool operator|=(const MySparseBitVector &RHS) {
    bool changed = false;
    ElementListIter Iter1 = Elements.begin();
    ElementListConstIter Iter2 = RHS.Elements.begin();

    // If RHS is empty, we are done
    if (RHS.Elements.empty())
      return false;

    while (Iter2 != RHS.Elements.end()) {
      if (Iter1 == Elements.end() || Iter1->index() > Iter2->index()) {
        Elements.insert(Iter1,getNewElement(*Iter2));
        ++Iter2;
        changed = true;
      } else if (Iter1->index() == Iter2->index()) {
        changed |= Iter1->unionWith(*Iter2);
        ++Iter1;
        ++Iter2;
      } else {
        ++Iter1;
      }
    }
    CurrElementIter = Elements.begin();
    return changed;
  }

  // Intersect our bitmap with the RHS and return true if ours changed.
  bool operator&=(const MySparseBitVector &RHS) {
    bool changed = false;
    ElementListIter Iter1 = Elements.begin();
    ElementListConstIter Iter2 = RHS.Elements.begin();

    // Check if both bitmaps are empty.
    if (Elements.empty() && RHS.Elements.empty())
      return false;

    // Loop through, intersecting as we go, erasing elements when necessary.
    while (Iter2 != RHS.Elements.end()) {
      if (Iter1 == Elements.end()) {
        CurrElementIter = Elements.begin();
        return changed;
      }

      if (Iter1->index() > Iter2->index()) {
        ++Iter2;
      } else if (Iter1->index() == Iter2->index()) {
        bool BecameZero;
        changed |= Iter1->intersectWith(*Iter2, BecameZero);
        if (BecameZero) {
          ElementListIter IterTmp = Iter1;
          ++Iter1;
          eraseElement(IterTmp);
        } else {
          ++Iter1;
        }
        ++Iter2;
      } else {
        ElementListIter IterTmp = Iter1;
        ++Iter1;
        eraseElement(IterTmp);
      }
    }
    eraseElement(Iter1, Elements.end());
    CurrElementIter = Elements.begin();
    return changed;
  }

  // Intersect our bitmap with the complement of the RHS and return
  // true if ours changed.
  bool intersectWithComplement(const MySparseBitVector &RHS) {
    bool changed = false;
    ElementListIter Iter1 = Elements.begin();
    ElementListConstIter Iter2 = RHS.Elements.begin();

    // If either our bitmap or RHS is empty, we are done
    if (Elements.empty() || RHS.Elements.empty())
      return false;

    // Loop through, intersecting as we go, erasing elements when necessary.
    while (Iter2 != RHS.Elements.end()) {
      if (Iter1 == Elements.end()) {
        CurrElementIter = Elements.begin();
        return changed;
      }

      if (Iter1->index() > Iter2->index()) {
        ++Iter2;
      } else if (Iter1->index() == Iter2->index()) {
        bool BecameZero;
        changed |= Iter1->intersectWithComplement(*Iter2, BecameZero);
        if (BecameZero) {
          ElementListIter IterTmp = Iter1;
          ++Iter1;
          eraseElement(IterTmp);
        } else {
          ++Iter1;
        }
        ++Iter2;
      } else {
        ++Iter1;
      }
    }
    CurrElementIter = Elements.begin();
    return changed;
  }

  bool intersectWithComplement(const MySparseBitVector<ElementSize> *RHS) const {
    return intersectWithComplement(*RHS);
  }


  //  Three argument version of intersectWithComplement.  Result of
  //  RHS1 & ~RHS2 is stored into this bitmap.
  void intersectWithComplement(const MySparseBitVector<ElementSize> &RHS1,
                               const MySparseBitVector<ElementSize> &RHS2)
  {
    clear();
    ElementListConstIter Iter1 = RHS1.Elements.begin();
    ElementListConstIter Iter2 = RHS2.Elements.begin();

    // If RHS1 is empty, we are done
    // If RHS2 is empty, we still have to copy RHS1
    if (RHS1.Elements.empty())
      return;

    // Loop through, intersecting as we go, erasing elements when necessary.
    while (Iter2 != RHS2.Elements.end()) {
      if (Iter1 == RHS1.Elements.end())
        return;

      if (Iter1->index() > Iter2->index()) {
        ++Iter2;
      } else if (Iter1->index() == Iter2->index()) {
        bool BecameZero = false;
        MySparseBitVectorElement<ElementSize> *NewElement =
          getNewElement(Iter1->index());
        NewElement->intersectWithComplement(*Iter1, *Iter2, BecameZero);
        if (!BecameZero) {
          Elements.push_back(NewElement);
        }
        else eraseElement(NewElement);
        ++Iter1;
        ++Iter2;
      } else {
        MySparseBitVectorElement<ElementSize> *NewElement = 
	  getNewElement(*Iter1);
        Elements.push_back(NewElement);
        ++Iter1;
      }
    }

    // copy the remaining elements
    while (Iter1 != RHS1.Elements.end()) {
        MySparseBitVectorElement<ElementSize> *NewElement =
          getNewElement(*Iter1);
        Elements.push_back(NewElement);
        ++Iter1;
      }

    return;
  }

  void intersectWithComplement(const MySparseBitVector<ElementSize> *RHS1,
                               const MySparseBitVector<ElementSize> *RHS2) {
    intersectWithComplement(*RHS1, *RHS2);
  }

  bool intersects(const MySparseBitVector<ElementSize> *RHS) const {
    return intersects(*RHS);
  }

  // Return true if we share any bits in common with RHS
  bool intersects(const MySparseBitVector<ElementSize> &RHS) const {
    ElementListConstIter Iter1 = Elements.begin();
    ElementListConstIter Iter2 = RHS.Elements.begin();

    // Check if both bitmaps are empty.
    if (Elements.empty() && RHS.Elements.empty())
      return false;

    // Loop through, intersecting stopping when we hit bits in common.
    while (Iter2 != RHS.Elements.end()) {
      if (Iter1 == Elements.end())
        return false;

      if (Iter1->index() > Iter2->index()) {
        ++Iter2;
      } else if (Iter1->index() == Iter2->index()) {
        if (Iter1->intersects(*Iter2))
          return true;
        ++Iter1;
        ++Iter2;
      } else {
        ++Iter1;
      }
    }
    return false;
  }

  // Return the first set bit in the bitmap.  Return -1 if no bits are set.
  int find_first() const {
    if (Elements.empty())
      return -1;
    const MySparseBitVectorElement<ElementSize> &First = *(Elements.begin());
    return (First.index() * ElementSize) + First.find_first();
  }

  // Return true if the MySparseBitVector is empty
  bool empty() const {
    return Elements.empty();
  }

  unsigned count() const {
    unsigned BitCount = 0;
    for (ElementListConstIter Iter = Elements.begin();
         Iter != Elements.end();
         ++Iter)
      BitCount += Iter->count();

    return BitCount;
  }
  iterator begin() const {
    return iterator(this);
  }

  iterator end() const {
    return iterator(this, true);
  }

  // Get a hash value for this bitmap.
  uint64_t getHashValue() const {
    uint64_t HashVal = 0;
    for (ElementListConstIter Iter = Elements.begin();
         Iter != Elements.end();
         ++Iter) {
      HashVal ^= Iter->index();
      HashVal ^= Iter->getHashValue();
    }
    return HashVal;
  }

  bool operator>>=(unsigned off)
  {
    if (Elements.empty() || off == 0) { return false; }

    int els = off / ElementSize; // whole BitArrays contained in offset

    if (els) {
      for (ElementListIter i = Elements.begin(), e = Elements.end();
	   i != e; ++i) { // first shift by whole BitArrays
	i->incIndex(els);
      }

      off %= ElementSize; // now we'll shift by the remainder
      if (off == 0) { return true; }
    }

    int whole  = off / WORDSZ;    // whole words contained in offset
    int msb_sz = off % WORDSZ;    // most-significant bits
    int lsb_sz = WORDSZ - msb_sz; // least-significant bits

    // NOTE: if lsb_sz == WORDSZ then lsb and msb will be incorrect,
    // because shifting a word by >= the word size is undefined (and
    // gcc just leaves the word alone without shifting at all); this
    // doesn't matter because we don't need lsb and msb if lsb_sz ==
    // WORDSZ, see the code below for details
    
    BitWord lsb = (1 << lsb_sz) - 1;  // lsb mask
    BitWord msb = ~lsb;               // msb mask
    
    int i, j;
    
    BitWord buffer[2*NWORDS];
    BitWord *buff = &buffer[0];
    
    memset(buff,0,BYTES_PER_ELEMENT*2);
    
    ElementListIter ii = Elements.begin(), k = Elements.end();
    
    while (ii != Elements.end()) {
      BitWord *bits = ii->getBits();
      
      // copy bits to buff at offset; if we're shifting by whole words
      // take advantage to make the copy more efficient
      //
      if (off % WORDSZ == 0) { 
	memcpy(&buff[whole],bits,BYTES_PER_ELEMENT);
      }
      else {
	BitWord prev = 0; // msb bits from previous word, shifted to lsb

	for (i = NWORDS+whole, j = NWORDS-1; i > whole; --i, --j) {
	  buff[i] = ((bits[j] & msb) >> lsb_sz) | prev;
	  prev = (bits[j] & lsb) << msb_sz;
	}

	buff[i] |= prev;
      }
      
      //
      // if buff_1 is all 0 erase this element
      // else memcpy buff_1 to this element
      //
      
      bool not_zero = memcmp(&buff[0],&MySparseBitVectorElement<ElementSize>::zero[0],BYTES_PER_ELEMENT);
            
      if (not_zero) { memcpy(bits,buff,BYTES_PER_ELEMENT); }
      else { k = ii; }
      
      // if buff_2 is not all 0
      //   if next element is in series
      //     memcpy buff_2 to buff_1, memset buff_2 to 0
      //   else insert new next element, init to buff_2, memset buff to 0
      // else memset buff_1 to 0
      
      not_zero = memcmp(&buff[NWORDS],&MySparseBitVectorElement<ElementSize>::zero[0],BYTES_PER_ELEMENT);
      
      if (not_zero) { // carry-over to next Element
	if (ii->getNext()->index() == ii->index()+1) { // adjacent Element
	  memcpy(buff,&buff[NWORDS],BYTES_PER_ELEMENT);
	  memset(&buff[NWORDS],0,BYTES_PER_ELEMENT);
	}
	else { // no adjacent Element
	  int idx = ii->index();
	  ii = Elements.insert(++ii,getNewElement(idx+1));
	  bits = ii->getBits();
	  memcpy(bits,&buff[NWORDS],BYTES_PER_ELEMENT);
	  memset(buff,0,BYTES_PER_ELEMENT*2);
	}
      }
      else { memset(buff,0,BYTES_PER_ELEMENT); }
      
      // erase this element if necessary
      //
      if (k != Elements.end()) {
	++ii;
	eraseElement(k);
	k = Elements.end();
      }
      else { ++ii; }
    }

    CurrElementIter = Elements.begin();

    return true;
  }

  void clear()
  {
    eraseElement(Elements.begin(),Elements.end());
    CurrElementIter = Elements.begin();
  }

  // return true iff there is exactly one bit set
  bool single_bit_p()
  {
    ElementListIter i = Elements.begin();

    if (i != Elements.end() && i->getNext()->index() == ~0U) {
      return (i->count() == 1);
    }

    return false;
  }

  // clear bits [bgn..bgn+cnt), taken largely from GCC's bitmap.c
  void clear_range(unsigned bgn, unsigned cnt)
  {
    if (cnt == 0 || Elements.empty()) { return; }
    
    unsigned end = bgn+cnt;
    unsigned bgn_idx = bgn/ElementSize, end_idx = (end-1)/ElementSize;
    ElementListIter it = FindLowerBound(bgn_idx), k;

    if (it == Elements.end()) { return; }
    if (it->index() < bgn_idx) { ++it; }

    while (it != Elements.end() && it->index() <= end_idx) {
      unsigned el_bgn = it->index() * ElementSize;
      unsigned el_end = el_bgn + ElementSize;

      if (el_bgn >= bgn && el_end <= end) {
	k = it; ++it; eraseElement(k);
      }
      else {
	unsigned bgn_word, end_word;
	BitWord bgn_msk, end_msk, *bits = it->getBits();

	if (el_bgn <= bgn) {
	  bgn_word = (bgn - el_bgn) / WORDSZ;
	  bgn_msk = ~((1 << (bgn % WORDSZ)) - 1);
	}
	else { bgn_word = 0; bgn_msk = ~0U; }

	if (el_end <= end) { end_word = NWORDS-1; end_msk = ~0U; }
	else {
	  end_word = (end - el_bgn) / WORDSZ;
	  end_msk = (1 << (end % WORDSZ)) - 1;
	}

	if (bgn_word == end_word) { bits[bgn_word] &= ~(bgn_msk & end_msk); }
	else {
	  bits[bgn_word] &= ~bgn_msk;
	  bits[end_word] &= ~end_msk;
	  for (unsigned i = bgn_word+1; i < end_word; ++i) { bits[i] = 0; }
	}

	if (it->empty()) { k = it; ++it; eraseElement(k); }
	else { ++it; }
      }
    }

    CurrElementIter = it;
  }

  // set the bits [bgn..bgn+cnt), taken largely from GCC's bitmap.c
  void set_range(unsigned bgn, unsigned cnt)
  {
    if (cnt == 0) { return; }

    unsigned end = bgn+cnt;
    unsigned bgn_idx = bgn/ElementSize, end_idx = (end-1)/ElementSize;
    ElementListIter it = FindLowerBound(bgn_idx);

    if (it != Elements.end() && it->index() < bgn_idx) { ++it; }

    for (unsigned i = bgn_idx; i <= end_idx; ++i, ++it) {
      if (it == Elements.end() || it->index() != i) {
	it = Elements.insert(it,getNewElement(i));
      }

      unsigned el_bgn = i * ElementSize;
      unsigned el_end = el_bgn + ElementSize;

      unsigned bgn_word, end_word;
      BitWord bgn_msk, end_msk, *bits = it->getBits();

      if (el_bgn <= bgn) {
	bgn_word = (bgn - el_bgn) / WORDSZ;
	bgn_msk = ~((1 << (bgn % WORDSZ)) - 1);
      }
      else { bgn_word = 0; bgn_msk = ~0U; }

      if (el_end <= end) { end_word = NWORDS-1; end_msk = ~0U; }
      else {
	end_word = (end - el_bgn) / WORDSZ;
	end_msk = (1 << (end % WORDSZ)) - 1;
      }

      if (bgn_word == end_word) { bits[bgn_word] |= (bgn_msk & end_msk); }
      else {
	bits[bgn_word] |= bgn_msk;
	bits[end_word] |= end_msk;
	for (unsigned j = bgn_word+1; j < end_word; ++j) { bits[j] = ~0U; }
      }      
    }

    CurrElementIter = it;
  }

  unsigned num_els() { return Elements.size(); }

  static unsigned stack_size() { return alloc_stack.size(); }
};
  
template<unsigned ElementSize> std::stack<MySparseBitVectorElement<ElementSize>*> MySparseBitVector<ElementSize>::alloc_stack;


// Convenience functions to allow Or and And without dereferencing in
// the user code.

template <unsigned ElementSize>
inline bool operator |=(MySparseBitVector<ElementSize> &LHS,
                        const MySparseBitVector<ElementSize> *RHS) {
  return LHS |= *RHS;
}

template <unsigned ElementSize>
inline bool operator |=(MySparseBitVector<ElementSize> *LHS,
                        const MySparseBitVector<ElementSize> &RHS) {
  return LHS->operator|=(RHS);
}

template <unsigned ElementSize>
inline bool operator &=(MySparseBitVector<ElementSize> *LHS,
                        const MySparseBitVector<ElementSize> &RHS) {
  return LHS->operator&=(RHS);
}

template <unsigned ElementSize>
inline bool operator &=(MySparseBitVector<ElementSize> &LHS,
                        const MySparseBitVector<ElementSize> *RHS) {
  return LHS &= (*RHS);
}


// Dump a MySparseBitVector to a stream
template <unsigned ElementSize>
void dump(const MySparseBitVector<ElementSize> &LHS, llvm::raw_ostream &out) {
  out << "[ ";
  
  typename MySparseBitVector<ElementSize>::iterator bi;
  for (bi = LHS.begin(); bi != LHS.end(); ++bi) {
    out << *bi << " ";
  }
  out << " ]\n";
}

} // end llvm namespace


#endif