// lottery.cpp
// Include common code
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <random>

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
    std::vector<Process> processes = {
        {"P1", 0, 8, 2, 8, 0, 0},
        {"P2", 1, 4, 1, 4, 0, 0},
        {"P3", 2, 9, 3, 9, 0, 0},
        {"P4", 3, 5, 4, 5, 0, 0},
    };
    for (auto& p : processes) p.remaining_time = p.burst_time;

    // TODO: Assign tickets, e.g., vector<int> tickets(processes.size())
    // In loop: Build ticket list, draw random, run for quantum
    // Enhancement: Adjust tickets dynamically
    std::vector<int> tickets(processes.size());
    for (size_t i = 0; i < processes.size(); ++i) {
        int pr = std::max(1, processes[i].priority);
        tickets[i] = std::max(1, 10 / pr);
    }

    std::vector<int> order(processes.size());
    for (int i = 0; i < (int)order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (processes[a].arrival_time != processes[b].arrival_time)
            return processes[a].arrival_time < processes[b].arrival_time;
        return processes[a].id < processes[b].id;
    });

    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<std::pair<std::string, int>> gantt;
    int current_time = 0;
    int quantum = 4;
    int next_i = 0;

    auto any_left = [&]() {
        for (auto& p : processes) if (p.remaining_time > 0) return true;
        return false;
    };
    auto push_to_time = [&](int t) {
        if (t > current_time) {
            gantt.emplace_back("IDLE", t - current_time);
            current_time = t;
        }
    };

    while (any_left()) {
        while (next_i < (int)order.size() &&
               processes[order[next_i]].arrival_time <= current_time) {
            ++next_i;
        }

        std::vector<int> bag; bag.reserve(64);
        for (size_t i = 0; i < processes.size(); ++i) {
            if (processes[i].remaining_time > 0 &&
                processes[i].arrival_time <= current_time) {
                bag.insert(bag.end(), tickets[i], (int)i);
            }
        }

        if (bag.empty()) {
            if (next_i < (int)order.size())
                push_to_time(processes[order[next_i]].arrival_time);
            continue;
        }

        std::uniform_int_distribution<> dist(0, (int)bag.size() - 1);
        int idx = bag[dist(gen)];

        int slice = std::min(quantum, processes[idx].remaining_time);
        gantt.emplace_back(processes[idx].id, slice);
        processes[idx].remaining_time -= slice;
        current_time += slice;

        if (processes[idx].remaining_time == 0) {
            processes[idx].turnaround_time = current_time - processes[idx].arrival_time;
            processes[idx].waiting_time = processes[idx].turnaround_time - processes[idx].burst_time;
        }
    }

    calculateMetrics(processes, current_time);
    printGantt(gantt);
    return 0;
}
