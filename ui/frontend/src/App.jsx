import { useEffect, useMemo, useState } from "react";
import { fetchExamples, fetchHealth, runSimulation } from "./api.js";

const TABS = ["Overview", "Registers", "Memory", "Cache"];
const STAGE_ORDER = [
  ["if_id", "IF_ID"],
  ["id_ex", "ID_EX"],
  ["ex_mem", "EX_MEM"],
  ["mem_wb", "MEM_WB"],
  ["wb", "WB"],
];

function StatCard({ label, value, accent }) {
  return (
    <div className="stat-card" style={{ borderColor: accent }}>
      <span className="stat-label">{label}</span>
      <strong className="stat-value">{value}</strong>
    </div>
  );
}

function RegisterGrid({ registers }) {
  return (
    <div className="register-grid">
      {registers.map((value, index) => (
        <div key={index} className="register-cell">
          <span>x{index}</span>
          <strong>{value}</strong>
        </div>
      ))}
    </div>
  );
}

function MemoryTable({ rows, emptyLabel }) {
  if (!rows.length) {
    return <p className="empty-state">{emptyLabel}</p>;
  }

  return (
    <div className="table-wrap">
      <table>
        <thead>
          <tr>
            <th>Address</th>
            <th>Value</th>
          </tr>
        </thead>
        <tbody>
          {rows.map((row) => (
            <tr key={row.address}>
              <td>0x{row.address.toString(16).toUpperCase()}</td>
              <td>{row.value}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

function StageCell({ stage }) {
  if (!stage || !stage.valid) {
    return <span className="stage-empty">—</span>;
  }
  return (
    <span>
      {stage.instruction}
      {stage.args?.length > 0 ? " " + stage.args.join(", ") : ""}
    </span>
  );
}

function StepThrough({ trace }) {
  const [stepIndex, setStepIndex] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [selectedCore, setSelectedCore] = useState(0);

  const maxIndex = trace.length - 1;

  useEffect(() => {
    setStepIndex(0);
    setPlaying(false);
  }, [trace]);

  useEffect(() => {
    if (!playing) return undefined;
    if (stepIndex >= maxIndex) {
      setPlaying(false);
      return undefined;
    }
    const timer = setTimeout(() => {
      setStepIndex((current) => Math.min(current + 1, maxIndex));
    }, 500);
    return () => clearTimeout(timer);
  }, [playing, stepIndex, maxIndex]);

  const snapshot = trace[stepIndex];
  const prevSnapshot = stepIndex > 0 ? trace[stepIndex - 1] : null;
  const coreSnapshot = snapshot.cores.find((c) => c.id === selectedCore) || snapshot.cores[0];
  const prevCoreSnapshot = prevSnapshot?.cores.find((c) => c.id === selectedCore);

  function goTo(index) {
    setPlaying(false);
    setStepIndex(Math.max(0, Math.min(index, maxIndex)));
  }

  return (
    <div className="step-view">
      <p className="step-hint">
        Use <strong>Step</strong> to advance one clock cycle at a time, <strong>Play</strong>/
        <strong>Pause</strong> to auto-advance, or <strong>Run All</strong> to jump straight to
        the final result.
      </p>
      <div className="stepper-controls">
        <button onClick={() => goTo(0)} disabled={stepIndex === 0} title="Reset to first cycle">
          ⏮ Reset
        </button>
        <button onClick={() => goTo(stepIndex - 1)} disabled={stepIndex === 0}>
          ◀ Prev
        </button>
        <button onClick={() => goTo(stepIndex + 1)} disabled={stepIndex === maxIndex}>
          Step ▶
        </button>
        <button
          className="primary-btn play-btn"
          onClick={() => setPlaying((current) => !current)}
          disabled={stepIndex === maxIndex && !playing}
        >
          {playing ? "⏸ Pause" : "▶ Play"}
        </button>
        <button
          className="run-all-btn"
          onClick={() => goTo(maxIndex)}
          disabled={stepIndex === maxIndex}
          title="Jump to the final cycle (run every remaining instruction at once)"
        >
          ⏭ Run All
        </button>
        <input
          type="range"
          min={0}
          max={maxIndex}
          value={stepIndex}
          onChange={(event) => goTo(Number(event.target.value))}
        />
        <span className="step-counter">
          Cycle {snapshot.cycle} / {trace[maxIndex].cycle}
        </span>
      </div>

      <div className="table-wrap">
        <table className="pipeline-table">
          <thead>
            <tr>
              <th>Core</th>
              <th>PC</th>
              {STAGE_ORDER.map(([, label]) => (
                <th key={label}>{label}</th>
              ))}
            </tr>
          </thead>
          <tbody>
            {snapshot.cores.map((c) => (
              <tr
                key={c.id}
                className={c.id === selectedCore ? "pipeline-row selected" : "pipeline-row"}
                onClick={() => setSelectedCore(c.id)}
              >
                <td>Core {c.id}</td>
                <td>{c.pc}</td>
                {STAGE_ORDER.map(([key, label]) => (
                  <td key={label}>
                    <StageCell stage={c[key]} />
                  </td>
                ))}
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      <div className="core-switcher">
        {snapshot.cores.map((c) => (
          <button
            key={c.id}
            className={selectedCore === c.id ? "tab active" : "tab"}
            onClick={() => setSelectedCore(c.id)}
          >
            Core {c.id}
          </button>
        ))}
      </div>

      <h3>Registers (Core {selectedCore}) — this cycle</h3>
      <div className="register-grid">
        {coreSnapshot?.registers.map((value, index) => {
          const prevValue = prevCoreSnapshot?.registers[index];
          const changed = prevValue !== undefined && prevValue !== value;
          return (
            <div key={index} className={changed ? "register-cell changed" : "register-cell"}>
              <span>x{index}</span>
              <strong>{value}</strong>
            </div>
          );
        })}
      </div>

      <h3>Memory writes this cycle</h3>
      <MemoryTable rows={snapshot.memory_changes} emptyLabel="No memory writes this cycle." />
    </div>
  );
}

export default function App() {
  const [assembly, setAssembly] = useState("");
  const [specifications, setSpecifications] = useState("");
  const [results, setResults] = useState(null);
  const [activeTab, setActiveTab] = useState("Overview");
  const [selectedCore, setSelectedCore] = useState(0);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [simulatorReady, setSimulatorReady] = useState(false);
  const [includeTrace, setIncludeTrace] = useState(false);

  const tabs = useMemo(
    () => (results?.trace?.length ? [...TABS, "Step-by-step"] : TABS),
    [results]
  );

  useEffect(() => {
    async function bootstrap() {
      try {
        const [health, examples] = await Promise.all([
          fetchHealth(),
          fetchExamples(),
        ]);
        setSimulatorReady(Boolean(health.simulator_ready));
        setAssembly(examples.assembly);
        setSpecifications(examples.specifications);
      } catch (err) {
        setError(err.message);
      }
    }

    bootstrap();
  }, []);

  const core = results?.cores?.[selectedCore];

  const overviewCards = useMemo(() => {
    if (!results?.cores?.length) return [];
    const totalInstructions = results.cores.reduce(
      (sum, item) => sum + item.instructions,
      0
    );
    const totalStalls = results.cores.reduce((sum, item) => sum + item.stalls, 0);
    const avgIpc =
      results.cores.reduce((sum, item) => sum + item.ipc, 0) /
      results.cores.length;
    const maxCycles = Math.max(...results.cores.map((item) => item.clock_cycles));

    return [
      { label: "Clock Cycles", value: maxCycles, accent: "#7c9cff" },
      { label: "Total Instructions", value: totalInstructions, accent: "#5ad1a5" },
      { label: "Total Stalls", value: totalStalls, accent: "#f0b35a" },
      { label: "Average IPC", value: avgIpc.toFixed(3), accent: "#f08ea8" },
    ];
  }, [results]);

  async function handleRun() {
    setLoading(true);
    setError("");
    try {
      const payload = await runSimulation(assembly, specifications, includeTrace);
      setResults(payload);
      setActiveTab(payload.trace?.length ? "Step-by-step" : "Overview");
      setSelectedCore(0);
    } catch (err) {
      setError(err.message);
      setResults(null);
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="app-shell">
      <header className="hero">
        <div>
          <p className="eyebrow">COA Project · Phase 3</p>
          <h1>RISC-V Simulator UI</h1>
          <p className="subtitle">
            Run multi-core pipelined programs with cache hierarchy, stalls, and IPC
            metrics from your browser.
          </p>
        </div>
        <div className="hero-actions">
          <span className={`status-pill ${simulatorReady ? "ready" : "pending"}`}>
            {simulatorReady ? "Simulator ready" : "Build simulator to run"}
          </span>
          <label className="trace-toggle">
            <input
              type="checkbox"
              checked={includeTrace}
              onChange={(event) => setIncludeTrace(event.target.checked)}
            />
            Record step-by-step trace
          </label>
          <button className="primary-btn" onClick={handleRun} disabled={loading}>
            {loading ? "Running..." : "Run Simulation"}
          </button>
        </div>
      </header>

      <main className="layout">
        <section className="panel editors">
          <div className="editor-block">
            <div className="panel-header">
              <h2>Assembly</h2>
            </div>
            <textarea
              value={assembly}
              onChange={(event) => setAssembly(event.target.value)}
              spellCheck={false}
            />
          </div>

          <div className="editor-block">
            <div className="panel-header">
              <h2>Specifications</h2>
            </div>
            <textarea
              value={specifications}
              onChange={(event) => setSpecifications(event.target.value)}
              spellCheck={false}
            />
          </div>
        </section>

        <section className="panel results">
          <div className="panel-header">
            <h2>Results</h2>
            <div className="tabs">
              {tabs.map((tab) => (
                <button
                  key={tab}
                  className={activeTab === tab ? "tab active" : "tab"}
                  onClick={() => setActiveTab(tab)}
                >
                  {tab}
                </button>
              ))}
            </div>
          </div>

          {error && <div className="error-banner">{error}</div>}

          {!results && !error && (
            <p className="empty-state">
              Edit assembly and specifications, then run the simulation.
            </p>
          )}

          {results && activeTab === "Overview" && (
            <div className="overview">
              <div className="stat-grid">
                {overviewCards.map((card) => (
                  <StatCard key={card.label} {...card} />
                ))}
              </div>
              <div className="core-summary-grid">
                {results.cores.map((item) => (
                  <div key={item.id} className="core-card">
                    <h3>Core {item.id}</h3>
                    <p>IPC: {item.ipc.toFixed(3)}</p>
                    <p>Cycles: {item.clock_cycles}</p>
                    <p>Stalls: {item.stalls}</p>
                    <p>Instructions: {item.instructions}</p>
                  </div>
                ))}
              </div>
            </div>
          )}

          {results && activeTab === "Registers" && (
            <div>
              <div className="core-switcher">
                {results.cores.map((item) => (
                  <button
                    key={item.id}
                    className={selectedCore === item.id ? "tab active" : "tab"}
                    onClick={() => setSelectedCore(item.id)}
                  >
                    Core {item.id}
                  </button>
                ))}
              </div>
              {core && <RegisterGrid registers={core.registers} />}
            </div>
          )}

          {results && activeTab === "Memory" && (
            <div className="memory-section">
              <h3>Main Memory</h3>
              <MemoryTable
                rows={results.memory}
                emptyLabel="No non-zero memory values."
              />
              <h3>Scratchpad (Core {selectedCore})</h3>
              <div className="core-switcher">
                {results.cores.map((item) => (
                  <button
                    key={item.id}
                    className={selectedCore === item.id ? "tab active" : "tab"}
                    onClick={() => setSelectedCore(item.id)}
                  >
                    Core {item.id}
                  </button>
                ))}
              </div>
              <MemoryTable
                rows={core?.scratchpad || []}
                emptyLabel="No scratchpad values for this core."
              />
            </div>
          )}

          {results && activeTab === "Cache" && (
            <div className="cache-section">
              <div className="stat-grid">
                <StatCard
                  label="L2 Hits"
                  value={results.cache.l2_hits}
                  accent="#7c9cff"
                />
                <StatCard
                  label="L2 Misses"
                  value={results.cache.l2_misses}
                  accent="#f0b35a"
                />
                <StatCard
                  label="Memory Accesses"
                  value={results.cache.memory_accesses}
                  accent="#5ad1a5"
                />
                <StatCard
                  label="L2 Hit Rate"
                  value={`${results.cache.l2_hit_rate}%`}
                  accent="#f08ea8"
                />
              </div>
              <div className="core-summary-grid">
                {results.cores.map((item, index) => (
                  <div key={item.id} className="core-card">
                    <h3>Core {item.id}</h3>
                    <p>L1 Hits: {results.cache.l1_hits[index]}</p>
                    <p>L1 Misses: {results.cache.l1_misses[index]}</p>
                    <p>L1 Hit Rate: {results.cache.l1_hit_rates[index]}%</p>
                  </div>
                ))}
              </div>
            </div>
          )}

          {results && activeTab === "Step-by-step" && results.trace?.length > 0 && (
            <StepThrough trace={results.trace} />
          )}
        </section>
      </main>
    </div>
  );
}
