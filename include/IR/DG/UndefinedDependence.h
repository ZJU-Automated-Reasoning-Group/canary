
#ifndef DG_UNDEFINED_DEPENDENCE_H_
#define DG_UNDEFINED_DEPENDENCE_H_

#include "IR/DG/DGEdge.h"


template <class T, class SubT>
class UndefinedDependence : public DGEdge<T, SubT> {
public:
  UndefinedDependence(DGNode<T> *src, DGNode<T> *dst);

  UndefinedDependence(const UndefinedDependence<T, SubT> &edgeToCopy);

  UndefinedDependence() = delete;

  std::string toString(void) override;

  static bool classof(const DGEdge<T, SubT> *s);
};

template <class T, class SubT>
UndefinedDependence<T, SubT>::UndefinedDependence(DGNode<T> *src,
                                                  DGNode<T> *dst)
  : DGEdge<T, SubT>(DGEdge<T, SubT>::DependenceKind::UNDEFINED_DEPENDENCE,
                    src,
                    dst) {
  return;
}

template <class T, class SubT>
UndefinedDependence<T, SubT>::UndefinedDependence(
    const UndefinedDependence<T, SubT> &edgeToCopy)
  : DGEdge<T, SubT>(edgeToCopy) {
  return;
}

template <class T, class SubT>
std::string UndefinedDependence<T, SubT>::toString(void) {
  if (this->getNumberOfSubEdges() > 0) {
    std::string edgesStr;
    raw_string_ostream ros(edgesStr);
    for (auto edge : this->getSubEdges()) {
      ros << edge->toString();
    }
    return ros.str();
  }
  std::string edgeStr;
  raw_string_ostream ros(edgeStr);
  ros << "Attributes: ";
  if (this->isLoopCarriedDependence()) {
    ros << "Loop-carried ";
  }
  ros << "Undefined\n";
  ros.flush();
  return edgeStr;
}

template <class T, class SubT>
bool UndefinedDependence<T, SubT>::classof(const DGEdge<T, SubT> *s) {
  return (s->getKind()
          == DGEdge<T, SubT>::DependenceKind::UNDEFINED_DEPENDENCE);
}

#endif
