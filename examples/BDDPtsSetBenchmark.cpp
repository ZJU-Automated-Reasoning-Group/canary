#include "Alias/Andersen/PtsSet.h"
#include "Alias/Andersen/BDDPtsSet.h"
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

// Benchmark various operations on points-to sets
template <typename PtsSetType>
void benchmarkPtsSet(const std::string &name, int numNodes, int numOperations) {
  std::cout << "Benchmarking " << name << " with " << numNodes << " nodes and "
            << numOperations << " operations" << std::endl;

  // Create a random number generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, numNodes - 1);

  // Create vectors to hold the points-to sets
  std::vector<PtsSetType> ptsSets(numOperations);

  // Time the insert operation
  auto startInsert = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < numOperations; ++i) {
    // Insert random elements into each set
    int numElements = dis(gen) % 100 + 1; // 1-100 elements
    for (int j = 0; j < numElements; ++j) {
      ptsSets[i].insert(dis(gen));
    }
  }
  auto endInsert = std::chrono::high_resolution_clock::now();
  auto insertTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      endInsert - startInsert);
  std::cout << "  Insert time: " << insertTime.count() << " ms" << std::endl;

  // Time the has operation
  auto startHas = std::chrono::high_resolution_clock::now();
  int hasCount = 0;
  for (int i = 0; i < numOperations; ++i) {
    for (int j = 0; j < 100; ++j) {
      if (ptsSets[i].has(dis(gen))) {
        hasCount++;
      }
    }
  }
  auto endHas = std::chrono::high_resolution_clock::now();
  auto hasTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(endHas - startHas);
  std::cout << "  Has time: " << hasTime.count() << " ms (found " << hasCount
            << " elements)" << std::endl;

  // Time the unionWith operation
  auto startUnion = std::chrono::high_resolution_clock::now();
  int unionCount = 0;
  for (int i = 0; i < numOperations - 1; ++i) {
    if (ptsSets[i].unionWith(ptsSets[i + 1])) {
      unionCount++;
    }
  }
  auto endUnion = std::chrono::high_resolution_clock::now();
  auto unionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      endUnion - startUnion);
  std::cout << "  Union time: " << unionTime.count() << " ms (changed "
            << unionCount << " sets)" << std::endl;

  // Time the intersectWith operation
  auto startIntersect = std::chrono::high_resolution_clock::now();
  int intersectCount = 0;
  for (int i = 0; i < numOperations - 1; ++i) {
    if (ptsSets[i].intersectWith(ptsSets[i + 1])) {
      intersectCount++;
    }
  }
  auto endIntersect = std::chrono::high_resolution_clock::now();
  auto intersectTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      endIntersect - startIntersect);
  std::cout << "  Intersect time: " << intersectTime.count()
            << " ms (found " << intersectCount << " intersections)" << std::endl;

  // Measure memory usage (not accurate)
  std::cout << "  Approx memory per set: " << sizeof(PtsSetType) << " bytes + contents" << std::endl;

  // Calculate average set size
  size_t totalSize = 0;
  for (const auto &pts : ptsSets) {
    totalSize += pts.getSize();
  }
  std::cout << "  Average set size: " << (totalSize / numOperations) << " elements" << std::endl;

  std::cout << std::endl;
}

int main(int argc, char *argv[]) {
  // Default values
  int numNodes = 10000;
  int numOps = 1000;

  // Parse command line arguments
  if (argc > 1) {
    numNodes = std::stoi(argv[1]);
  }
  if (argc > 2) {
    numOps = std::stoi(argv[2]);
  }

  std::cout << "Running points-to set benchmarks" << std::endl;
  std::cout << "================================" << std::endl;

  // Benchmark SparseBitVector-based implementation
  benchmarkPtsSet<AndersPtsSet>("SparseBitVector", numNodes, numOps);

  // Benchmark BDD-based implementation
  benchmarkPtsSet<BDDAndersPtsSet>("BDD", numNodes, numOps);

  return 0;
} 