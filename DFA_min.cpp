// DFA minimization simulator (C++17), minimizing by Moore partition refinement.
// Build: g++ -std=c++17 -O2 -o dfa_min dfa_min.cpp
// Run:   ./dfa_min              interactive editor (type "help")
//        ./dfa_min f.dfa [in..] report for a file, then test inputs
//        ./dfa_min --test       self-tests
// File format:  states q0 q1 / initial q0 / final q1 / q0 q1 0,1   (from to symbols)
#include <bits/stdc++.h>
using namespace std;
using Names = vector<string>;
using Part = vector<vector<int>>;

// ---------------------------------------------------------------- automaton
struct DFA {
    Names st; string init; set<string> fin; map<pair<string, char>, string> d;
    bool has(const string& s) const { return find(st.begin(), st.end(), s) != st.end(); }
    string alpha() const { set<char> s; for (auto& [k, v] : d) s.insert(k.second); return {s.begin(), s.end()}; }
};
string join(const Names& v, const string& sep = ", ") {
    string o; for (size_t i = 0; i < v.size(); ++i) o += (i ? sep : "") + v[i]; return o;
}
string norm(const string& t) { set<char> s; for (char c : t) if (!isspace((unsigned char)c) && c != ',') s.insert(c); return {s.begin(), s.end()}; }
Names validate(const DFA& a) {
    if (a.st.empty()) return {"Add at least one state."};
    if (a.init.empty()) return {"Mark one state as the initial state."};
    return {};
}
struct Run { bool ok = false; Names path; };
Run run(const DFA& m, const string& in) {
    Run r{false, {m.init}};
    for (char c : in) { auto it = m.d.find({r.path.back(), c}); if (it == m.d.end()) return r; r.path.push_back(it->second); }
    r.ok = m.fin.count(r.path.back()); return r;
}
bool sameLanguage(const DFA& A, const DFA& B) {  // product BFS; "" is the implicit dead state
    auto go = [](const DFA& m, const string& s, char c) { auto it = m.d.find({s, c}); return s.empty() || it == m.d.end() ? string() : it->second; };
    auto acc = [](const DFA& m, const string& s) { return !s.empty() && m.fin.count(s); };
    vector<pair<string, string>> q{{A.init, B.init}}; set<pair<string, string>> seen(q.begin(), q.end());
    for (size_t i = 0; i < q.size(); ++i) {
        auto [x, y] = q[i];
        if (acc(A, x) != acc(B, y)) return false;
        for (char c : A.alpha()) { pair<string, string> n{go(A, x, c), go(B, y, c)}; if (seen.insert(n).second) q.push_back(n); }
    }
    return true;
}

// ------------------------------------------------------------- minimization
struct Round { Part prev, next, sig; bool stable; };  // sig[state] = block of each move
struct Result {
    Names err, reach, unreach, names; string alpha, dead = "dead"; bool incomplete = false, verified = false;
    vector<vector<int>> delta;
    vector<char> fin, isDead; int init = 0;
    Part classes; vector<Round> rounds; DFA min; vector<Names> members;
    string set(const vector<int>& m) const { Names v; for (int s : m) v.push_back(names[s]); return "{" + join(v) + "}"; }
    int walk(int s, const string& w) const { for (char c : w) s = delta[s][alpha.find(c)]; return s; }
    int visible(bool showDead) const { int c = 0; for (char d : isDead) c += showDead || !d; return c; }
};
Part canon(Part p) { for (auto& b : p) sort(b.begin(), b.end()); sort(p.begin(), p.end()); return p; }

