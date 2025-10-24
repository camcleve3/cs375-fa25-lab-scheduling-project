## CS375 — Scheduling Simulator (student version)

Hey — this is my lab project for CS375. I built a small CPU scheduling simulator in C++17 so I could try out different scheduling algorithms and compare their behavior.

What’s in this repo
- `simulator.cpp` — the whole simulator in one file. It contains a tiny `Scheduler` interface and implementations for several algorithms.

Quick build
I compiled this locally with g++ on the dev container. To build the program yourself run from the repo root:

```bash
g++ -std=c++17 -O2 -Wall simulator.cpp -o simulator
```

Run examples
The simulator has a simple CLI. Examples I used while testing:

```bash
# run Round-Robin with quantum 4
./simulator --scheduler rr --quantum 4

# run First-Come First-Served
./simulator --scheduler fcfs

# run the multi-level queue (MLQ)
./simulator --scheduler mlq

# generate some random processes (quick smoke test)
./simulator --random 1 --scheduler rr --quantum 3
```

Notes on flags
- `--scheduler`: which scheduler to run. Supported: `fcfs`, `sjf`, `srtf`, `prio` (or `priority`), `rr`, `mlq`, `mlfq`, `lottery`, `cfs`, `edf`.
- `--quantum`: time quantum for RR-based schedulers (default 4).
- `--input`: path to a text file with processes. If omitted the simulator uses a small built-in example.
- `--random`: pass `--random 1` to generate a set of random processes instead of using a file.

Input format (if you use `--input`)
Each line should be whitespace-separated columns:

```
ID arrival_time burst_time priority [deadline]
```

Example lines:

```
P1 0 8 2
P2 1 4 1
P3 2 9 3
P4 3 5 4
P5 4 3 1 12   # optional deadline for EDF
```

What the program prints
- A simple textual Gantt-like list of (process, duration) blocks
- Average waiting time and turnaround time
- CPU utilization (%) and throughput

A few dev notes (from me)
- I implemented each scheduler as a class derived from `Scheduler` inside `simulator.cpp` so it’s easy to add/modify algorithms.
- If something crashes, check the scheduler you changed first — I left comments near each scheduler class.
- I can add a sample `processes.txt` and a `Makefile` if you want; tell me and I’ll drop them in.

If you want me to tweak wording, add examples, or include a sample input file, say which one and I’ll update it.

# cs375-fa25-lab-scheduling-project