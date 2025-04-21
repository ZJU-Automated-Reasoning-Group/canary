#ifndef DG_CONTROL_DEPENDENCE_H_
#define DG_CONTROL_DEPENDENCE_H_

#include "IR/DG/DGEdge.h"


template <class T, class SubT>
class ControlDependence : public DGEdge<T, SubT> {
public:
  ControlDependence(DGNode<T> *src, DGNode<T> *dst);

  ControlDependence(const ControlDependence<T, SubT> &edgeToCopy);

  ControlDependence() = delete;

  std::string toString(void) override;

  static bool classof(const DGEdge<T, SubT> *s);
};

template <class T, class SubT>
ControlDependence<T, SubT>::ControlDependence(DGNode<T> *src, DGNode<T> *dst)
  : DGEdge<T, SubT>(DGEdge<T, SubT>::DependenceKind::CONTROL_DEPENDENCE,
                    src,
                    dst) {
  return;
}

template <class T, class SubT>
ControlDependence<T, SubT>::ControlDependence(
    const ControlDependence<T, SubT> &edgeToCopy)
  : DGEdge<T, SubT>(edgeToCopy) {
  return;
}

template <class T, class SubT>
std::string ControlDependence<T, SubT>::toString(void) {
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
  ros << "Control\n";
  ros.flush();
  return edgeStr;
}

template <class T, class SubT>
bool ControlDependence<T, SubT>::classof(const DGEdge<T, SubT> *s) {
  return (s->getKind() == DGEdge<T, SubT>::DependenceKind::CONTROL_DEPENDENCE);
}

#endif
