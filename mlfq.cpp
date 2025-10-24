// mlfq.cpp
// Include common code
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>

struct Process {
    std::string id;
    int arrival_time;
    int burst_time;
    int priority;        // Lower number = higher priority
    int remaining_time;  // For preemptive
    int waiting_time;
    int turnaround_time;
};

void calculateMetrics(std::vector<Process>& processes, int total_time) {
    double avg_wait = 0, avg_turn = 0;
    for (auto& p : processes) {
        avg_wait += p.waiting_time;
        avg_turn += p.turnaround_time;
    }
    avg_wait /= processes.size();
    avg_turn /= processes.size();
    double cpu_util = (double)total_time / total_time * 100; // Simplified; adjust if idle time exists

    std::cout << "Avg Waiting Time: " << avg_wait << "\n";
    std::cout << "Avg Turnaround Time: " << avg_turn << "\n";
    std::cout << "CPU Utilization: " << cpu_util << "%\n";
}

void printGantt(const std::vector<std::pair<std::string, int>>& gantt) {
    std::cout << "Gantt Chart: ";
    for (auto& entry : gantt) std::cout << "(" << entry.first << " " << entry.second << ") ";
    std::cout << "\n";
}

int main() {
    // std::vector<std::queue<Process*>> queues(3);
    std::vector<Process> processes = {
        {"P1", 0, 8, 2, 8, 0, 0},
        {"P2", 1, 4, 1, 4, 0, 0},
        {"P3", 2, 9, 3, 9, 0, 0},
        {"P4", 3, 5, 4, 5, 0, 0},
    };
    for (auto& p : processes) p.remaining_time = p.burst_time;

    // Quanta: 2, 4, 8 for levels
    // TODO: Simulate, move processes down on full quantum use
    // Prefer higher queues
    // Enhancement: Promote if waiting too long
    std::queue<Process*> q[3];
    int quantum[3] = {2, 4, 8};
    std::vector<int> level(processes.size(), 0);
    std::vector<int> last_enq(processes.size(), 0);
    const int AGE = 10;

    std::vector<int> order(processes.size());
    for (int i = 0; i < (int)order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (processes[a].arrival_time != processes[b].arrival_time)
            return processes[a].arrival_time < processes[b].arrival_time;
        return processes[a].id < processes[b].id;
    });

    auto enqueue_at = [&](int idx, int lvl, int now) {
        level[idx] = std::max(0, std::min(2, lvl));
        q[level[idx]].push(&processes[idx]);
        last_enq[idx] = now;
    };

    int next_i = 0, current_time = 0;
    std::vector<std::pair<std::string, int>> gantt;

    auto push_arrivals = [&]() {
        while (next_i < (int)order.size() &&
               processes[order[next_i]].arrival_time <= current_time) {
            int idx = order[next_i++];
            enqueue_at(idx, 0, current_time); // start in top queue
        }
    };

    auto any_left = [&]() {
        for (auto& p : processes) if (p.remaining_time > 0) return true;
        return false;
    };

    std::vector<int> completion(processes.size(), -1);

    while (any_left()) {
        push_arrivals();

        int qi = -1;
        for (int i = 0; i < 3; ++i) if (!q[i].empty()) { qi = i; break; }

        if (qi == -1) {
            if (next_i < (int)order.size()) {
                int next_arrival = processes[order[next_i]].arrival_time;
                if (next_arrival > current_time) {
                    gantt.emplace_back("IDLE", next_arrival - current_time);
                    current_time = next_arrival;
                }
            }
            continue;
        }

        Process* p = q[qi].front(); q[qi].pop();
        int idx = int(p - &processes[0]);

        // simple promotion on long wait
        if (qi > 0 && current_time - last_enq[idx] >= AGE) {
            enqueue_at(idx, qi - 1, current_time);
            continue;
        }

        int ran = 0, qlim = quantum[qi];
        while (ran < qlim && p->remaining_time > 0) {
            p->remaining_time -= 1;
            current_time += 1;
            ran += 1;
            push_arrivals();
            // preempt if higher queue gets work
            if (qi > 0 && (!q[0].empty())) break;
        }
        if (ran > 0) gantt.emplace_back(p->id, ran);

        if (p->remaining_time == 0) {
            completion[idx] = current_time;
        } else {
            // demote if used full quantum without preemption; otherwise stay
            int next_lvl = (ran == qlim && (qi < 2)) ? qi + 1 : qi;
            enqueue_at(idx, next_lvl, current_time);
        }
    }

    for (size_t i = 0; i < processes.size(); ++i) {
        processes[i].turnaround_time = completion[i] - processes[i].arrival_time;
        processes[i].waiting_time = processes[i].turnaround_time - processes[i].burst_time;
    }

    calculateMetrics(processes, current_time);
    printGantt(gantt);
    return 0;
}
