
#ifndef DG_MEMORY_DEPENDENCE_H_
#define DG_MEMORY_DEPENDENCE_H_

#include "IR/DG/DataDependence.h"


template <class T, class SubT>
class MemoryDependence : public DataDependence<T, SubT> {
public:
  MemoryDependence() = delete;

  static bool classof(const DGEdge<T, SubT> *s);

protected:
  MemoryDependence(typename DGEdge<T, SubT>::DependenceKind k,
                   DGNode<T> *src,
                   DGNode<T> *dst,
                   DataDependenceType t);

  MemoryDependence(const MemoryDependence<T, SubT> &edgeToCopy);
};

template <class T, class SubT>
MemoryDependence<T, SubT>::MemoryDependence(
    typename DGEdge<T, SubT>::DependenceKind k,
    DGNode<T> *src,
    DGNode<T> *dst,
    DataDependenceType t)
  : DataDependence<T, SubT>(k, src, dst, t) {
  return;
}

template <class T, class SubT>
MemoryDependence<T, SubT>::MemoryDependence(
    const MemoryDependence<T, SubT> &edgeToCopy)
  : DataDependence<T, SubT>(edgeToCopy) {
  return;
}

template <class T, class SubT>
bool MemoryDependence<T, SubT>::classof(const DGEdge<T, SubT> *s) {
  auto sKind = s->getKind();
  return (sKind >= DGEdge<T, SubT>::DependenceKind::FIRST_MEMORY_DEPENDENCE)
         && (sKind <= DGEdge<T, SubT>::DependenceKind::LAST_MEMORY_DEPENDENCE);
}

#endif
