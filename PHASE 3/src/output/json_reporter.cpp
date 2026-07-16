#include "json_reporter.hpp"
#include "../memory/memory.hpp"
#include "../common/constants.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

static string jsonEscape(const string &value) {
    string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

static void printStageJson(const char *key, const StageSnapshot &stage, bool last) {
    cout << "          \"" << key << "\": {\"valid\": " << (stage.valid ? "true" : "false");
    if (stage.valid) {
        cout << ", \"instruction\": \"" << jsonEscape(stage.instruction) << "\", \"args\": [";
        for (size_t a = 0; a < stage.args.size(); ++a) {
            if (a > 0) cout << ", ";
            cout << "\"" << jsonEscape(stage.args[a]) << "\"";
        }
        cout << "]";
    }
    cout << "}" << (last ? "\n" : ",\n");
}

static void printTraceJson(const vector<CycleSnapshot> &trace) {
    cout << "  \"trace\": [\n";
    for (size_t i = 0; i < trace.size(); ++i) {
        const CycleSnapshot &snap = trace[i];
        cout << "    {\n";
        cout << "      \"cycle\": " << snap.cycle << ",\n";
        cout << "      \"cores\": [\n";
        for (size_t c = 0; c < snap.cores.size(); ++c) {
            const CoreCycleSnapshot &cc = snap.cores[c];
            cout << "        {\n";
            cout << "          \"id\": " << cc.core_id << ",\n";
            cout << "          \"pc\": " << cc.pc << ",\n";
            printStageJson("if_id", cc.if_id, false);
            printStageJson("id_ex", cc.id_ex, false);
            printStageJson("ex_mem", cc.ex_mem, false);
            printStageJson("mem_wb", cc.mem_wb, false);
            printStageJson("wb", cc.wb, false);
            cout << "          \"registers\": [";
            for (size_t r = 0; r < cc.registers.size(); ++r) {
                if (r > 0) cout << ", ";
                cout << cc.registers[r];
            }
            cout << "]\n";
            cout << "        }" << (c + 1 < snap.cores.size() ? ",\n" : "\n");
        }
        cout << "      ],\n";
        cout << "      \"memory_changes\": [";
        for (size_t m = 0; m < snap.memory_changes.size(); ++m) {
            if (m > 0) cout << ", ";
            cout << "{\"address\": " << snap.memory_changes[m].address
                 << ", \"value\": " << snap.memory_changes[m].value << "}";
        }
        cout << "]\n";
        cout << "    }" << (i + 1 < trace.size() ? ",\n" : "\n");
    }
    cout << "  ]\n";
}

void printJsonResults(const vector<Core> &cores, const Specifications &specs,
                       const vector<CycleSnapshot> *trace) {
    cout << "{\n";
    cout << "  \"success\": true,\n";
    cout << "  \"config\": {\n";
    cout << "    \"data_forwarding\": " << (specs.data_forwarding ? "true" : "false") << ",\n";
    cout << "    \"replacement_policy\": \"" << jsonEscape(specs.replacement_policy) << "\",\n";
    cout << "    \"line_size\": " << specs.line_size << ",\n";
    cout << "    \"L1_cache_size\": " << specs.L1_cache_size << ",\n";
    cout << "    \"L2_cache_size\": " << specs.L2_cache_size << ",\n";
    cout << "    \"L1_associativity\": " << specs.L1_associativity << ",\n";
    cout << "    \"L2_associativity\": " << specs.L2_associativity << ",\n";
    cout << "    \"L1_latency\": " << specs.L1_latency << ",\n";
    cout << "    \"L2_latency\": " << specs.L2_latency << ",\n";
    cout << "    \"main_memory_latency\": " << specs.main_memory_latency << "\n";
    cout << "  },\n";
    cout << "  \"cores\": [\n";
    for (size_t i = 0; i < cores.size(); ++i) {
        const Core &core = cores[i];
        double ipc = core.clock_cycles == 0 ? 0.0
            : static_cast<double>(core.number_instructions) / core.clock_cycles;
        cout << "    {\n";
        cout << "      \"id\": " << core.core_id << ",\n";
        cout << "      \"clock_cycles\": " << core.clock_cycles << ",\n";
        cout << "      \"stalls\": " << core.pipeline.stalls << ",\n";
        cout << "      \"instructions\": " << core.number_instructions << ",\n";
        cout << "      \"ipc\": " << fixed << setprecision(4) << ipc << ",\n";
        cout << "      \"registers\": [";
        for (int r = 0; r < REGISTER_COUNT; ++r) {
            if (r > 0) cout << ", ";
            cout << core.registers[r];
        }
        cout << "],\n";
        cout << "      \"scratchpad\": [";
        bool firstSp = true;
        for (int addr = 0; addr < core.spm.size_sp; addr += 4) {
            int value = core.spm.readWord(addr);
            if (value == 0) continue;
            if (!firstSp) cout << ", ";
            firstSp = false;
            cout << "{\"address\": " << addr << ", \"value\": " << value << "}";
        }
        cout << "]\n";
        cout << "    }";
        if (i + 1 < cores.size()) cout << ",";
        cout << "\n";
    }
    cout << "  ],\n";
    cout << "  \"memory\": [";
    bool firstMem = true;
    for (int addr = 0; addr < MEMORY_SIZE; addr += 4) {
        int value = readMemoryWord(addr);
        if (value == 0) continue;
        if (!firstMem) cout << ", ";
        firstMem = false;
        cout << "{\"address\": " << addr << ", \"value\": " << value << "}";
    }
    cout << "],\n";
    cout << "  \"cache\": {\n";
    cout << "    \"l1_hits\": [";
    for (int i = 0; i < CORE_COUNT; ++i) {
        if (i > 0) cout << ", ";
        cout << cache_l1_hits[i];
    }
    cout << "],\n";
    cout << "    \"l1_misses\": [";
    for (int i = 0; i < CORE_COUNT; ++i) {
        if (i > 0) cout << ", ";
        cout << cache_l1_misses[i];
    }
    cout << "],\n";
    cout << "    \"l2_hits\": " << cache_l2_hits << ",\n";
    cout << "    \"l2_misses\": " << cache_l2_misses << ",\n";
    cout << "    \"memory_accesses\": " << memory_accesses << ",\n";
    cout << "    \"l1_hit_rates\": [";
    for (int i = 0; i < CORE_COUNT; ++i) {
        int total = cache_l1_hits[i] + cache_l1_misses[i];
        float rate = total == 0 ? 0.0f : static_cast<float>(cache_l1_hits[i]) / total * 100.0f;
        if (i > 0) cout << ", ";
        cout << fixed << setprecision(2) << rate;
    }
    cout << "],\n";
  int l2_total = cache_l2_hits + cache_l2_misses;
  float l2_hit_rate = l2_total == 0 ? 0.0f
      : static_cast<float>(cache_l2_hits) / l2_total * 100.0f;
  float l2_miss_rate = l2_total == 0 ? 0.0f
      : static_cast<float>(cache_l2_misses) / l2_total * 100.0f;
    cout << "    \"l2_hit_rate\": " << fixed << setprecision(2) << l2_hit_rate << ",\n";
    cout << "    \"l2_miss_rate\": " << fixed << setprecision(2) << l2_miss_rate << "\n";
    cout << "  }" << (trace ? ",\n" : "\n");
    if (trace) {
        printTraceJson(*trace);
    }
    cout << "}\n";
}
