# RISC-V Simulator UI

Web UI for the Phase 3 multi-core RISC-V simulator (cache hierarchy, stalls, IPC).

## Architecture

```text
React UI  →  FastAPI backend  →  PHASE 3/simulator.exe --json
```

## 1. Build the simulator

You need a C++ compiler installed (MinGW-w64, MSVC, or WSL).

```bash
cd "PHASE 3"
./build.ps1   # Windows PowerShell
./build.sh    # Linux/macOS
```

Both scripts compile `main.cpp` plus every `.cpp` file under `src/` (the
modularized simulator) into `simulator.exe` / `simulator`. If you'd rather
compile by hand, see `PHASE 3/build.ps1` / `build.sh` for the exact file list,
or fall back to the pre-refactor single-file build in `PHASE 3/legacy/`
(`g++ -std=c++17 -O2 legacy/simulator.cpp -o simulator.exe`).

Test JSON output:

```bash
./simulator.exe --json --asm Sync.asm --specs Specifications.txt
```

## 2. Start the backend

```bash
cd ui/backend
pip install -r requirements.txt
uvicorn main:app --reload --port 8000
```

## 3. Start the frontend

```bash
cd ui/frontend
npm install
npm run dev
```

Open [http://localhost:5173](http://localhost:5173)

## API

- `GET /api/health` — backend and simulator status
- `GET /api/examples` — default assembly and specifications
- `POST /api/run` — run simulation

Example request:

```json
{
  "assembly": ".text\nadd x1,x2,x3\n",
  "specifications": "Specifications{\n    data_forwarding:1\n    ...\n}\n"
}
```

## UI features

- Assembly editor
- Specifications editor
- Run simulation
- Overview: IPC, cycles, stalls per core
- Registers view per core
- Memory and scratchpad dump
- Cache hit/miss statistics

## Notes

- The UI currently targets **Phase 3** only.
- If the simulator binary is missing, the UI still loads but shows **Build simulator to run**.
- Phase 1 and Phase 2 can be added later with the same pattern.
