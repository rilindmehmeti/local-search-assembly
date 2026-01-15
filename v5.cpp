#include <iostream>
#include <vector>
#include <numeric>
#include <queue>
#include <tuple>
#include <random>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>

using namespace std;
typedef vector<int> vi;

// ==================== Solver Parameters ====================
struct SolverParams {
	double task_efficiency_weight = 1.0;      // 0.1 to 3.0
	double distance_penalty = 1.0;            // 0.5 to 2.0
	double ownership_distance_factor = 2.0;   // 1.0 to 5.0
	double path_cost_threshold = 1.0;         // 0.5 to 2.0
	double bfs_randomness = 0.5;              // 0.0 to 1.0
	
	void print() const {
		cerr << "Params(eff=" << task_efficiency_weight 
		     << ", dist=" << distance_penalty
		     << ", own=" << ownership_distance_factor
		     << ", thresh=" << path_cost_threshold
		     << ", rand=" << bfs_randomness << ")" << endl;
	}
	
	SolverParams mutate(mt19937& mt, double temperature = 0.2, 
	                   bool fix_task_eff = false,
	                   bool fix_dist_penalty = false,
	                   bool fix_ownership = false,
	                   bool fix_path_threshold = false,
	                   bool fix_bfs_random = false) const {
		SolverParams p;
		normal_distribution<double> dist(0.0, 1.0);
		
		if(fix_task_eff) {
			p.task_efficiency_weight = task_efficiency_weight;
		} else {
			p.task_efficiency_weight = max(0.1, min(3.0, 
				task_efficiency_weight + dist(mt) * temperature * 0.5));
		}
		
		if(fix_dist_penalty) {
			p.distance_penalty = distance_penalty;
		} else {
			p.distance_penalty = max(0.5, min(2.0,
				distance_penalty + dist(mt) * temperature * 0.3));
		}
		
		if(fix_ownership) {
			p.ownership_distance_factor = ownership_distance_factor;
		} else {
			p.ownership_distance_factor = max(1.0, min(5.0,
				ownership_distance_factor + dist(mt) * temperature * 0.8));
		}
		
		if(fix_path_threshold) {
			p.path_cost_threshold = path_cost_threshold;
		} else {
			p.path_cost_threshold = max(0.5, min(2.0,
				path_cost_threshold + dist(mt) * temperature * 0.3));
		}
		
		if(fix_bfs_random) {
			p.bfs_randomness = bfs_randomness;
		} else {
			p.bfs_randomness = max(0.0, min(1.0,
				bfs_randomness + dist(mt) * temperature * 0.2));
		}
		
		return p;
	}
};

// ==================== Parameter Fix Flags ====================
struct ParamFixFlags {
	bool fix_task_eff = false;
	bool fix_dist_penalty = false;
	bool fix_ownership = false;
	bool fix_path_threshold = false;
	bool fix_bfs_random = false;
	
	void print() const {
		vector<string> fixed;
		if(fix_task_eff) fixed.push_back("task-eff");
		if(fix_dist_penalty) fixed.push_back("dist-penalty");
		if(fix_ownership) fixed.push_back("ownership-factor");
		if(fix_path_threshold) fixed.push_back("path-threshold");
		if(fix_bfs_random) fixed.push_back("bfs-random");
		
		if(fixed.empty()) {
			cout << "All parameters will be randomized" << endl;
		} else {
			cout << "Fixed parameters: ";
			for(size_t i = 0; i < fixed.size(); ++i) {
				if(i > 0) cout << ", ";
				cout << fixed[i];
			}
			cout << endl;
		}
	}
};

// ==================== Core Structures ====================
struct Point {
	int x, y;
	Point() = default;
	Point(int x, int y): x(x), y(y) {}
	inline friend bool operator==(const Point &a, const Point &b) {
		return a.x == b.x && a.y == b.y;
	}
	inline friend bool operator!=(const Point &a, const Point &b) {
		return a.x != b.x || a.y != b.y;
	}
	inline friend int distance(const Point &a, const Point &b) {
		return abs(a.x - b.x) + abs(a.y - b.y);
	}
	Point& operator+=(char c) {
		switch(c) {
		case 'R': ++x; break;
		case 'L': --x; break;
		case 'U': ++y; break;
		case 'D': --y; break;
		case 'W': break;
		}
		return *this;
	}
	inline Point operator+(char c) { return Point(*this) += c; }
};

