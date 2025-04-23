/*
* @author: rainoftime
* @brief: ImmutableMap is a map data structure that is immutable.
* 一个不可变的映射实现，是其他不可变容器的基础。它提供了高效的查找、插入和删除操作，同时保持了树的平衡性。它是一个函数式数据结构，操作不会修改原有结构，而是返回新的结构。
*/

#pragma once

#include <functional>

#include "Support/ADT/ImmutableTree.h"

template<class V, class D>
struct _Select1st {
  D &operator()(V &a) const { return a.first; }
  const D &operator()(const V &a) const { return a.first; }
};

template<class K, class D, class CMP=std::less<K> >
class ImmutableMap {
public:
  typedef K key_type;
  typedef std::pair<K,D> value_type;

  typedef ImmutableTree<K, value_type, _Select1st<value_type,key_type>, CMP> Tree;
  typedef typename Tree::iterator iterator;

private:
  Tree elts;

  ImmutableMap(const Tree &b): elts(b) {}

public:
  ImmutableMap() {}
  ImmutableMap(const ImmutableMap &b) : elts(b.elts) {}
  ~ImmutableMap() {}

  ImmutableMap &operator=(const ImmutableMap &b) { elts = b.elts; return *this; }
  
  bool empty() const { 
    return elts.empty(); 
  }
  size_t count(const key_type &key) const { 
    return elts.count(key); 
  }
  const value_type *lookup(const key_type &key) const { 
    return elts.lookup(key); 
  }
  const value_type *lookup_previous(const key_type &key) const { 
    return elts.lookup_previous(key); 
  }
  const value_type &min() const { 
    return elts.min(); 
  }
  const value_type &max() const { 
    return elts.max(); 
  }
  size_t size() const { 
    return elts.size(); 
  }

  ImmutableMap insert(const value_type &value) const { 
    return elts.insert(value); 
  }
  ImmutableMap replace(const value_type &value) const { 
    return elts.replace(value); 
  }
  ImmutableMap remove(const key_type &key) const { 
    return elts.remove(key); 
  }
  ImmutableMap popMin(const value_type &valueOut) const { 
    return elts.popMin(valueOut); 
  }
  ImmutableMap popMax(const value_type &valueOut) const { 
    return elts.popMax(valueOut); 
  }

  iterator begin() const { 
    return elts.begin(); 
  }
  iterator end() const { 
    return elts.end(); 
  }
  iterator find(const key_type &key) const { 
    return elts.find(key); 
  }
  iterator lower_bound(const key_type &key) const { 
    return elts.lower_bound(key); 
  }
  iterator upper_bound(const key_type &key) const { 
    return elts.upper_bound(key); 
  }

  static size_t getAllocated() { return Tree::allocated; }
};

