#ifndef BITMATRIX_H
#define BITMATRIX_H

#include "Support/SystemHeaders.h"

namespace llvm {

// BitMatrix is a NxN bit-matrix that depicts whether a relation R
// holds for a pair with indices (i,j) (i.e., R(i,j) = 0/1)
// BitMatrix is intended for a dense, asymmetric relation R.
struct BitMatrix {
  BitMatrix(uint32_t n = 1) : N(n), bv(n * n) {}

  // Returns the size of BitVector
  uint32_t count() const {
    return bv.count();
  }

  // Specifies that row is related to col, i.e., R(row,col) = 1
  void set(uint32_t row, uint32_t col, bool v = true) {
    const uint32_t i = idx(row, col);

    if (v) {
      bv.set(i);
    } else {
      bv.reset(i);
    }
  }

  // Checks whether row is related to col,
  // i.e., R(row,col) == 1 (R is not symmetric)
  bool test(uint32_t row, uint32_t col) const {
    const uint32_t i = idx(row, col);

    return bv.test(i);
  }

  // Resizes matrix to nxn
  void resize(uint32_t n) {
      N = n;
      bv.clear();
      bv.resize(n * n);
  }

  // Computes the transitive closure.
  // For example, given a adjacency matrix, it converts it to a connectivity
  // matrix, where (i,j) is set if there is a directed path from i to j

void transitiveClosure() {
  uint32_t nAdd = 0;

  typedef std::list<uint32_t> Worklist;
  Worklist worklist;
  for (uint32_t i = 0; i < N; ++i) {
    worklist.push_back(i);
  }

  while (!worklist.empty()) {
    uint32_t i = worklist.front();
    worklist.pop_front();

    bool changedI = false;

    // (i->j)
    for (int32_t j = firstSuccessor(i); j != -1; j = nextSuccessor(i, j)) {
      // bit-wise or row[i] |= row[j]

      // (j->k)
      for (int32_t k = firstSuccessor(j); k != -1; k = nextSuccessor(j, k)) {
        // but not (i->k)
        if (!test(i, k)) {
          changedI = true;
          ++nAdd;
          set(i, k);
        }
      }
    }

    if (changedI) {
      for (uint32_t p = 0; p < N; ++p) {
        if (test(p, i)) {
          if (std::find(worklist.begin(), worklist.end(), p)
              == worklist.end()) {
            worklist.push_back(p);
            }
          }
        }
      }
    }
  }


  // Emits to fout the BitMatrix
  void dump(raw_ostream &fout) const {

      for (uint32_t row = 0; row < N; ++row) {
    for (uint32_t col = 0; col < N; ++col) {
      if (test(row, col)) {
        fout << '#';
      } else {
        fout << '.';
        }
      }
      fout << '\n';
    }
  }

private:
  uint32_t N;
  BitVector bv;

  // For a given row returns the first col that is set.
  // Returns -1 if none found.
  int32_t firstSuccessor(uint32_t row) const {
    const uint32_t rowBegin = N * row;
    int32_t next = -1;

    if (0 == rowBegin) {
      next = bv.find_first();
    } else {
      next = bv.find_next(rowBegin - 1);
    }

  if (-1 == next) {
    return -1;
  }

  const uint32_t rowEnd = rowBegin + N;
  if (((uint32_t)next) >= rowEnd) {
    return -1;
  }

    return next - rowBegin;
  }

  // For a given row returns the first col after prev (col>prev) that is set.
  // Returns -1 if none found.
  int32_t nextSuccessor(uint32_t row, uint32_t prev) const {
    int32_t next = bv.find_next(N * row + prev);
    if (-1 == next) {
      return -1;
    }

    const uint32_t rowBegin = N * row;
    const uint32_t rowEnd = rowBegin + N;
    if (((uint32_t)next) >= rowEnd) {
      return -1;
    }

    return next - rowBegin;
  }

  // Returns the index corresponding to a pair (row.col)
  // i.e., idx = row * N + col
  uint32_t idx(uint32_t row, uint32_t col) const {
    assert(row < N);
    assert(col < N);
    return row * N + col;
  }
};

} // namespace llvm

#endif