struct Arm {
	vector<string> how;
	string path, cp;
	vector<Point> cur;
	vi z;
	int i;
	bool done;
	Arm() = default;
	Arm(int W, int H, int x, int y, int i): how(W, string(H, 'x')), cur(1, Point(x, y)), i(i), done(false) {}
};

char opp(char c) {
	if(c == 'R') return 'L';
	if(c == 'L') return 'R';
	if(c == 'U') return 'D';
	if(c == 'D') return 'U';
	return c;
}

// ==================== Solver ====================
int greedy_solver(int W, int H, int R, int M, int T, int L,
                  vector<Point>& ms, vi& S, vector<vector<Point>>& P, vi& Len,
                  vector<Arm>& arms, const SolverParams& params,
                  mt19937& mt, bool verbose) {
	
	int score = 0;
	vector<vi> owner(W, vi(H, -1));
	vector<vi> until(W, vi(H, -1));
	vi ts(T); iota(ts.begin(), ts.end(), 0);
	
	// Filter tasks with distance penalty (optimized - cache results)
	vi min_dist_to_other_task(T, 1e8);
	vi min_dist_to_mount(T, 1e8);
	for(int i = 0; i < T; ++i) {
		for(int j = 0; j < T; ++j) {
			if(i != j) {
				int d = distance(P[i][0], P[j].back());
				min_dist_to_other_task[i] = min(min_dist_to_other_task[i], d);
			}
		}
		for(const Point &m : ms) {
			int d = distance(P[i][0], m);
			min_dist_to_mount[i] = min(min_dist_to_mount[i], d);
		}
		Len[i] += (int)(min(min_dist_to_other_task[i], min_dist_to_mount[i]) * params.distance_penalty);
	}
	
	// Sort by efficiency with weight parameter (cache pow calculations)
	vector<double> efficiency_scores(T);
	for(int i = 0; i < T; ++i) {
		efficiency_scores[i] = pow(S[i], params.task_efficiency_weight) / (Len[i] + 1);
	}
	sort(ts.begin(), ts.end(), [&](int i, int j) { 
		return efficiency_scores[i] > efficiency_scores[j];
	});
	
	int j = 0;
	int gl = 0, GL = (int)(R * L * params.path_cost_threshold);
	while(j < T && gl + Len[ts[j]] < GL) {
		gl += Len[ts[j]];
		++j;
	}
	if(verbose) cerr << "KEEP: " << j << " / " << ts.size() << endl;
	ts.resize(j);
	
	// Sort mounting points (optimize min calculation)
	if(W == 300) {
		vi ms_dist(M);
		for(int i = 0; i < M; ++i) {
			ms_dist[i] = min({ms[i].x, W-1-ms[i].x, ms[i].y, H-1-ms[i].y});
		}
		vi ms_indices(M);
		iota(ms_indices.begin(), ms_indices.end(), 0);
		sort(ms_indices.begin(), ms_indices.end(), [&](int i, int j) {
			return ms_dist[i] < ms_dist[j];
		});
		vector<Point> ms_sorted(M);
		for(int i = 0; i < M; ++i) {
			ms_sorted[i] = ms[ms_indices[i]];
		}
		ms = move(ms_sorted);
	} else {
		shuffle(ms.begin(), ms.end(), mt);
	}
	
	arms.clear();
	arms.reserve(R);  // Pre-allocate to avoid reallocations
	for(int i = 0; i < M; ++i) {
		owner[ms[i].x][ms[i].y] = i;
		until[ms[i].x][ms[i].y] = L;
		if(i < R) arms.push_back(Arm(W, H, ms[i].x, ms[i].y, i));
	}
	
	vector<vi> seen(W, vi(H, 0));
	vector<vi> dist(W, vi(H, 0));
	vector<string> pred(W, string(H, 'x'));
	int SS = 0;
	
	// Pre-create random distribution outside hot loop
	uniform_real_distribution<double> rand_dist(0.0, 1.0);
	
	// Pre-allocate direction string
	const char dirs[4] = {'R', 'L', 'U', 'D'};
	
	while(true) {
		int i = -1;
		for(int i0 = 0; i0 < R; ++i0)
			if(!arms[i0].done && (i == -1 || arms[i0].path.size() < arms[i].path.size()))
				i = i0;
		if(i == -1) break;
		
		const int l0 = arms[i].path.size();
		if(verbose) cerr << "I " << i << ' ' << l0 << endl;
		
		Arm bestA;
		int bestT = -1;
		double bestS = -1;
		
		// Cache arm[i] positions to avoid repeated access
		const Point& arm_start = arms[i].cur[0];
		const Point& arm_current = arms[i].cur.back();
		const size_t arm_path_size = arms[i].path.size();
		
		for(int t : ts) {
			bool bad = false;
			// Use reference to avoid expensive copy - we'll copy only if needed
			const Arm& arm_ref = arms[i];
			Arm a = arm_ref;  // Still need copy for path simulation
			// Pre-reserve space to reduce reallocations
			a.path.reserve(L);
			a.cp.reserve(L);
			a.cur.reserve(100);
			
			for(const Point &pt : P[t]) {
				typedef tuple<Point, int, int> QEl;
				const auto comp = [](const QEl &a, const QEl &b) {
					return get<1>(a) > get<1>(b) || (get<1>(a) == get<1>(b) && get<2>(a) < get<2>(b));
				};
				priority_queue<QEl, vector<QEl>, decltype(comp)> Q(comp);
				++ SS;
				bool found = a.cur.back() == pt;
				seen[a.cur.back().x][a.cur.back().y] = SS;
				dist[a.cur.back().x][a.cur.back().y] = a.path.size();
				pred[a.cur.back().x][a.cur.back().y] = 'x';
				Q.emplace(a.cur.back(), a.path.size(), 0);
				
				// Pre-allocate direction array
				char vs[4] = {'R', 'L', 'U', 'D'};
				
				while(!Q.empty() && !found) {
					auto [q, l, depth] = Q.top(); Q.pop();
					if(l > dist[q.x][q.y]) continue;
					if(l >= L) break;
					
					// Apply randomness parameter per iteration
					if(rand_dist(mt) < params.bfs_randomness) {
						shuffle(vs, vs+4, mt);
					}
					
					if(a.how[q.x][q.y]!='x' || q==a.cur.back()) {
						for(int idx = 0; idx < 4; ++idx) {
							char c = vs[idx];
							Point p = q+c;
							if(p.x < 0 || p.x >= W || p.y < 0 || p.y >= H || a.how[p.x][p.y] != c) continue;
							seen[p.x][p.y] = SS;
							dist[p.x][p.y] = l+1;
							pred[p.x][p.y] = 'x';
							if(p == pt) { found = true; break; }
							Q.emplace(p, l+1, depth+1);
						}
					}
					
					for(int idx = 0; idx < 4; ++idx) {
						char c = vs[idx];
						Point p = q+c;
						if(p.x < 0 || p.x >= W || p.y < 0 || p.y >= H || a.how[p.x][p.y] != 'x') continue;
						int l2 = l;
						int j = owner[p.x][p.y];
						if(j != i && until[p.x][p.y] > l) {
							if(until[p.x][p.y] >= L) continue;
							// Use ownership distance factor parameter (use cached positions)
							if(distance(p, arm_start) > params.ownership_distance_factor * distance(p, arms[j].cur[0])) continue;
							// Use cached task start and arm current positions
							if(distance(P[t][0], arm_current) + arms[j].path.size() > distance(P[t][0], arms[j].cur.back()) + arms[j].path.size()) continue;
							l2 = until[p.x][p.y];
						}
						++ l2;
						if(seen[p.x][p.y] == SS && l2 >= dist[p.x][p.y]) continue;
						seen[p.x][p.y] = SS;
						dist[p.x][p.y] = l2;
						pred[p.x][p.y] = c;
						if(p == pt) { found = true; break; }
						Q.emplace(p, l2, depth);
					}
				}
				
				if(!found) { bad = true; break; }
				
				// Optimize path reconstruction
				string add;
				add.reserve(100);  // Pre-allocate to avoid reallocations
				Point p = pt;
				while(pred[p.x][p.y] != 'x') {
					add.push_back(pred[p.x][p.y]);
					p += opp(pred[p.x][p.y]);
				}
				while(a.cur.back() != p) {
					a.path += opp(a.cp.back());
					a.cur.pop_back();
					a.cp.pop_back();
					a.how[a.cur.back().x][a.cur.back().y] = 'x';
				}
				reverse(add.begin(), add.end());
				a.cp += add;
				for(char c : add) {
					a.how[a.cur.back().x][a.cur.back().y] = opp(c);
					const Point p = a.cur.back()+c;
					int wait_steps = dist[p.x][p.y] - dist[a.cur.back().x][a.cur.back().y] - 1;
					if(wait_steps > 0) {
						a.path.append(wait_steps, 'W');
					}
					a.path += c;
					a.cur.push_back(p);
				}
				if(a.path.size() > L) { bad = true; break; }
			}
			
			if(bad) continue;
			
			int path_diff = a.path.size() - arms[i].path.size();
			if(path_diff <= 0) continue;
			
			double sc = (double)S[t] / path_diff;
			if(sc > bestS) {
				bestA = move(a);
				bestS = sc;
				bestT = t;
			}
		}
		
		if(bestT == -1) {
			if(arms[i].cur.size() <= 1) {
				arms[i].done = true;
				continue;
			}
			while(arms[i].cur.size() > 1 && arms[i].path.size() < L) {
				Point p = arms[i].cur.back();
				until[p.x][p.y] = arms[i].path.size();
				arms[i].path.push_back(opp(arms[i].cp.back()));
				arms[i].cur.pop_back();
				arms[i].cp.pop_back();
				arms[i].how[arms[i].cur.back().x][arms[i].cur.back().y] = 'x';
			}
			for(int j = 0; j < R; ++j) if(arms[j].path.size() < L) arms[j].done = false;
			arms[i].done = true;
			continue;
		}
		
		Point p = arms[i].cur.back();
		for(int l = arms[i].path.size(); l < bestA.path.size(); ++l) if(bestA.path[l] != 'W') {
			if(bestA.how[p.x][p.y] == 'x' && p != bestA.cur.back()) until[p.x][p.y] = l;
			p += bestA.path[l];
			owner[p.x][p.y] = i;
			until[p.x][p.y] = L;
		}
		arms[i] = move(bestA);
		score += S[bestT];
		
		int ind = 0;
		while(ts[ind] != bestT) ++ind;
		swap(ts[ind], ts.back());
		ts.pop_back();
		arms[i].z.push_back(bestT);
		if(verbose) cerr << score << '\n';
		
		for(int j = 0; j < R; ++j) if(arms[j].path.size() < L) arms[j].done = false;
	}
	
	if(verbose) {
		cerr << "restant: " << ts.size() << endl;
		cerr << score << '\n';
	}
	
	return score;
}

