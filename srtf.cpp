// srtf.cpp
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <climits>

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
    // Example data (same pattern as your earlier parts)
    // id, arrival, burst, priority, remaining, waiting, turnaround
    std::vector<Process> processes = {
        {"P1", 0, 8, 2, 8, 0, 0},
        {"P2", 1, 4, 1, 4, 0, 0},
        {"P3", 2, 9, 3, 9, 0, 0},
        {"P4", 3, 5, 4, 5, 0, 0},
    };

    // Initialize remaining_time to burst_time
    for (auto& p : processes) p.remaining_time = p.burst_time;

    std::vector<std::pair<std::string, int>> gantt;
    int current_time = 0;

    const int n = (int)processes.size();
    std::vector<int> completion(n, -1);

    // Helper to check if all are finished
    auto all_done = [&]() {
        for (const auto& p : processes) if (p.remaining_time > 0) return false;
        return true;
    };

    // Track the current block for the Gantt chart
    std::string last_id = "";
    int block_len = 0;

    auto push_block_if_any = [&]() {
        if (block_len > 0 && !last_id.empty()) {
            gantt.emplace_back(last_id, block_len);
            block_len = 0;
            last_id.clear();
        }
    };

    while (!all_done()) {
        // Choose the arrived process with the smallest remaining_time
        int best_idx = -1;
        for (int i = 0; i < n; ++i) {
            if (processes[i].arrival_time <= current_time && processes[i].remaining_time > 0) {
                if (best_idx == -1 ||
                    processes[i].remaining_time < processes[best_idx].remaining_time ||
                    (processes[i].remaining_time == processes[best_idx].remaining_time &&
                     (processes[i].arrival_time < processes[best_idx].arrival_time ||
                      (processes[i].arrival_time == processes[best_idx].arrival_time &&
                       processes[i].id < processes[best_idx].id)))) {
                    best_idx = i;
                }
            }
        }

        // If nothing is ready, jump to the next arrival (idle)
        if (best_idx == -1) {
            int next_arrival = INT_MAX;
            for (int i = 0; i < n; ++i) {
                if (processes[i].remaining_time > 0) {
                    next_arrival = std::min(next_arrival, processes[i].arrival_time);
                }
            }
            // Only jump forward if needed
            if (next_arrival > current_time) {
                // Close any running block before inserting an IDLE block
                push_block_if_any();
                gantt.emplace_back("IDLE", next_arrival - current_time);
                current_time = next_arrival;
            }
            continue; // try again to pick a process at this new time
        }

        // Context switch? If process changes, close the previous block
        if (last_id != processes[best_idx].id) {
            push_block_if_any();
            last_id = processes[best_idx].id;
            block_len = 0;
        }

        // Run for one time unit
        processes[best_idx].remaining_time -= 1;
        current_time += 1;
        block_len += 1;

        // If finished, record completion
        if (processes[best_idx].remaining_time == 0) {
            completion[best_idx] = current_time;
        }
    }

    // Flush last running block
    push_block_if_any();

    // Compute waiting/turnaround from completion times
    for (int i = 0; i < n; ++i) {
        processes[i].turnaround_time = completion[i] - processes[i].arrival_time;
        processes[i].waiting_time    = processes[i].turnaround_time - processes[i].burst_time;
        processes[i].remaining_time  = 0;
    }

    calculateMetrics(processes, current_time);
    printGantt(gantt);
    return 0;
}
