// rr.cpp
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

// Function to print text-based Gantt chart
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
    for (auto& p : processes) p.remaining_time = p.burst_time;

    std::queue<Process*> ready_queue;
    std::vector<std::pair<std::string, int>> gantt;
    int current_time = 0;
    int quantum = 4;

    // TODO: Time loop
    // Add arrived to ready_queue
    // If !empty, take front, run min(quantum, remaining)
    // If remaining >0, push back; else complete

    std::vector<int> order(processes.size());
    for (int i = 0; i < (int)order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        if (processes[a].arrival_time != processes[b].arrival_time)
            return processes[a].arrival_time < processes[b].arrival_time;
        return processes[a].id < processes[b].id;
    });

    int next_i = 0;
    auto push_arrivals = [&]() {
        while (next_i < (int)order.size() &&
               processes[order[next_i]].arrival_time <= current_time) {
            ready_queue.push(&processes[order[next_i]]);
            ++next_i;
        }
    };

    while (true) {
        push_arrivals();

        if (ready_queue.empty()) {
            if (next_i >= (int)order.size()) break;
            int next_arrival = processes[order[next_i]].arrival_time;
            if (next_arrival > current_time) {
                gantt.emplace_back("IDLE", next_arrival - current_time);
                current_time = next_arrival;
            }
            push_arrivals();
            if (ready_queue.empty() && next_i >= (int)order.size()) break;
        }

        if (ready_queue.empty()) continue;

        Process* p = ready_queue.front();
        ready_queue.pop();

        int slice = std::min(quantum, p->remaining_time);
        gantt.emplace_back(p->id, slice);
        current_time += slice;
        p->remaining_time -= slice;

        push_arrivals();

        if (p->remaining_time > 0) {
            ready_queue.push(p);
        } else {
            p->turnaround_time = current_time - p->arrival_time;
            p->waiting_time = p->turnaround_time - p->burst_time;
        }
    }

    calculateMetrics(processes, current_time);
    printGantt(gantt);
    return 0;
}