// ==================== Local Search ====================
tuple<vector<Arm>, int, SolverParams> local_search(
	int W, int H, int R, int M, int T, int L,
	vector<Point> ms, vi S, vector<vector<Point>> P, vi Len,
	const SolverParams& initial_params,
	int iterations, int base_seed, bool verbose,
	const ParamFixFlags& fix_flags) {
	
	mt19937 mt(base_seed);
	
	SolverParams best_params = initial_params;
	vector<Arm> best_arms;
	int best_score = greedy_solver(W, H, R, M, T, L, ms, S, P, Len, best_arms, best_params, mt, verbose);
	
	cout << "\n=== Local Search ===" << endl;
	cout << "Initial: " << best_score << " points with ";
	best_params.print();
	fix_flags.print();
	
	SolverParams current_params = best_params;
	int current_score = best_score;
	
	auto start_time = chrono::steady_clock::now();
	
	for(int it = 0; it < iterations; ++it) {
		double temperature = 1.0 - ((double)it / iterations);
		
		SolverParams candidate_params = current_params.mutate(mt, temperature * 0.5,
		                                                      fix_flags.fix_task_eff,
		                                                      fix_flags.fix_dist_penalty,
		                                                      fix_flags.fix_ownership,
		                                                      fix_flags.fix_path_threshold,
		                                                      fix_flags.fix_bfs_random);
		
		vector<Arm> candidate_arms;
		mt19937 mt_iter(base_seed + it);
		int score = greedy_solver(W, H, R, M, T, L, ms, S, P, Len, 
		                         candidate_arms, candidate_params, mt_iter, false);
		
		if(score > best_score) {
			best_score = score;
			best_params = candidate_params;
			best_arms = candidate_arms;
			current_params = candidate_params;
			current_score = score;
			cout << "[" << (it+1) << "/" << iterations << "] NEW BEST: " << score << " points ";
			candidate_params.print();
		} else if(score > current_score * 0.95) {
			uniform_real_distribution<double> dist(0.0, 1.0);
			double delta = score - current_score;
			double prob = exp(delta / max(1.0, current_score * temperature * 0.1));
			if(dist(mt) < prob * 0.3) {
				current_params = candidate_params;
				current_score = score;
				if(verbose) cout << "[" << (it+1) << "/" << iterations << "] Accepted worse: " << score << endl;
			}
		}
		
		if((it + 1) % 10 == 0) {
			auto now = chrono::steady_clock::now();
			double elapsed = chrono::duration<double>(now - start_time).count();
			cout << "[" << (it+1) << "/" << iterations << "] Best: " << best_score 
			     << ", Current: " << current_score << ", Time: " << elapsed << "s" << endl;
		}
	}
	
	auto end_time = chrono::steady_clock::now();
	double total_time = chrono::duration<double>(end_time - start_time).count();
	
	cout << "\n=== Final Best ===" << endl;
	cout << "Score: " << best_score << endl;
	cout << "Params: ";
	best_params.print();
	cout << "Time: " << total_time << "s" << endl;
	
	return {best_arms, best_score, best_params};
}

