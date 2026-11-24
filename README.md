# DFA Minimizer and Simulation Engine (Milestone-II)
**Bennett University | SCSET | CSET302: Automata Theory and Computability**

---

## Overview: What does the simulator do?
The DFA Minimizer is an interactive educational and research tool designed to analyze, minimize, and simulate Deterministic Finite Automata (DFAs). Given any valid DFA specification, the simulator:
1. Validates the structural integrity and determinism of the automaton.
2. Identifies and eliminates all unreachable states via Breadth-First Search (BFS).
3. Handles incomplete transition functions by automatically synthesizing an implicit dead (trap) state.
4. Executes Moore’s partition refinement algorithm step-by-step, providing detailed visibility into block signatures and splitting events.
5. Synthesizes the quotient canonical minimal DFA (with up to 60%+ state reduction on redundant machines).
6. Mathematically proves language equivalence ($L(M) \equiv L(M_{min})$) using a synchronized product automaton BFS.
7. Simulates string evaluations step-by-step with an interactive tape and read-head visualizer.

The system is delivered in two synchronized implementations: an interactive Web GUI (`dfa-minimizer.html`) and an optimized native C++17 command-line engine (`dfa_min (1).cpp`).

---

## Supported features: list implemented operations
* **Interactive Visual Editor:** Point-and-click state placement, transition rubber-banding, self-loop bezier curves, and attribute inspector.
* **Deterministic Transition Validation:** Rejection of non-deterministic transitions, multiple initial states, or illegal $\varepsilon$-moves.
* **Unreachable State Pruning:** Automated isolation and removal of disconnected state subgraphs.
* **Implicit Trap State Synthesis:** Automatic completion of partial transition tables using an absorbing dead state ($\emptyset$).
* **Round-by-Round Moore Partition Refinement:** Step-by-step breakdown of partitions $P_0, P_1, \dots, P_k$ with state signature tables.
* **Formal Language Equivalence Verifier:** Product automaton BFS checking that no distinguishing string exists between original and minimal machines.
* **Step-by-Step String Testing:** Simulation tape with real-time read head pointer (`|`), configuration history, and accept/reject decision.
* **File Persistence:** Textual grammar serialization and deserialization (`save`/`load`).
* **Multi-Platform Support:** Runs natively in any browser with zero setup, or compiles to a native C++17 binary.

---

## Technology stack and dependencies: versions
* **Frontend Web Application:** HTML5, CSS3 (CSS Grid, Flexbox, Custom Properties), Vanilla JavaScript (ES2022+), SVG (Scalable Vector Graphics).
  * **Dependencies:** None. Zero external libraries, zero frameworks, zero build tools.
* **Command-Line Engine:** C++17 (ISO/IEC 14882:2017) using the C++ Standard Template Library (`<vector>`, `<set>`, `<map>`, `<iostream>`, `<random>`).
  * **Dependencies:** None. Zero third-party dependencies.
* **Verification & Test Harness:** Node.js v16+ (optional for headless runner), GCC 9+ / Clang 10+ / MSVC 2019+.

---

## Installation: numbered setup commands
1. **Clone or Download the Repository:**
   ```bash
   cd c:\Users\ADMIN\Downloads\DFA_minimizer
   ```
2. **Web Application Setup:**
   No installation or build step is required. Double-click `dfa-minimizer.html` to open it in your browser, or start a local lightweight HTTP server:
   ```bash
   python -m http.server 8000
   ```
   Navigate to `http://localhost:8000/dfa-minimizer.html`.
3. **C++ Native Engine Compilation:**
   Compile the source file using any standard C++17 compiler:
   * Using GCC / MinGW:
     ```bash
     g++ -std=c++17 -O2 -o dfa_min "dfa_min (1).cpp"
     ```
   * Using Clang:
     ```bash
     clang++ -std=c++17 -O2 -o dfa_min "dfa_min (1).cpp"
     ```

---

## How to run: exact command and entry point

### 1. Web GUI
* Open `dfa-minimizer.html` in any browser.
* Use the toolbar to create states, drag transitions, and configure properties.
* Click **Minimize DFA** to trigger partition refinement and view quotient machine results.
* Click **Run input…** to simulate test strings with read-head tape tracking.

### 2. C++ Interactive CLI
```bash
./dfa_min
```
Enter `help` at the prompt to view commands (`state`, `trans`, `initial`, `final`, `minimize`, `run`, `save`, `load`).

### 3. C++ File Batch Evaluation
```bash
./dfa_min sample.dfa "01" "100" "00"
```

### 4. C++ Automated Self-Test Harness
```bash
./dfa_min --test
```

---

## Example: sample automaton definition, input and output

### Sample Automaton Definition (`sample.dfa`)
```text
# 7-state textbook DFA with unreachable state q6
states q0 q1 q2 q3 q4 q5 q6
initial q0
final q2 q3 q4
q0 q1 0
q1 q0 0
q0 q2 1
q1 q3 1
q2 q4 0
q2 q5 1
q3 q4 0
q3 q5 1
q4 q4 0
q4 q5 1
q5 q5 0,1
q6 q6 0
q6 q5 1
```

