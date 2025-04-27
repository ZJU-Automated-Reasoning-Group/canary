#include "Dataflow/WPDS/InterProceduralDataFlow.h"
#include <algorithm>

namespace dataflow {

// Initialize the static universe set
std::set<Value*> DataFlowFacts::universe;

DataFlowFacts::DataFlowFacts() = default;

DataFlowFacts::DataFlowFacts(const std::set<Value*>& facts)
    : facts(facts) {
}

DataFlowFacts::DataFlowFacts(const DataFlowFacts& other)
    : facts(other.facts) {
}

DataFlowFacts& DataFlowFacts::operator=(const DataFlowFacts& other) {
    if (this != &other) {
        facts = other.facts;
    }
    return *this;
}

bool DataFlowFacts::operator==(const DataFlowFacts& other) const {
    return facts == other.facts;
}

DataFlowFacts DataFlowFacts::EmptySet() {
    return DataFlowFacts();
}

DataFlowFacts DataFlowFacts::UniverseSet() {
    return DataFlowFacts(universe);
}

DataFlowFacts DataFlowFacts::Union(const DataFlowFacts& x, const DataFlowFacts& y) {
    DataFlowFacts result = x;
    result.facts.insert(y.facts.begin(), y.facts.end());
    return result;
}

DataFlowFacts DataFlowFacts::Intersect(const DataFlowFacts& x, const DataFlowFacts& y) {
    DataFlowFacts result;
    std::set_intersection(
        x.facts.begin(), x.facts.end(),
        y.facts.begin(), y.facts.end(),
        std::inserter(result.facts, result.facts.begin())
    );
    return result;
}

DataFlowFacts DataFlowFacts::Diff(const DataFlowFacts& x, const DataFlowFacts& y) {
    DataFlowFacts result;
    std::set_difference(
        x.facts.begin(), x.facts.end(),
        y.facts.begin(), y.facts.end(),
        std::inserter(result.facts, result.facts.begin())
    );
    return result;
}

bool DataFlowFacts::Eq(const DataFlowFacts& x, const DataFlowFacts& y) {
    return x.facts == y.facts;
}

const std::set<Value*>& DataFlowFacts::getFacts() const {
    return facts;
}

void DataFlowFacts::addFact(Value* val) {
    facts.insert(val);
    // Also add to universe set
    universe.insert(val);
}

void DataFlowFacts::removeFact(Value* val) {
    facts.erase(val);
}

bool DataFlowFacts::containsFact(Value* val) const {
    return facts.find(val) != facts.end();
}

std::size_t DataFlowFacts::size() const {
    return facts.size();
}

bool DataFlowFacts::isEmpty() const {
    return facts.empty();
}

std::ostream& DataFlowFacts::print(std::ostream& os) const {
    os << "DataFlowFacts{";
    bool first = true;
    for (auto* val : facts) {
        if (!first) {
            os << ", ";
        }
        first = false;
        
        if (val == nullptr) {
            os << "null";
            continue;
        }
        
        if (auto* inst = dyn_cast<Instruction>(val)) {
            os << inst->getName().str();
            if (inst->getName().empty()) {
                os << "<unnamed-inst>";
            }
        } else if (auto* arg = dyn_cast<Argument>(val)) {
            os << arg->getName().str();
            if (arg->getName().empty()) {
                os << "<unnamed-arg>";
            }
        } else if (auto* global = dyn_cast<GlobalValue>(val)) {
            os << global->getName().str();
        } else {
            os << "<unknown-value>";
        }
    }
    os << "}";
    return os;
}

} // namespace dataflow 