// ==================== I/O ====================
void write_output(const vector<Arm>& arms, const string& filename) {
	ofstream out(filename);
	int A = 0;
	for(const Arm &a : arms) if(!a.z.empty()) ++A;
	out << A << '\n';
	for(const Arm &a : arms) if(!a.z.empty()) {
		out << a.cur[0].x << ' ' << a.cur[0].y << ' ' << a.z.size() << ' ' << a.path.size() << '\n';
		for(int t : a.z) out << t << ' ';
		out << '\n';
		for(char c : a.path) out << c << ' ';
		out << '\n';
	}
}

string get_fixed_param_name(const ParamFixFlags& fix_flags) {
	if(fix_flags.fix_task_eff) return "fix-task-eff";
	if(fix_flags.fix_dist_penalty) return "fix-dist-penalty";
	if(fix_flags.fix_ownership) return "fix-ownership-factor";
	if(fix_flags.fix_path_threshold) return "fix-path-threshold";
	if(fix_flags.fix_bfs_random) return "fix-bfs-random";
	return "fix-none";
}

void write_params_json(const string& filename,
                      const SolverParams& initial_params,
                      const SolverParams& final_params,
                      const ParamFixFlags& fix_flags,
                      const string& map_name,
                      int score,
                      int iterations,
                      int seed,
                      bool local_search_mode,
                      double execution_time_minutes) {
	ofstream out(filename);
	out << "{\n";
	out << "  \"map\": \"" << map_name << "\",\n";
	out << "  \"score\": " << score << ",\n";
	out << "  \"execution_time_minutes\": " << fixed << setprecision(4) << execution_time_minutes << ",\n";
	out << "  \"local_search\": " << (local_search_mode ? "true" : "false") << ",\n";
	if(local_search_mode) {
		out << "  \"iterations\": " << iterations << ",\n";
	}
	out << "  \"seed\": " << seed << ",\n";
	if(local_search_mode) {
		out << "  \"fixed_param\": \"" << get_fixed_param_name(fix_flags) << "\",\n";
	}
	out << "  \"initial_params\": {\n";
	out << "    \"task_efficiency_weight\": " << initial_params.task_efficiency_weight << ",\n";
	out << "    \"distance_penalty\": " << initial_params.distance_penalty << ",\n";
	out << "    \"ownership_distance_factor\": " << initial_params.ownership_distance_factor << ",\n";
	out << "    \"path_cost_threshold\": " << initial_params.path_cost_threshold << ",\n";
	out << "    \"bfs_randomness\": " << initial_params.bfs_randomness << "\n";
	out << "  }";
	if(local_search_mode) {
		out << ",\n";
		out << "  \"final_params\": {\n";
		out << "    \"task_efficiency_weight\": " << final_params.task_efficiency_weight << ",\n";
		out << "    \"distance_penalty\": " << final_params.distance_penalty << ",\n";
		out << "    \"ownership_distance_factor\": " << final_params.ownership_distance_factor << ",\n";
		out << "    \"path_cost_threshold\": " << final_params.path_cost_threshold << ",\n";
		out << "    \"bfs_randomness\": " << final_params.bfs_randomness << "\n";
		out << "  },\n";
		out << "  \"fix_flags\": {\n";
		out << "    \"fix_task_eff\": " << (fix_flags.fix_task_eff ? "true" : "false") << ",\n";
		out << "    \"fix_dist_penalty\": " << (fix_flags.fix_dist_penalty ? "true" : "false") << ",\n";
		out << "    \"fix_ownership\": " << (fix_flags.fix_ownership ? "true" : "false") << ",\n";
		out << "    \"fix_path_threshold\": " << (fix_flags.fix_path_threshold ? "true" : "false") << ",\n";
		out << "    \"fix_bfs_random\": " << (fix_flags.fix_bfs_random ? "true" : "false") << "\n";
		out << "  }";
	}
	out << "\n}\n";
}

