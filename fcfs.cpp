#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>

// Include common code from Part 1
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

    // Sort by arrival time
    std::sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.arrival_time < b.arrival_time;
    });

    std::vector<std::pair<std::string, int>> gantt;
    int current_time = 0;

    // TODO: Implement scheduling loop
    /// ---- FCFS scheduling loop (run each to completion) ----
    for (auto& p : processes) {
        // If CPU is idle before the next arrival, skip forward (optional: record idle)
        if (current_time < p.arrival_time) {
            // Record idle time as a block in the Gantt chart (can omit if not desired)
            gantt.emplace_back("IDLE", p.arrival_time - current_time);
            current_time = p.arrival_time;
        }

        // Waiting time is how long it sat in the ready queue
        p.waiting_time = current_time - p.arrival_time;

        // Run to completion
        gantt.emplace_back(p.id, p.burst_time);
        current_time += p.burst_time;

        // Turnaround = finish - arrival
        p.turnaround_time = current_time - p.arrival_time;

        // Not used for FCFS, but keep consistent
        p.remaining_time = 0;
    }
    // -------------------------------------------------------

    calculateMetrics(processes, current_time);
    printGantt(gantt);
    return 0;
}