Result minimize(const DFA& a) {
    Result r; r.err = validate(a); if (!r.err.empty()) return r;
    r.alpha = a.alpha(); const int A = r.alpha.size();
    auto mv = [&](const string& s, int k) { auto it = a.d.find({s, r.alpha[k]}); return it == a.d.end() ? string() : it->second; };
    while (a.has(r.dead)) r.dead += "'";

    // 1. reachable states (BFS), in drawing order
    r.reach = {a.init}; set<string> seen{a.init};
    for (size_t i = 0; i < r.reach.size(); ++i)
        for (int k = 0; k < A; ++k) { string t = mv(r.reach[i], k); if (!t.empty() && seen.insert(t).second) r.reach.push_back(t); }
    for (auto& s : a.st) (seen.count(s) ? r.names : r.unreach).push_back(s);

    // 2. complete the reachable part with an implicit dead state
    const int m = r.names.size();
    for (auto& s : r.names) for (int k = 0; k < A; ++k) r.incomplete |= mv(s, k).empty();
    if (r.incomplete) r.names.push_back(r.dead);
    const int n = r.names.size();
    auto id = [&](const string& s) { return int(find(r.names.begin(), r.names.end(), s) - r.names.begin()); };
    r.delta.assign(n, vector<int>(A, n - 1)); r.fin.assign(n, 0); r.init = id(a.init);
    for (int i = 0; i < m; ++i) {
        r.fin[i] = a.fin.count(r.names[i]);
        for (int k = 0; k < A; ++k) { string t = mv(r.names[i], k); if (!t.empty()) r.delta[i][k] = id(t); }
    }

    // 3. partition refinement (Moore): start with final / non-final, split blocks until every row of moves agrees
    Part blocks(2);
    for (int s = 0; s < n; ++s) blocks[r.fin[s] ? 0 : 1].push_back(s);
    blocks.erase(remove_if(blocks.begin(), blocks.end(), [](auto& b) { return b.empty(); }), blocks.end());
    for (;;) {
        vector<int> of(n); for (size_t b = 0; b < blocks.size(); ++b) for (int s : blocks[b]) of[s] = b;
        Round rd{blocks, {}, Part(n), false};
        for (auto& b : blocks) {
            vector<pair<vector<int>, vector<int>>> groups;
            for (int s : b) {
                vector<int> sig; for (int k = 0; k < A; ++k) sig.push_back(of[r.delta[s][k]]);
                rd.sig[s] = sig;
                auto g = find_if(groups.begin(), groups.end(), [&](auto& x) { return x.first == sig; });
                if (g == groups.end()) groups.push_back({sig, {s}}); else g->second.push_back(s);
            }
            for (auto& g : groups) rd.next.push_back(g.second);
        }
        rd.stable = rd.next.size() == blocks.size();
        r.rounds.push_back(rd);
        if (rd.stable) break;
        blocks = rd.next;
    }

    // 4. equivalence classes = the stable blocks, ordered by BFS from the start state
    vector<int> cls(n, -1), order{r.init}, classOrder;
    for (size_t b = 0; b < blocks.size(); ++b) for (int s : blocks[b]) cls[s] = b;
    vector<char> in(n, 0); in[r.init] = 1;
    for (size_t i = 0; i < order.size(); ++i) for (int k = 0; k < A; ++k) { int t = r.delta[order[i]][k]; if (!in[t]) { in[t] = 1; order.push_back(t); } }
    for (int s : order) if (find(classOrder.begin(), classOrder.end(), cls[s]) == classOrder.end()) classOrder.push_back(cls[s]);
    for (int c : classOrder) { r.classes.push_back({}); for (int s = 0; s < n; ++s) if (cls[s] == c) r.classes.back().push_back(s); }

    // 5. the minimal DFA: one state per class
    vector<string> cid(n);
    for (size_t k = 0; k < r.classes.size(); ++k) {
        string name = "Q" + to_string(k); Names mem; auto& c = r.classes[k];
        for (int s : c) { cid[s] = name; if (!(r.names[s] == r.dead && c.size() > 1 && r.incomplete)) mem.push_back(r.names[s]); }
        r.min.st.push_back(name); r.members.push_back(mem);
        r.isDead.push_back(r.incomplete && c.size() == 1 && c[0] == n - 1);
        if (r.fin[c[0]]) r.min.fin.insert(name);
        for (int k2 = 0; k2 < A; ++k2) r.min.d[{name, r.alpha[k2]}] = "";
    }
    for (size_t k = 0; k < r.classes.size(); ++k) for (int c = 0; c < A; ++c) r.min.d[{r.min.st[k], r.alpha[c]}] = cid[r.delta[r.classes[k][0]][c]];
    r.min.init = cid[r.init];
    r.verified = sameLanguage(a, r.min);
    return r;
}

