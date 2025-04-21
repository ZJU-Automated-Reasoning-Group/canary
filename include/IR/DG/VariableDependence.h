#ifndef DG_VARIABLE_DEPENDENCE_H_
#define DG_VARIABLE_DEPENDENCE_H_

#include "IR/DG/DataDependence.h"


template <class T, class SubT>
class VariableDependence : public DataDependence<T, SubT> {
public:
  VariableDependence(DGNode<T> *src, DGNode<T> *dst, DataDependenceType t);

  VariableDependence(const VariableDependence<T, SubT> &edgeToCopy);

  VariableDependence() = delete;

  std::string toString(void) override;

  static bool classof(const DGEdge<T, SubT> *s);
};

template <class T, class SubT>
VariableDependence<T, SubT>::VariableDependence(DGNode<T> *src,
                                                DGNode<T> *dst,
                                                DataDependenceType t)
  : DataDependence<T, SubT>(
      DGEdge<T, SubT>::DependenceKind::VARIABLE_DEPENDENCE,
      src,
      dst,
      t) {
  return;
}

template <class T, class SubT>
VariableDependence<T, SubT>::VariableDependence(
    const VariableDependence<T, SubT> &edgeToCopy)
  : DataDependence<T, SubT>(edgeToCopy) {
  return;
}

template <class T, class SubT>
std::string VariableDependence<T, SubT>::toString(void) {
  if (this->getNumberOfSubEdges() > 0) {
    std::string edgesStr;
    raw_string_ostream ros(edgesStr);
    for (auto edge : this->getSubEdges())
      ros << edge->toString();
    return ros.str();
  }
  std::string edgeStr;
  raw_string_ostream ros(edgeStr);
  ros << "Attributes: ";
  if (this->isLoopCarriedDependence()) {
    ros << "Loop-carried ";
  }
  ros << "Data ";
  ros << this->dataDepToString();
  ros << "\n";
  ros.flush();
  return edgeStr;
}

template <class T, class SubT>
bool VariableDependence<T, SubT>::classof(const DGEdge<T, SubT> *s) {
  auto sKind = s->getKind();
  return (sKind == DGEdge<T, SubT>::DependenceKind::VARIABLE_DEPENDENCE);
}

#endif
