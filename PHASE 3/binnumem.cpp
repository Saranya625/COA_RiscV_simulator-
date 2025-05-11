string fetch_instruction_with_icache(int pc, int core_id) {
    uint32_t address = pc * sizeof(uint32_t);  // Assuming 1 instruction = 4 bytes
    uint32_t offset = getOffset(address);
    uint32_t l1_idx = getL1Index(address);
    uint32_t l1_tag = getTag_L1(address);
    auto& l1_set = l1_cache[core_id][l1_idx];

    if (l1_set.empty()) l1_set.resize(associativity_l1);

    // Look for hit in L1 I-Cache
    for (int way = 0; way < associativity_l1; ++way) {
        if (l1_set[way].valid && l1_set[way].tag == l1_tag) {
            cache_l1_hits[core_id]++;
            updateLRU(l1_set, way);
            return fetch_instruction(pc);  // Fetch from decoded instruction vector
        }
    }

    // L1 miss
    cache_l1_misses[core_id]++;
    uint32_t l2_idx = getL2Index(address);
    uint32_t l2_tag = getTag_L2(address);
    auto& l2_set = l2_cache[l2_idx];
    if (l2_set.empty()) l2_set.resize(associativity_l2);

    // Look for instruction in L2
    for (int way = 0; way < associativity_l2; ++way) {
        if (l2_set[way].valid && l2_set[way].tag == l2_tag) {
            cache_l2_hits++;
            updateLRU(l2_set, way);

            int l1_replace = getLRUWay(l1_set);
            memcpy(&l1_set[l1_replace].line_data, &l2_set[way].line_data, LINE_SIZE);
            l1_set[l1_replace].tag = l1_tag;
            l1_set[l1_replace].valid = true;
            updateLRU(l1_set, l1_replace);

            return fetch_instruction(pc);
        }
    }

    // Miss in both L1 and L2 → fetch from instruction vector
    cache_l2_misses++;
    memory_accesses++;

    int l2_replace = getLRUWay(l2_set);
    int l1_replace = getLRUWay(l1_set);

    // Simulate loading from main memory into L2
    memset(&l2_set[l2_replace].line_data, 0, LINE_SIZE); // You can fill with dummy data or leave as zero
    l2_set[l2_replace].valid = true;
    l2_set[l2_replace].tag = l2_tag;
    updateLRU(l2_set, l2_replace);

    // Fill L1 with dummy data for instruction
    memset(&l1_set[l1_replace].line_data, 0, LINE_SIZE);
    l1_set[l1_replace].valid = true;
    l1_set[l1_replace].tag = l1_tag;
    updateLRU(l1_set, l1_replace);

    return fetch_instruction(pc);
}