// ---------------------------------------------------------------- rendering
void table(const Names& h, const vector<Names>& rows) {
    vector<size_t> w; for (auto& s : h) w.push_back(s.size());
    for (auto& r : rows) for (size_t c = 0; c < r.size(); ++c) w[c] = max(w[c], r[c].size());
    auto line = [&](const Names& r) { cout << "  "; for (size_t c = 0; c < r.size(); ++c) cout << (c ? " | " : "") << r[c] << string(w[c] - r[c].size(), ' '); cout << "\n"; };
    line(h); cout << "  "; for (size_t c = 0; c < w.size(); ++c) cout << (c ? "-+-" : "") << string(w[c], '-'); cout << "\n";
    for (auto& r : rows) line(r);
}
void box(const Names& L) {
    size_t w = 0; for (auto& l : L) w = max(w, l.size());
    cout << "\n  +" << string(w + 2, '-') << "+\n";
    for (auto& l : L) cout << "  | " << l << string(w - l.size(), ' ') << " |\n";
    cout << "  +" << string(w + 2, '-') << "+\n";
}
string mark(const DFA& m, const string& s) { return string(s == m.init ? "->" : "  ") + (m.fin.count(s) ? "*" : " ") + " " + s; }
void show(const DFA& a) {
    string al = a.alpha(); Names h{"State"}; for (char c : al) h.push_back(string(1, c));
    vector<Names> rows;
    for (auto& s : a.st) { Names row{mark(a, s)}; for (char c : al) { auto it = a.d.find({s, c}); row.push_back(it == a.d.end() ? "-" : it->second); } rows.push_back(row); }
    table(h, rows); cout << "  (-> initial state, * final state)\n";
}
void blocksLine(const Result& r, const Part& p) { cout << "  "; for (size_t b = 0; b < p.size(); ++b) cout << (b ? "  B" : "B") << b + 1 << r.set(p[b]); cout << "\n"; }
void head(const string& t) { cout << "\n== " << t << " ==\n"; }

void report(const DFA& a, const Result& r, bool showDead) {
    if (!r.err.empty()) { box({"Nothing to minimize yet.", join(r.err, " ")}); return; }
    head("Summary");
    cout << "  " << a.st.size() << " states drawn -> " << r.reach.size() << " reachable -> " << r.visible(showDead) << " in the minimal DFA\n  Equivalence classes: ";
    for (auto& c : r.classes) cout << r.set(c) << ' ';
    cout << (r.verified ? "\n  Checked: the minimal DFA accepts exactly the same strings as your DFA.\n" : "\n  The language check FAILED.\n");

    head("1. Unreachable states");
    cout << "  Reachable from the initial state (breadth-first): " << join(r.reach, " -> ")
         << "\n  Unreachable (removed): " << (r.unreach.empty() ? "none" : join(r.unreach)) << "\n";
    if (r.incomplete) cout << "  Some moves are missing. They go to an implicit dead state \"" << r.dead << "\" that loops on every symbol,\n  so the next steps treat the DFA as complete.\n";

    head("2. Partition refinement");
    cout << "  P0 (final / non-final):\n"; blocksLine(r, r.rounds[0].prev);
    for (size_t k = 0; k < r.rounds.size(); ++k) {
        auto& rd = r.rounds[k];
        cout << "\n  Round " << k + 1 << ": block of each state's move, using the blocks of P" << k << "\n";
        Names h{"Block", "State"}; for (char c : r.alpha) h.push_back(string("on ") + c);
        vector<Names> rows;
        for (size_t b = 0; b < rd.prev.size(); ++b) for (size_t i = 0; i < rd.prev[b].size(); ++i) {
            int s = rd.prev[b][i]; Names row{i ? "" : "B" + to_string(b + 1), r.names[s]};
            for (int x : rd.sig[s]) row.push_back("B" + to_string(x + 1));
            rows.push_back(row);
        }
        table(h, rows);
        cout << (rd.stable ? "  No block splits: the partition is stable. Each block is one state of the minimal DFA.\n" : "  Blocks split where rows differ. New partition:\n");
        blocksLine(r, rd.next);
    }

    head("3. Minimized DFA");
    Names h{"State"}; for (char c : r.alpha) h.push_back(string(1, c));
    vector<Names> rows; bool hidden = false;
    auto vis = [&](const string& q) { int k = stoi(q.substr(1)); return showDead || !r.isDead[k]; };
    for (size_t k = 0; k < r.min.st.size(); ++k) {
        if (!vis(r.min.st[k])) { hidden = true; continue; }
        Names row{mark(r.min, r.min.st[k]) + " {" + join(r.members[k]) + "}"};
        for (char c : r.alpha) { const string& t = r.min.d.at({r.min.st[k], c}); row.push_back(vis(t) ? t : "-"); }
        rows.push_back(row);
    }
    table(h, rows);
    if (hidden) cout << "  (Moves shown as - go to the implicit dead state and reject. Use \"dead on\" to show it.)\n";
}

