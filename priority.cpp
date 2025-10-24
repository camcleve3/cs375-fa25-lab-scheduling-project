// priority.cpp
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>

struct Process {
    std::string id;
    int arrival_time;
    int burst_time;
    int priority;
    int remaining_time;
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
    double cpu_util = (double)total_time / total_time * 100;

    std::cout << "Avg Waiting Time: " << avg_wait << "\n";
    std::cout << "Avg Turnaround Time: " << avg_turn << "\n";
    std::cout << "CPU Utilization: " << cpu_util << "%\n";
}

void printGantt(const std::vector<std::pair<std::string, int>>& gantt) {
    std::cout << "Gantt Chart: ";
    for (auto& entry : gantt) {
        std::cout << "(" << entry.first << " " << entry.second << ") ";
    }
    std::cout << "\n";
}

int main() {
    std::vector<Process> processes = {
        {"P1", 0, 8, 2, 8, 0, 0},
        {"P2", 1, 4, 1, 4, 0, 0},
        {"P3", 2, 9, 3, 9, 0, 0},
        {"P4", 3, 5, 4, 5, 0, 0},
    };

    std::vector<int> order(processes.size());
    for (int i = 0; i < (int)order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (processes[a].arrival_time != processes[b].arrival_time)
            return processes[a].arrival_time < processes[b].arrival_time;
        return processes[a].id < processes[b].id;
    });

    const int AGE_QUANTUM = 3;
    std::vector<int> aged_steps(processes.size(), 0);
    std::vector<std::pair<std::string, int>> gantt;
    int current_time = 0;
    std::vector<int> ready;
    int next_i = 0;

    auto push_arrivals = [&]() {
        while (next_i < (int)order.size() &&
               processes[order[next_i]].arrival_time <= current_time) {
            ready.push_back(order[next_i]);
            ++next_i;
        }
    };

    while (next_i < (int)order.size() || !ready.empty()) {
        push_arrivals();

        if (ready.empty()) {
            int next_arrival = processes[order[next_i]].arrival_time;
            int idle_len = next_arrival - current_time;
            if (idle_len > 0) gantt.emplace_back("IDLE", idle_len);
            current_time = next_arrival;
            push_arrivals();
        }

        for (int idx : ready) {
            int waited = current_time - processes[idx].arrival_time;
            int should_have_steps = std::max(0, waited / AGE_QUANTUM);
            while (aged_steps[idx] < should_have_steps) {
                if (processes[idx].priority > 0) processes[idx].priority -= 1;
                aged_steps[idx] += 1;
            }
        }

        int best_pos = 0;
        for (int k = 1; k < (int)ready.size(); ++k) {
            int i_best = ready[best_pos], i_k = ready[k];
            if (processes[i_k].priority < processes[i_best].priority ||
               (processes[i_k].priority == processes[i_best].priority &&
                (processes[i_k].arrival_time < processes[i_best].arrival_time ||
                 (processes[i_k].arrival_time == processes[i_best].arrival_time &&
                  processes[i_k].id < processes[i_best].id)))) {
                best_pos = k;
            }
        }

        int idx = ready[best_pos];
        ready.erase(ready.begin() + best_pos);

        processes[idx].waiting_time = current_time - processes[idx].arrival_time;
        gantt.emplace_back(processes[idx].id, processes[idx].burst_time);
        current_time += processes[idx].burst_time;
        processes[idx].turnaround_time = current_time - processes[idx].arrival_time;
        processes[idx].remaining_time = 0;
    }

    calculateMetrics(processes, current_time);
    printGantt(gantt);
    return 0;
}