// ==================== Main ====================
int main(int argc, char* argv[]) {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);
	
	// Parse arguments
	string map_name = "";
	bool verbose = false;
	bool local_search_mode = false;
	int iterations = 50;
	int seed = -1;  // -1 means use random seed
	SolverParams params;
	ParamFixFlags fix_flags;
	
	for(int i = 1; i < argc; ++i) {
		string arg = argv[i];
		if(arg == "-m" && i+1 < argc) {
			map_name = argv[++i];
		} else if(arg == "-v" || arg == "--verbose") {
			verbose = true;
		} else if(arg == "--local-search") {
			local_search_mode = true;
		} else if(arg == "--iterations" && i+1 < argc) {
			iterations = stoi(argv[++i]);
		} else if(arg == "--seed" && i+1 < argc) {
			seed = stoi(argv[++i]);
		} else if(arg == "--task-eff" && i+1 < argc) {
			params.task_efficiency_weight = stod(argv[++i]);
		} else if(arg == "--dist-penalty" && i+1 < argc) {
			params.distance_penalty = stod(argv[++i]);
		} else if(arg == "--ownership-factor" && i+1 < argc) {
			params.ownership_distance_factor = stod(argv[++i]);
		} else if(arg == "--path-threshold" && i+1 < argc) {
			params.path_cost_threshold = stod(argv[++i]);
		} else if(arg == "--bfs-random" && i+1 < argc) {
			params.bfs_randomness = stod(argv[++i]);
		} else if(arg == "--fix-task-eff") {
			fix_flags.fix_task_eff = true;
		} else if(arg == "--fix-dist-penalty") {
			fix_flags.fix_dist_penalty = true;
		} else if(arg == "--fix-ownership-factor") {
			fix_flags.fix_ownership = true;
		} else if(arg == "--fix-path-threshold") {
			fix_flags.fix_path_threshold = true;
		} else if(arg == "--fix-bfs-random") {
			fix_flags.fix_bfs_random = true;
		}
	}
	
	if(map_name.empty()) {
		cerr << "Usage: " << argv[0] << " -m <map> [options]" << endl;
		cerr << "Maps: A, B, C, D, E, F" << endl;
		cerr << "Options:" << endl;
		cerr << "  -v, --verbose          Enable console output" << endl;
		cerr << "  --local-search         Enable local search" << endl;
		cerr << "  --iterations N         Number of iterations (default: 50)" << endl;
		cerr << "  --seed N               Random seed (default: random)" << endl;
		cerr << "  --task-eff VALUE       Task efficiency weight (default: 1.0)" << endl;
		cerr << "  --dist-penalty VALUE   Distance penalty (default: 1.0)" << endl;
		cerr << "  --ownership-factor V   Ownership factor (default: 2.0)" << endl;
		cerr << "  --path-threshold V     Path threshold (default: 1.0)" << endl;
		cerr << "  --bfs-random VALUE     BFS randomness (default: 0.5)" << endl;
		cerr << "" << endl;
		cerr << "Local Search Parameter Fixing (use with --local-search):" << endl;
		cerr << "  --fix-task-eff         Fix task efficiency weight, randomize others" << endl;
		cerr << "  --fix-dist-penalty     Fix distance penalty, randomize others" << endl;
		cerr << "  --fix-ownership-factor Fix ownership factor, randomize others" << endl;
		cerr << "  --fix-path-threshold   Fix path threshold, randomize others" << endl;
		cerr << "  --fix-bfs-random       Fix BFS randomness, randomize others" << endl;
		cerr << "  (If none specified, all parameters are randomized)" << endl;
		return 1;
	}
	
	string input_file = "input/" + string(1, tolower(map_name[0])) + "_";
	if(map_name == "A") input_file += "example.txt";
	else if(map_name == "B") input_file += "single_arm.txt";
	else if(map_name == "C") input_file += "few_arms.txt";
	else if(map_name == "D") input_file += "tight_schedule.txt";
	else if(map_name == "E") input_file += "dense_workspace.txt";
	else if(map_name == "F") input_file += "decentralized.txt";
	
	// Generate random seed if not provided
	if(seed == -1) {
		random_device rd;
		seed = rd();
	}
	
	cout << "Simulation parameters:" << endl;
	cout << "Map: " << map_name << endl;
	cout << "Seed: " << seed << endl;
	cout << "Local Search: " << (local_search_mode ? "true" : "false") << endl;
	if(local_search_mode) {
		cout << "Iterations: " << iterations << endl;
		fix_flags.print();
	}
	cout << "Initial params: ";
	params.print();
	
	// Read input
	ifstream in(input_file);
	if(!in) {
		cerr << "Could not open file: " << input_file << endl;
		return 1;
	}
	
	int W, H, R, M, T, L;
	in >> W >> H >> R >> M >> T >> L;
	
	vector<Point> ms(M);
	for(int i = 0; i < M; ++i) in >> ms[i].x >> ms[i].y;
	
	vi S(T), Len(T, 0);
	vector<vector<Point>> P(T);
	for(int t = 0; t < T; ++t) {
		int p;
		in >> S[t] >> p;
		P[t].reserve(p);
		while(p--) {
			int x, y;
			in >> x >> y;
			P[t].emplace_back(x, y);
		}
		for(int i = 1; i < P[t].size(); ++i) Len[t] += distance(P[t][i-1], P[t][i]);
	}
	
	// Solve with timing
	auto start_time = chrono::steady_clock::now();
	
	vector<Arm> arms;
	int score;
	SolverParams final_params = params;
	
	if(local_search_mode) {
		auto [best_arms, best_score, final_params_result] = local_search(
			W, H, R, M, T, L, ms, S, P, Len, params, iterations, seed, verbose, fix_flags
		);
		arms = best_arms;
		score = best_score;
		final_params = final_params_result;
	} else {
		mt19937 mt(seed);
		score = greedy_solver(W, H, R, M, T, L, ms, S, P, Len, arms, params, mt, verbose);
		final_params = params;
	}
	
	auto end_time = chrono::steady_clock::now();
	double execution_time_seconds = chrono::duration<double>(end_time - start_time).count();
	double execution_time_minutes = execution_time_seconds / 60.0;
	
	// Write output
	string base_name = string(1, tolower(map_name[0]));
	string output_file = "output/" + base_name + "_" + to_string(score) + ".out";
	write_output(arms, output_file);
	
	// Write params JSON file with same base name
	string json_file = "output/" + base_name + "_" + to_string(score) + ".json";
	write_params_json(json_file, params, final_params, fix_flags, 
	                 map_name, score, iterations, seed, local_search_mode, execution_time_minutes);
	
	cout << "\nFinal score: " << (score < 1000 ? to_string(score) : to_string(score/1000) + " K") << endl;
	cout << "Saved: " << output_file << endl;
	cout << "Params: " << json_file << endl;
	
	return 0;
}