### Minimization Execution Output
```text
== Summary ==
  7 states drawn -> 6 reachable -> 3 in the minimal DFA
  Equivalence classes: {q0, q1} {q2, q3, q4} {q5} 
  Checked: the minimal DFA accepts exactly the same strings as your DFA.

== 1. Unreachable states ==
  Reachable from the initial state: q0 -> q1 -> q2 -> q3 -> q4 -> q5
  Unreachable (removed): q6

== 2. Partition refinement ==
  P0 (final / non-final):
  B1{q2, q3, q4}  B2{q0, q1, q5}

  Round 1: block of each state's move, using the blocks of P0
  Block | State | on 0 | on 1
  ------+-------+------+-----
  B1    | q2    | B1   | B2  
        | q3    | B1   | B2  
        | q4    | B1   | B2  
  B2    | q0    | B2   | B1  
        | q1    | B2   | B1  
        | q5    | B2   | B2  
  Blocks split where rows differ. New partition:
  B1{q2, q3, q4}  B2{q0, q1}  B3{q5}

  Round 2: block of each state's move, using the blocks of P1
  No block splits: the partition is stable. Each block is one state of the minimal DFA.

== 3. Minimized DFA ==
  State               | 0  | 1 
  --------------------+----+---
  ->   Q0 {q0, q1}    | Q0 | Q1
    *  Q1 {q2, q3, q4}| Q1 | Q2
       Q2 {q5}        | Q2 | Q2
```

---

## Project structure: modules and responsibilities

```
DFA_minimizer/
├── dfa-minimizer.html          # Web graphical application (Canvas, UI, Core JS)
├── dfa_min (1).cpp             # Native C++ reference engine and automated CLI
├── Technical_Project_Report.md # Formal Milestone-II Technical Project Report
├── Source_Code_Documentation.md# Comprehensive source code API reference
├── README.md                   # University guideline repository overview
└── Guide.pdf                   # University project guidelines & grading rubric
```

### Module Responsibilities:
* **Sanjana Kumari (Testing & Documentation):** Benchmark test suite authoring (T01–T12), edge-case validation, JFLAP cross-verification, and technical report drafting.
* **Tanmay Kaul (Automaton Model & File Handling):** 5-tuple data structure, validation constraints, custom grammar parser, serialization and disk I/O routines.
* **Sanyam Jain (Minimization Engine):** BFS reachability pruning, implicit trap state augmentation, Moore partition refinement loop, quotient machine synthesis, and product automaton BFS language verification.
* **Anubhuti Singh (String Testing Engine):** Deterministic execution runner, read-head tape position tracker, configuration step history, and acceptance logic.
* **Vedant Kapoor (User Interface & Canvas):** Interactive SVG canvas, Bézier transition curve routing, inspector toolbar, round-by-round tables, and UI themes.

---

## Testing: command, test data and expected behavior

### Running Automated Regression Tests
Execute the C++ test runner:
```bash
./dfa_min --test
```

### Summary of Key Test Cases

| Test ID | Category | Scenario / Input | Expected Result | Status |
| :---: | :--- | :--- | :--- | :---: |
| **T01** | Simulation | Binary ending in 0 with input `"10001000"` | Halts in accepting state $q_1$ | **PASS** |
| **T02** | Simulation | Binary ending in 0 with input `"1010010111"` | Halts in non-accepting state $q_0$ | **PASS** |
| **T03** | Validation | Unrecognized symbol in input string (`"10201"`) | Premature rejection at index 2 (`'2'`) | **PASS** |
| **T04** | Edge Case | Empty string $\varepsilon$ on accepting start state | Accepted immediately at $q_0$ | **PASS** |
| **T05** | Incomplete | Input traversing missing transition (`"aa"`) | Routed to synthesized trap state | **PASS** |
| **T06** | Minimization | 7-state textbook DFA with isolated state $q_6$ | $q_6$ eliminated; 3 minimal states output | **PASS** |
| **T07** | Equivalence | Cartesian product BFS on original vs minimal | Returns `verified = true` ($L(M) = L(M_{min})$) | **PASS** |
| **T08** | Stress Test | 400 randomized DFAs compared against string oracle | 100% equivalence match; zero discrepancies | **PASS** |

---

## Known limitations and troubleshooting
1. **Deterministic Automata Only:** Non-deterministic finite automata (NFAs) or $\varepsilon$-NFAs must be converted to DFAs via subset construction before input.
2. **Trap State Visibility:** Incomplete DFAs route undefined moves to an internal dead state ($\emptyset$). In the GUI, this state can be toggled via the dead-state checkbox; in the CLI, use `dead on`.
3. **Character-Based Alphabets:** Alphabet symbols are processed as single ASCII characters. Multi-character tokens are interpreted as sequences of characters.

---

## Team contributions and acknowledgements
* **Sanjana Kumari:** Testing and documentation
* **Tanmay Kaul:** Automaton model and file handling
* **Sanyam Jain:** Minimization engine
* **Anubhuti Singh:** String testing
* **Vedant Kapoor:** User interface

*Special thanks to Bennett University SCSET faculty and the creators of JFLAP for pedagogical inspiration.*
