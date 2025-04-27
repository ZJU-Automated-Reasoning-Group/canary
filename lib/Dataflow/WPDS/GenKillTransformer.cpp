#include "Dataflow/WPDS/InterProceduralDataFlow.h"

namespace dataflow {

GenKillTransformer::GenKillTransformer() 
    : count(0), kill(DataFlowFacts::EmptySet()), gen(DataFlowFacts::EmptySet()) {
}

GenKillTransformer::GenKillTransformer(const DataFlowFacts& kill, const DataFlowFacts& gen) 
    : count(0), kill(DataFlowFacts::Diff(kill, gen)), gen(gen) {
}

GenKillTransformer::GenKillTransformer(const DataFlowFacts& k, const DataFlowFacts& g, int) 
    : count(0), kill(k), gen(g) {
}

GenKillTransformer* GenKillTransformer::makeGenKillTransformer(
    const DataFlowFacts& kill, 
    const DataFlowFacts& gen) {
    
    DataFlowFacts k_normalized = DataFlowFacts::Diff(kill, gen);
    
    if (DataFlowFacts::Eq(k_normalized, DataFlowFacts::EmptySet()) && 
        DataFlowFacts::Eq(gen, DataFlowFacts::UniverseSet())) {
        return GenKillTransformer::bottom();
    }
    else if (DataFlowFacts::Eq(k_normalized, DataFlowFacts::EmptySet()) && 
             DataFlowFacts::Eq(gen, DataFlowFacts::EmptySet())) {
        return GenKillTransformer::one();
    }
    else {
        return new GenKillTransformer(k_normalized, gen);
    }
}

GenKillTransformer* GenKillTransformer::one() {
    static GenKillTransformer* ONE =
        new GenKillTransformer(DataFlowFacts::EmptySet(), DataFlowFacts::EmptySet(), 1);
    return ONE;
}

GenKillTransformer* GenKillTransformer::zero() {
    static GenKillTransformer* ZERO =
        new GenKillTransformer(DataFlowFacts::UniverseSet(), DataFlowFacts::EmptySet(), 1);
    return ZERO;
}

GenKillTransformer* GenKillTransformer::bottom() {
    static GenKillTransformer* BOTTOM = 
        new GenKillTransformer(DataFlowFacts::EmptySet(), DataFlowFacts::UniverseSet(), 1);
    return BOTTOM;
}

GenKillTransformer* GenKillTransformer::extend(GenKillTransformer* y) {
    // Special cases
    if (equal(GenKillTransformer::zero()) || y->equal(GenKillTransformer::zero())) {
        return GenKillTransformer::zero();
    }
    
    if (equal(GenKillTransformer::one())) {
        return y;
    }
    
    if (y->equal(GenKillTransformer::one())) {
        return this;
    }
    
    // General case: (f∘g)(x) = f(g(x))
    DataFlowFacts temp_k = DataFlowFacts::Union(kill, y->kill);
    DataFlowFacts temp_g = DataFlowFacts::Union(
        DataFlowFacts::Diff(gen, y->kill), 
        y->gen
    );
    
    return makeGenKillTransformer(temp_k, temp_g);
}

GenKillTransformer* GenKillTransformer::combine(GenKillTransformer* y) {
    // Special cases
    if (equal(GenKillTransformer::zero())) {
        return y;
    }
    
    if (y->equal(GenKillTransformer::zero())) {
        return this;
    }
    
    // General case: join operation
    DataFlowFacts temp_k = DataFlowFacts::Intersect(kill, y->kill);
    DataFlowFacts temp_g = DataFlowFacts::Union(gen, y->gen);
    
    return makeGenKillTransformer(temp_k, temp_g);
}

GenKillTransformer* GenKillTransformer::diff(GenKillTransformer* y) {
    // Special cases
    if (equal(GenKillTransformer::zero())) {
        return GenKillTransformer::zero();
    }
    
    if (y->equal(GenKillTransformer::zero())) {
        return this;
    }
    
    // General case
    DataFlowFacts temp_k = DataFlowFacts::Diff(
        DataFlowFacts::UniverseSet(),
        DataFlowFacts::Diff(y->kill, kill)
    );
    DataFlowFacts temp_g = DataFlowFacts::Diff(gen, y->gen);
    
    // Test if *this >= *y
    if (DataFlowFacts::Eq(temp_k, DataFlowFacts::UniverseSet()) && 
        DataFlowFacts::Eq(temp_g, DataFlowFacts::EmptySet())) {
        return GenKillTransformer::zero();
    }
    
    return makeGenKillTransformer(temp_k, temp_g);
}

GenKillTransformer* GenKillTransformer::quasiOne() const {
    return one();
}

bool GenKillTransformer::equal(GenKillTransformer* y) const {
    // Handle special values
    if (this == one() && y == one()) return true;
    if (this == zero() && y == zero()) return true;
    if (this == bottom() && y == bottom()) return true;
    
    if ((this == one() && y != one()) ||
        (this == zero() && y != zero()) ||
        (this == bottom() && y != bottom())) {
        return false;
    }
    
    // Compare gen and kill sets
    return DataFlowFacts::Eq(kill, y->kill) && DataFlowFacts::Eq(gen, y->gen);
}

DataFlowFacts GenKillTransformer::apply(const DataFlowFacts& input) {
    // Apply kill and gen operation: (input - kill) ∪ gen
    DataFlowFacts result = DataFlowFacts::Diff(input, kill);
    return DataFlowFacts::Union(result, gen);
}

const DataFlowFacts& GenKillTransformer::getKill() const {
    return kill;
}

const DataFlowFacts& GenKillTransformer::getGen() const {
    return gen;
}

std::ostream& GenKillTransformer::print(std::ostream& os) const {
    os << "GenKillTransformer{kill=";
    kill.print(os);
    os << ", gen=";
    gen.print(os);
    os << "}";
    return os;
}

} // namespace dataflow 