void accepted(const DFA& m, const Run& r, const string& in, const string& name) {
    Names L{"Message", "", "(i) Accepting configuration found!", "", name + " ends in " + r.path.back() + " (final) after reading " + to_string(in.size()) + " symbol(s).", "'|' marks the read head.", ""};
    size_t w = 0; for (auto& s : m.st) w = max(w, s.size());
    for (size_t k = 0; k < r.path.size(); ++k) {
        const string& s = r.path[k];
        L.push_back(string(s == m.init ? ">" : " ") + (m.fin.count(s) ? "*" : " ") + " " + s + string(w - s.size(), ' ') + "   " + in.substr(0, k) + "|" + in.substr(k) + (k ? "     (read " + string(1, in[k - 1]) + ")" : ""));
    }
    L.push_back(""); L.push_back("[ Keep looking ]   [ I'm done ]"); box(L);
}

// ------------------------------------------------------------------ file I/O
string save(const DFA& a) {
    ostringstream o; o << "states"; for (auto& s : a.st) o << ' ' << s;
    o << "\n"; if (!a.init.empty()) o << "initial " << a.init << "\n";
    o << "final"; for (auto& s : a.st) if (a.fin.count(s)) o << ' ' << s;
    o << "\n"; map<pair<string, string>, string> e;
    for (auto& [k, v] : a.d) { auto& s = e[{k.first, v}]; s += (s.empty() ? "" : ","); s += k.second; }
    for (auto& [k, s] : e) o << k.first << ' ' << k.second << ' ' << s << "\n";
    return o.str();
}
string parse(istream& in, DFA& out) {  // returns an error message, or ""
    DFA a; string line, w; int n = 0;
    auto fail = [&](const string& m) { return "Line " + to_string(n) + ": " + m; };
    while (getline(in, line)) {
        ++n; istringstream ls(line.substr(0, line.find('#')));
        if (!(ls >> w)) continue;
        string s;
        if (w == "states") { while (ls >> s) { if (a.has(s)) return fail("duplicate state " + s + "."); a.st.push_back(s); } }
        else if (w == "initial" || w == "final") {
            while (ls >> s) {
                if (!a.has(s)) return fail("unknown state " + s + ". List it in the states line first.");
                if (w == "initial") a.init = s; else a.fin.insert(s);
            }
        } else {
            string to, rest; ls >> to; getline(ls, rest);
            if (!a.has(w) || !a.has(to)) return fail("unknown state in transition.");
            if (norm(rest).empty()) return fail("a transition needs at least one symbol.");
            for (char c : norm(rest)) { auto [it, fresh] = a.d.emplace(make_pair(w, c), to); if (!fresh && it->second != to) return w + " has two different moves on " + c + "."; }
        }
    }
    out = a; return "";
}
const char* EX[3] = {
    "states q0 q1 q2 q3 q4 q5 q6\ninitial q0\nfinal q2 q3 q4\nq0 q1 0\nq1 q0 0\nq0 q2 1\nq1 q3 1\nq2 q4 0\nq2 q5 1\nq3 q4 0\nq3 q5 1\nq4 q4 0\nq4 q5 1\nq5 q5 0,1\nq6 q6 0\nq6 q5 1\n",
    "states q0 q1\ninitial q0\nfinal q1\nq0 q1 0\nq0 q0 1\nq1 q1 0\nq1 q0 1\n",
    "states q0 q1 q2 q3\ninitial q0\nfinal q1 q2 q3\nq0 q1 a\nq1 q2 b\nq2 q1 b\nq3 q3 a\n"};
const char* EXNAME[3] = {"7 states with an unreachable state", "binary strings ending in 0 (2 states)", "missing transitions (implicit dead state)"};
const char* EXIN[3] = {"01", "10001000", "abbb"};
DFA example(int i) { DFA a; istringstream in(EX[i]); parse(in, a); return a; }

