// cfs.cpp
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
    double vruntime;     // Added for CFS
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

struct CFSCompare {
    bool operator()(const Process* a, const Process* b) const {
        return a->vruntime > b->vruntime; // Min-heap on vruntime
    }
};

int main() {
    std::vector<Process> processes = {
        {"P1", 0, 8, 2, 8, 0, 0, 0.0},
        {"P2", 1, 4, 1, 4, 0, 0, 0.0},
        {"P3", 2, 9, 3, 9, 0, 0, 0.0},
        {"P4", 3, 5, 4, 5, 0, 0, 0.0},
    };
    for (auto& p : processes) p.remaining_time = p.burst_time;

    // TODO: Simulate, select min vruntime, run slice, update vruntime += slice / weight
    // Enhancement: Handle weights based on priority
    auto weight_of = [&](const Process& p) -> double {
        int pr = std::max(1, p.priority);     // treat priority as "nice"
        return 1.0 / pr;                       // lower priority value => higher weight
    };

    std::priority_queue<Process*, std::vector<Process*>, CFSCompare> ready_queue;

    std::vector<int> order(processes.size());
    for (int i = 0; i < (int)order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (processes[a].arrival_time != processes[b].arrival_time)
            return processes[a].arrival_time < processes[b].arrival_time;
        return processes[a].id < processes[b].id;
    });

    int current_time = 0;
    int next_i = 0;
    int base_slice = 2;
    std::vector<std::pair<std::string, int>> gantt;

    auto push_arrivals = [&]() {
        while (next_i < (int)order.size() &&
               processes[order[next_i]].arrival_time <= current_time) {
            ready_queue.push(&processes[order[next_i]]);
            ++next_i;
        }
    };

    auto any_left = [&]() {
        for (auto& p : processes) if (p.remaining_time > 0) return true;
        return false;
    };

    std::vector<int> completion(processes.size(), -1);

    while (any_left()) {
        push_arrivals();

        if (ready_queue.empty()) {
            if (next_i < (int)order.size()) {
                int next_arrival = processes[order[next_i]].arrival_time;
                if (next_arrival > current_time) {
                    gantt.emplace_back("IDLE", next_arrival - current_time);
                    current_time = next_arrival;
                }
            }
            continue;
        }

        Process* p = ready_queue.top(); ready_queue.pop();
        int idx = int(p - &processes[0]);

        int slice = std::min(base_slice, p->remaining_time);
        gantt.emplace_back(p->id, slice);
        p->remaining_time -= slice;
        current_time += slice;

        double w = weight_of(*p);
        if (w <= 0.0) w = 1.0;
        p->vruntime += slice / w;

        push_arrivals();

        if (p->remaining_time == 0) {
            completion[idx] = current_time;
        } else {
            ready_queue.push(p);
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
