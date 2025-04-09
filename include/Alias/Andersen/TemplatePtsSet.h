#ifndef ANDERSEN_TEMPLATE_PTSSET_H
#define ANDERSEN_TEMPLATE_PTSSET_H

#include "Alias/Andersen/PtsSet.h"
#include "Alias/Andersen/BDDPtsSet.h"

// An enumeration for the available points-to set implementations
enum class PtsSetImpl {
  SPARSE_BITVECTOR,
  BDD
};

// Template class for points-to sets
// The implementation type is selected at compile time
template <PtsSetImpl Impl = PtsSetImpl::SPARSE_BITVECTOR>
class TemplatePtsSet {
};

// Specialization for SparseBitVector implementation
template <>
class TemplatePtsSet<PtsSetImpl::SPARSE_BITVECTOR> : public AndersPtsSet {
public:
  using AndersPtsSet::AndersPtsSet;
};

// Specialization for BDD implementation
template <>
class TemplatePtsSet<PtsSetImpl::BDD> : public BDDAndersPtsSet {
public:
  using BDDAndersPtsSet::BDDAndersPtsSet;
};

// This typedef controls which implementation is used by default
// To switch to BDD implementation, change this to:
// using DefaultPtsSet = TemplatePtsSet<PtsSetImpl::BDD>;
using DefaultPtsSet = TemplatePtsSet<PtsSetImpl::SPARSE_BITVECTOR>;

#endif // ANDERSEN_TEMPLATE_PTSSET_H 