// ------------------------------------------------------------------- editor
struct Session {
    DFA cur; vector<DFA> undo, redo; string last; bool dead = false;

    void push() { undo.push_back(cur); redo.clear(); }
    void err(const string& e) { if (!e.empty()) cout << "  ! " << e << "\n"; }
    string addTrans(const string& f, const string& t, const string& text) {
        string raw; for (char c : text) if (!isspace((unsigned char)c)) raw += tolower(c);
        if (!cur.has(f)) return "Unknown state " + f + ".";
        if (!cur.has(t)) return "Unknown state " + t + ".";
        if (raw == "eps" || raw == "epsilon" || raw == "lambda") return "A DFA reads one symbol per move, so epsilon and lambda moves are not allowed.";
        string s = norm(text); if (s.empty()) return "Type at least one symbol, for example 0 or 0,1.";
        for (char c : s) { auto it = cur.d.find({f, c}); if (it != cur.d.end() && it->second != t) return f + " already moves to " + it->second + " on " + c + ". A DFA has one move per symbol."; }
        push(); for (char c : s) cur.d[{f, c}] = t;
        cout << "  Added transition " << f << " -> " << t << " on " << s << ".\n"; return "";
    }
    void test(const Result& r, bool min, const string& in, bool ask) {
        const DFA& m = min ? r.min : cur;
        Run x = run(m, in);
        if (!x.ok) { box({"Message", "", "(i) The input was rejected.", "", "[ OK ]"}); return; }
        accepted(m, x, in, min ? "Minimized DFA (" + to_string(r.visible(dead)) + " states)" : "Original DFA (" + to_string(cur.st.size()) + " states)");
        while (ask) {
            cout << "  [K]eep looking or [D]one: "; string a;
            if (!getline(cin, a) || a.empty() || tolower(a[0]) != 'k') break;
            box({"Message", "", "(i) There are no other configurations.", "A DFA has exactly one path for each input.", "", "[ OK ]"});
        }
    }
    void runCmd(istringstream& in) {
        string w, s; bool min = true, have = false; in >> w;
        if (w == "orig" || w == "min") { min = w == "min"; have = bool(in >> s); } else if (!w.empty()) { s = w; have = true; }
        Result r = minimize(cur);
        if (!r.err.empty()) { box({"Message", "", "Fix the automaton first. " + join(r.err, " ")}); return; }
        if (!have) {
            cout << "\n  Input" << (last.empty() ? "" : " [" + last + "]") << " (type \"\" for the empty string): "; string line;
            if (!getline(cin, line)) return;
            for (char c : line) if (!isspace((unsigned char)c)) s += c;
            if (s.empty()) s = last;
        }
        if (s == "\"\"") s.clear();
        last = s; test(r, min, s, true);
    }
    void check() {
        auto e = validate(cur); if (!e.empty()) { box({"Message", "", join(e, " ")}); return; }
        Names miss; for (auto& s : cur.st) for (char c : cur.alpha()) if (!cur.d.count({s, c})) miss.push_back(s + " on " + string(1, c));
        box(miss.empty() ? Names{"Message", "", "This automaton is a complete DFA."}
                         : Names{"Message", "", "Deterministic, but incomplete: " + join(miss, "; ") + ".", "Missing moves go to an implicit dead state when minimizing."});
    }
    bool go(const string& line) {
        istringstream in(line); string c; if (!(in >> c)) return true;
        for (char& x : c) x = tolower(x);
        if (c == "quit" || c == "exit" || c == "q") return false;
        if (c == "help" || c == "?") cout <<
            "  Editor\n    state [name]              add a state (the first one becomes initial)\n    initial <s>   final <s>   toggle the initial / final mark\n"
            "    trans <from> <to> <syms>  add a transition, e.g.  trans q0 q1 0,1\n    delete state <s>          delete trans <from> <to>\n    show   undo   redo   new\n"
            "  Minimization\n    minimize                  unreachable states, partitions, minimal DFA\n"
            "    dead on|off               show or hide the implicit dead state in the result\n  Testing\n    run [min|orig] [string]   test an input (min = minimized DFA, the default)\n"
            "    check                     deterministic / complete?\n  Files and samples\n    example [1-3]   save [file]   load <file>   quit\n";
        else if (c == "show") show(cur);
        else if (c == "new") { push(); cur = DFA(); cout << "  Cleared. Add your first state with: state\n"; }
        else if (c == "state") {
            string n; in >> n;
            for (int k = 0; n.empty(); ++k) if (!cur.has("q" + to_string(k))) n = "q" + to_string(k);
            if (cur.has(n)) err("The name " + n + " is already used by another state.");
            else { push(); bool first = cur.init.empty(); cur.st.push_back(n); if (first) cur.init = n; cout << "  Added " << n << (first ? " as the initial state." : ".") << "\n"; }
        } else if (c == "initial" || c == "final") {
            string s; in >> s;
            if (!cur.has(s)) { err("Unknown state " + s + "."); return true; }
            push(); bool on;
            if (c == "initial") { on = cur.init != s; cur.init = on ? s : ""; } else { on = !cur.fin.count(s); if (on) cur.fin.insert(s); else cur.fin.erase(s); }
            cout << "  " << s << " is " << (on ? "now " : "no longer ") << (c == "initial" ? "the initial state." : "final.") << "\n";
        } else if (c == "trans") { string f, t, rest; in >> f >> t; getline(in, rest); err(addTrans(f, t, rest)); }
        else if (c == "delete") {
            string w, a, b; in >> w >> a >> b; DFA before = cur;
            if (w == "state" && cur.has(a)) {
                push(); cur.st.erase(find(cur.st.begin(), cur.st.end(), a)); cur.fin.erase(a); if (cur.init == a) cur.init = "";
                for (auto it = cur.d.begin(); it != cur.d.end();) it = (it->first.first == a || it->second == a) ? cur.d.erase(it) : next(it);
            } else if (w == "trans" && !b.empty()) {
                for (auto it = cur.d.begin(); it != cur.d.end();) it = (it->first.first == a && it->second == b) ? cur.d.erase(it) : next(it);
                if (cur.d.size() != before.d.size()) { undo.push_back(before); redo.clear(); } else err("No such transition.");
            } else err("Usage: delete state <name>   or   delete trans <from> <to>");
        } else if (c == "undo" || c == "redo") {
            auto &from = c == "undo" ? undo : redo, &to = c == "undo" ? redo : undo;
            if (from.empty()) err("Nothing to " + c + "."); else { to.push_back(cur); cur = from.back(); from.pop_back(); }
        } else if (c == "example") {
            int n = 0; in >> n;
            if (n < 1 || n > 3) for (int k = 0; k < 3; ++k) cout << "  example " << k + 1 << ": " << EXNAME[k] << "\n";
            else { push(); cur = example(n - 1); last = EXIN[n - 1]; cout << "  Loaded: " << EXNAME[n - 1] << "\n"; }
        } else if (c == "minimize" || c == "report") report(cur, minimize(cur), dead);
        else if (c == "dead") { string v; in >> v; dead = v == "on"; cout << "  The implicit dead state is " << (dead ? "shown" : "hidden") << " in the minimized DFA.\n"; }
        else if (c == "run") runCmd(in);
        else if (c == "check") check();
        else if (c == "save") {
            string p; in >> p;
            if (p.empty()) cout << save(cur); else { ofstream(p) << save(cur); cout << "  Saved to " << p << "\n"; }
        } else if (c == "load") {
            string p; in >> p; ifstream f(p); DFA a;
            if (!f) err("Cannot open " + p); else { string e = parse(f, a); err(e); if (e.empty()) { push(); cur = a; cout << "  Loaded " << p << "\n"; } }
        } else cout << "  Unknown command \"" << c << "\". Type help.\n";
        return true;
    }
};

