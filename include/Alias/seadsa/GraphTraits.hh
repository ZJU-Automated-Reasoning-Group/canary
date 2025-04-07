#ifndef __DSA_GRAPH_TRAITS_H
#define __DSA_GRAPH_TRAITS_H

#include "Alias/seadsa/Graph.hh"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/STLExtras.h"

#include <iterator>

namespace seadsa {
    
  template<typename NodeTy>
  class NodeIterator {
    
    friend class Node;
    
    typename Node::links_type::const_iterator _links_it;
    
    typedef NodeIterator<NodeTy> this_type;
    
    NodeIterator(NodeTy *N) : _links_it(N->links().begin()) {}   // begin iterator
    NodeIterator(NodeTy *N, bool) : _links_it(N->links().end()) {}  // Create end iterator
    
  public:
    typedef std::forward_iterator_tag iterator_category;
    typedef Node value_type;
    typedef Node* pointer;
    typedef Node& reference;
    typedef std::ptrdiff_t difference_type;
    
    bool operator==(const this_type& x) const {
      return _links_it == x._links_it;
    }
    
    bool operator!=(const this_type& x) const { return !operator==(x); }
    
    NodeIterator(const NodeIterator&) = default;

    const this_type &operator=(const this_type &o) {
      _links_it = o._links_it;
      return *this;
    }
    
    pointer operator*() const {
      return _links_it->second->getNode();
    }
    
    pointer operator->() const { return operator*(); }
    
    this_type& operator++() {                // Preincrement
      ++_links_it;
      return *this;
    }
    
    this_type operator++(int) { // Postincrement
      this_type tmp = *this; ++*this; return tmp;
    }

    Field getField() const { return _links_it->first; }
    const Cell &getCell() const {
      assert(_links_it->second);
      return *_links_it->second;
    }
  };
  
  
// Provide iterators for Node...
inline Node::iterator Node::begin() { return Node::iterator(this); }
  
inline Node::iterator Node::end() { return Node::iterator(this, false); }
  
inline Node::const_iterator Node::begin() const { return Node::const_iterator(this); }
  
inline Node::const_iterator Node::end() const { return Node::const_iterator(this, false); }
  
} // end namespace seadsa 

namespace llvm {
  
  template <> struct GraphTraits<seadsa::Node*> {
    typedef seadsa::Node NodeType;
    typedef seadsa::Node::iterator ChildIteratorType;
    
    static NodeType *getEntryNode(NodeType *N) { return N; }
    static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
    static ChildIteratorType child_end(NodeType *N) { return N->end(); }
  };

  template <> struct GraphTraits<const seadsa::Node*> {
    typedef const seadsa::Node NodeType;
    typedef seadsa::Node::const_iterator ChildIteratorType;
    
    static NodeType *getEntryNode(NodeType *N) { return N; }
    static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
    static ChildIteratorType child_end(NodeType *N) { return N->end(); }
  };

  template <> struct GraphTraits<seadsa::Graph*> {
    typedef seadsa::Node NodeType;
    typedef seadsa::Node::iterator ChildIteratorType;
    
    // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
    typedef seadsa::Graph::iterator nodes_iterator;
    
    static nodes_iterator nodes_begin(seadsa::Graph *G) { return G->begin(); }
    static nodes_iterator nodes_end(seadsa::Graph *G) { return G->end(); }
    
    static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
    static ChildIteratorType child_end(NodeType *N) { return N->end(); }
  };

  template <> struct GraphTraits<const seadsa::Graph*> {
    typedef const seadsa::Node NodeType;
    typedef seadsa::Node::const_iterator ChildIteratorType;
    
    // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
    typedef seadsa::Graph::const_iterator nodes_iterator;
    
    static nodes_iterator nodes_begin(const seadsa::Graph *G) { return G->begin(); }
    static nodes_iterator nodes_end(const seadsa::Graph *G) { return G->end(); }
    
    static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
    static ChildIteratorType child_end(NodeType *N) { return N->end(); }
  };
  
  /// Iterator to traverse all the nodes of a given graph.
  template <typename Set>
  class nodes_iterator_tpl {
    typedef typename Set::iterator iterator;
    typedef typename Set::value_type value_type;

    iterator m_it;
    iterator m_end;

  public:
    typedef value_type *pointer;
    typedef value_type &reference;
    typedef std::forward_iterator_tag iterator_category;
    typedef std::ptrdiff_t difference_type;

    nodes_iterator_tpl() {}
    nodes_iterator_tpl(iterator it, iterator end) : m_it(it), m_end(end) {}

    bool operator==(const nodes_iterator_tpl<Set> &o) const {
      return m_it == o.m_it;
    }
    bool operator!=(const nodes_iterator_tpl<Set> &o) const {
      return m_it != o.m_it;
    }
    void operator++() { ++m_it; }
    reference operator*() const { return *m_it; }
    pointer operator->() const { return m_it.operator->(); }
  };
  
  // Just use Graph's built-in iterators for nodes
  typedef seadsa::Graph::iterator nodes_iterator;
} // End llvm namespace

#endif
