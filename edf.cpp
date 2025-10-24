// edf.cpp
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
    int priority;
    int remaining_time;
    int waiting_time;
    int turnaround_time;
    int deadline;
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
    for (auto& entry : gantt) std::cout << "(" << entry.first << " " << entry.second << ") ";
    std::cout << "\n";
}

struct EDFCompare {
    bool operator()(const Process* a, const Process* b) const {
        return a->deadline > b->deadline; // earliest deadline first
    }
};

int main() {
    std::vector<Process> processes = {
        {"P1", 0, 8, 2, 8, 0, 0, 0},
        {"P2", 1, 4, 1, 4, 0, 0, 0},
        {"P3", 2, 9, 3, 9, 0, 0, 0},
        {"P4", 3, 5, 4, 5, 0, 0, 0},
    };

    for (auto& p : processes) {
        p.remaining_time = p.burst_time;
        p.deadline = p.arrival_time + p.burst_time * 2;
    }

    // TODO: Use priority_queue sorted by deadline (soonest first)
    // Preemptive if nearer deadline arrives
    // Enhancement: Check schedulability
    std::priority_queue<Process*, std::vector<Process*>, EDFCompare> ready_queue;

    int current_time = 0;
    int next_i = 0;
    std::vector<int> order(processes.size());
    for (int i = 0; i < (int)order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return processes[a].arrival_time < processes[b].arrival_time;
    });

    std::vector<std::pair<std::string, int>> gantt;
    std::vector<int> completion(processes.size(), -1);

    auto push_arrivals = [&]() {
        while (next_i < (int)order.size() &&
               processes[order[next_i]].arrival_time <= current_time) {
            ready_queue.push(&processes[order[next_i]]);
            ++next_i;
        }
    };

    auto all_done = [&]() {
        for (auto& p : processes) if (p.remaining_time > 0) return false;
        return true;
    };

    std::string last_id;
    int block_len = 0;

    auto flush_block = [&]() {
        if (!last_id.empty() && block_len > 0) {
            gantt.emplace_back(last_id, block_len);
            last_id.clear();
            block_len = 0;
        }
    };

    while (!all_done()) {
        push_arrivals();

        if (ready_queue.empty()) {
            if (next_i < (int)order.size()) {
                int next_arrival = processes[order[next_i]].arrival_time;
                gantt.emplace_back("IDLE", next_arrival - current_time);
                current_time = next_arrival;
            }
            continue;
        }

        Process* p = ready_queue.top();
        ready_queue.pop();

        if (last_id != p->id) {
            flush_block();
            last_id = p->id;
        }

        p->remaining_time -= 1;
        current_time += 1;
        block_len += 1;

        push_arrivals();

        if (p->remaining_time == 0) {
            completion[p - &processes[0]] = current_time;
        } else {
            ready_queue.push(p);
        }
    }

    flush_block();

    for (size_t i = 0; i < processes.size(); ++i) {
        processes[i].turnaround_time = completion[i] - processes[i].arrival_time;
        processes[i].waiting_time = processes[i].turnaround_time - processes[i].burst_time;
    }

    calculateMetrics(processes, current_time);
    printGantt(gantt);
    return 0;
}