// -------------------------------------------------------- tests and main
int selfTest() {
    int bad = 0; auto chk = [&](bool ok, const string& w) { if (!ok) { ++bad; cout << "FAIL: " << w << "\n"; } };
    auto cls = [](const Result& r) { string s; for (auto& c : r.classes) { Names v; for (int i : c) v.push_back(r.names[i]); s += join(v, ",") + "|"; } return s; };
    Result r = minimize(example(0));
    chk(r.unreach == Names{"q6"} && cls(r) == "q0,q1|q2,q3,q4|q5|" && r.verified && r.rounds.size() == 2 && r.rounds.back().stable, "textbook");
    DFA e0 = example(1); r = minimize(e0);
    chk(r.min.st.size() == 2 && run(r.min, "10001000").ok && !run(r.min, "1010010111").ok && run(e0, "10001000").ok, "ends in 0");
    r = minimize(example(2));
    chk(r.incomplete && r.verified && r.unreach == Names{"q3"} && cls(r) == "q0|q1,q2|dead|" && r.isDead[2], "missing transitions");
    DFA one; one.st = {"q0"}; one.init = "q0"; one.fin = {"q0"}; r = minimize(one);
    chk(!minimize(DFA()).err.empty() && r.min.st.size() == 1 && run(r.min, "").ok, "edge cases");
    DFA b; istringstream sv(save(example(0))); parse(sv, b); chk(save(b) == save(example(0)), "save/parse round trip");
    mt19937 rng(12345); auto rnd = [&](int k) { return int(rng() % k); };
    for (int t = 0; t < 400; ++t) {
        DFA a; int n = 2 + rnd(6); a.init = "q0";
        for (int i = 0; i < n; ++i) { a.st.push_back("q" + to_string(i)); if (rnd(10) < 4) a.fin.insert(a.st.back()); }
        for (auto& s : a.st) for (char c : string("01")) if (rnd(10) < 9) a.d[{s, c}] = "q" + to_string(rnd(n));
        r = minimize(a); chk(r.verified, "random: language check");
        // independent check: two states share a class exactly when no string of length <= n tells them apart
        vector<string> ws{""}; for (size_t w = 0; w < ws.size() && ws[w].size() < r.names.size(); ++w) for (char c : r.alpha) ws.push_back(ws[w] + c);
        vector<int> of(r.names.size()); for (size_t k = 0; k < r.classes.size(); ++k) for (int s : r.classes[k]) of[s] = k;
        for (int i = 1; i < int(r.names.size()); ++i) for (int j = 0; j < i; ++j) {
            bool apart = false; for (auto& w : ws) apart |= r.fin[r.walk(i, w)] != r.fin[r.walk(j, w)];
            chk(apart == (of[i] != of[j]), "random: classes are exactly the indistinguishable states");
        }
        for (int k = 0; k < 40; ++k) { string s; for (int l = rnd(9); l > 0; --l) s += "01"[rnd(2)]; if (run(a, s).ok != run(r.min, s).ok) { chk(false, "random: language differs on " + s); break; } }
    }
    cout << (bad ? "FAILED" : "ALL OK") << "\n"; return bad != 0;
}

