import json
import subprocess
import sys
import tempfile
from pathlib import Path

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field

ROOT = Path(__file__).resolve().parents[2]
PHASE3 = ROOT / "PHASE 3"
DEFAULT_ASM = (PHASE3 / "Sync.asm").read_text(encoding="utf-8")
DEFAULT_SPECS = (PHASE3 / "Specifications.txt").read_text(encoding="utf-8")

SIMULATOR_NAMES = (
    ["simulator.exe"] if sys.platform.startswith("win") else ["simulator", "a.out"]
)


def is_runnable_binary(path: Path) -> bool:
    if not path.exists() or not path.is_file():
        return False
    if sys.platform.startswith("win") and path.suffix.lower() != ".exe":
        return False
    return True


def find_simulator() -> Path | None:
    for name in SIMULATOR_NAMES:
        candidate = PHASE3 / name
        if is_runnable_binary(candidate):
            return candidate
    return None


class RunRequest(BaseModel):
    assembly: str = Field(..., min_length=1)
    specifications: str = Field(..., min_length=1)


app = FastAPI(title="RISC-V Simulator API", version="1.0.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


def build_hint() -> str:
    if sys.platform.startswith("win"):
        return (
            "Build the simulator first:\n"
            "  cd \"PHASE 3\"\n"
            "  g++ -std=c++17 -O2 simulator.cpp -o simulator.exe"
        )
    return (
        "Build the simulator first:\n"
        "  cd \"PHASE 3\"\n"
        "  g++ -std=c++17 -O2 simulator.cpp -o simulator"
    )


@app.get("/api/health")
def health():
    simulator = find_simulator()
    return {
        "status": "ok",
        "simulator_ready": simulator is not None,
        "simulator_path": str(simulator) if simulator else None,
        "phase": "3",
    }


@app.get("/api/examples")
def examples():
    return {
        "assembly": DEFAULT_ASM,
        "specifications": DEFAULT_SPECS,
    }


@app.post("/api/run")
def run_simulation(request: RunRequest):
    simulator = find_simulator()
    if simulator is None:
        raise HTTPException(status_code=503, detail=build_hint())

    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)
        asm_path = tmp_dir / "program.asm"
        specs_path = tmp_dir / "Specifications.txt"
        asm_path.write_text(request.assembly, encoding="utf-8")
        specs_path.write_text(request.specifications, encoding="utf-8")

        command = [
            str(simulator),
            "--json",
            "--asm",
            str(asm_path),
            "--specs",
            str(specs_path),
        ]

        try:
            completed = subprocess.run(
                command,
                cwd=PHASE3,
                capture_output=True,
                text=True,
                timeout=120,
                check=False,
            )
        except subprocess.TimeoutExpired as exc:
            raise HTTPException(status_code=504, detail="Simulation timed out") from exc
        except OSError as exc:
            raise HTTPException(status_code=500, detail=str(exc)) from exc

        if completed.returncode != 0:
            stderr = completed.stderr.strip() or "Simulation failed"
            raise HTTPException(status_code=400, detail=stderr)

        stdout = completed.stdout.strip()
        if not stdout:
            raise HTTPException(status_code=500, detail="Simulator produced no output")

        try:
            return json.loads(stdout)
        except json.JSONDecodeError as exc:
            raise HTTPException(
                status_code=500,
                detail=f"Invalid JSON from simulator: {stdout[:500]}",
            ) from exc
