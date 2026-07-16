#pragma once
#include <vector>
#include <iostream>
#include <cstdlib>
#include "memory_config.hpp"

// Generic LRU/MRU replacement policy shared by the data cache (CacheLine)
// and the instruction cache (ICacheLine). Both line types expose `valid`,
// `tag` and `counter`, which is all this policy needs, so a single
// templated implementation replaces what used to be two near-identical
// copies of these functions (one plain, one with an `_I` suffix).

template <typename Line>
int getLRUWay(std::vector<Line>& set) {
    int lru_way = 0;
    int max_counter = -1;
    for (size_t way = 0; way < set.size(); ++way) {
        if (!set[way].valid) return static_cast<int>(way);
        if (set[way].counter > max_counter) {
            max_counter = set[way].counter;
            lru_way = static_cast<int>(way);
        }
    }
    return lru_way;
}

template <typename Line>
void updateLRU(std::vector<Line>& set, int accessed_way) {
    for (size_t way = 0; way < set.size(); ++way) {
        if (static_cast<int>(way) == accessed_way) {
            set[way].counter = 0;
        } else if (set[way].valid) {
            set[way].counter++;
        }
    }
}

template <typename Line>
int getMRUWay(std::vector<Line>& set) {
    int mru_way = -1;
    int min_counter = -1;
    for (size_t way = 0; way < set.size(); ++way) {
        if (!set[way].valid) return static_cast<int>(way); // Prefer invalid (empty) lines first
        if (set[way].counter > min_counter) {
            min_counter = set[way].counter;
            mru_way = static_cast<int>(way);
        }
    }
    return mru_way; // Return the most recently used valid way
}

template <typename Line>
void updateMRU(std::vector<Line>& set, int accessed_way) {
    int max_counter = 0;
    for (const auto& line : set) {
        if (line.valid && line.counter > max_counter) {
            max_counter = line.counter;
        }
    }
    set[accessed_way].counter = max_counter + 1;
}

template <typename Line>
int get_replacement_way(std::vector<Line>& set) {
    if (replacement_policy == "LRU") {
        return getLRUWay(set);
    } else if (replacement_policy == "MRU") {
        return getMRUWay(set);
    } else {
        std::cerr << "Invalid replacement policy: " << replacement_policy << std::endl;
        exit(1);
    }
}

template <typename Line>
void updateReplacement(std::vector<Line>& set, int accessed_way) {
    if (replacement_policy == "LRU") {
        updateLRU(set, accessed_way);
    } else if (replacement_policy == "MRU") {
        updateMRU(set, accessed_way);
    } else {
        std::cerr << "Invalid replacement policy: " << replacement_policy << std::endl;
        exit(1);
    }
}