int main(int argc, char** argv) {
    Names args(argv + 1, argv + argc); Session s;
    if (!args.empty() && args[0] == "--test") return selfTest();
    if (args.empty()) {
        cout << "DFA Minimizer - type \"help\" for commands, \"example 2\" to load a sample.\n";
        for (string line; cout << "\ndfa> " << flush, getline(cin, line) && s.go(line);) {}
        return 0;
    }
    bool brief = args[0] == "--brief"; if (brief) args.erase(args.begin());
    ifstream f(args.empty() ? "" : args[0]); string e = f ? parse(f, s.cur) : "Cannot open file";
    if (!e.empty()) { cerr << e << "\n"; return 1; }
    Result r = minimize(s.cur);
    if (!brief) {
        report(s.cur, r, false);
        for (size_t i = 1; i < args.size(); ++i) { cout << "\nInput \"" << args[i] << "\" on the minimized DFA:"; s.test(r, true, args[i] == "\"\"" ? "" : args[i], false); }
        return 0;
    }
    cout << "unreachable:" << join(r.unreach, ",") << "\nclasses:";  // machine-readable, used to cross-check
    for (size_t i = 0; i < r.classes.size(); ++i) { Names v; for (int x : r.classes[i]) v.push_back(r.names[x]); cout << (i ? "|" : "") << join(v, ","); }
    cout << "\nrounds:" << r.rounds.size() << " verified:" << r.verified << "\n";
    for (size_t i = 1; i < args.size(); ++i) { string x = args[i] == "\"\"" ? "" : args[i]; cout << "run:" << x << ":" << run(s.cur, x).ok << ":" << run(r.min, x).ok << "\n"; }
}
