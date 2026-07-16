const API_BASE = import.meta.env.VITE_API_URL || "";

export async function fetchHealth() {
  const response = await fetch(`${API_BASE}/api/health`);
  if (!response.ok) throw new Error("Backend unavailable");
  return response.json();
}

export async function fetchExamples() {
  const response = await fetch(`${API_BASE}/api/examples`);
  if (!response.ok) throw new Error("Failed to load examples");
  return response.json();
}

export async function runSimulation(assembly, specifications, includeTrace = false) {
  const response = await fetch(`${API_BASE}/api/run`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      assembly,
      specifications,
      include_trace: includeTrace,
    }),
  });

  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    const detail = payload.detail || "Simulation failed";
    throw new Error(typeof detail === "string" ? detail : JSON.stringify(detail));
  }
  return payload;
}
