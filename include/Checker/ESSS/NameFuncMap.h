#pragma once

#include <llvm/IR/Function.h>
#include <unordered_map>
#include <string>

using namespace llvm;

// Custom map using std::string as keys instead of std::string_view
// to solve StringRef compatibility issues
class NameFuncMap {
private:
    std::unordered_map<std::string, Function*> map;
    std::unordered_map<uint64_t, Function*> guidMap;  // Map for GUID keys

public:
    // Insert or update with string key
    Function*& operator[](const StringRef& key) {
        return map[key.str()];
    }

    // Insert or update with GUID key
    Function*& operator[](uint64_t guid) {
        return guidMap[guid];
    }

    // Find with string key
    Function* find(const StringRef& key) const {
        auto it = map.find(key.str());
        if (it != map.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Find with GUID key
    Function* find(uint64_t guid) const {
        auto it = guidMap.find(guid);
        if (it != guidMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Clear
    void clear() {
        map.clear();
        guidMap.clear();
    }

    // Size
    size_t size() const {
        return map.size() + guidMap.size();
    }

    // Begin/end for iteration
    auto begin() { return map.begin(); }
    auto end() { return map.end(); }
    auto begin() const { return map.begin(); }
    auto end() const { return map.end(); }
}; 