// simulator.cpp
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <fstream>
#include <numeric>
#include <random>
#include <chrono>
#include <map>
#include <memory>
#include <climits>

struct Process {
    std::string id;
    int arrival_time;
    int burst_time;
    int priority;              // lower = higher
    int remaining_time = 0;
    int waiting_time = 0;
    int turnaround_time = 0;
    int deadline = 0;          // for EDF
    double vruntime = 0.0;     // for CFS
};

void calculateMetrics(const std::vector<Process>& ps, int total_time,
                      double& avg_wait, double& avg_turn, double& cpu_util, double& throughput) {
    avg_wait = 0; avg_turn = 0;
    for (auto& p : ps) { avg_wait += p.waiting_time; avg_turn += p.turnaround_time; }
    int n = (int)ps.size();
    avg_wait /= n; avg_turn /= n;
    int busy = std::accumulate(ps.begin(), ps.end(), 0,
                               [](int s, const Process& p){ return s + p.burst_time; });
    cpu_util = total_time ? (double)busy / total_time * 100.0 : 0.0;
    throughput = total_time ? (double)n / total_time : 0.0;
}

void printGantt(const std::vector<std::pair<std::string,int>>& gantt) {
    std::cout << "Gantt Chart: ";
    for (auto& e : gantt) std::cout << "(" << e.first << " " << e.second << ") ";
    std::cout << "\n";
}

void printResults(const std::vector<Process>& processes, int total_time,
                  const std::vector<std::pair<std::string,int>>& gantt) {
    double avg_wait, avg_turn, cpu_util, throughput;
    calculateMetrics(processes, total_time, avg_wait, avg_turn, cpu_util, throughput);
    printGantt(gantt);
    std::cout << "Average Waiting Time: "  << avg_wait   << "\n";
    std::cout << "Average Turnaround Time: " << avg_turn << "\n";
    std::cout << "CPU Utilization: "       << cpu_util   << "%\n";
    std::cout << "Throughput: "            << throughput << " processes/unit time\n";
}

// ---------- Base ----------
class Scheduler {
public:
    virtual ~Scheduler() = default;
    virtual void schedule(std::vector<Process>& ps,
                          std::vector<std::pair<std::string,int>>& gantt,
                          int& total_time) = 0;
};

// Helpers
static inline void finalizeBlock(const std::string& id, int start, int now,
                                 std::vector<std::pair<std::string,int>>& gantt) {
    if (start != -1 && !id.empty() && now > start) gantt.emplace_back(id, now - start);
}
static inline bool anyLeft(const std::vector<Process>& ps) {
    for (auto& p : ps) if (p.remaining_time > 0) return true;
    return false;
}
static inline void initRemaining(std::vector<Process>& ps) {
    for (auto& p : ps) p.remaining_time = p.burst_time;
}

// ---------- FCFS ----------
class FCFSScheduler : public Scheduler {
public:
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps);
        std::sort(ps.begin(), ps.end(), [](auto& a, auto& b){ return a.arrival_time < b.arrival_time; });
        int t = 0;
        for (auto& p : ps) {
            if (t < p.arrival_time) { gantt.emplace_back("IDLE", p.arrival_time - t); t = p.arrival_time; }
            gantt.emplace_back(p.id, p.burst_time);
            t += p.burst_time;
            p.turnaround_time = t - p.arrival_time;
            p.waiting_time = p.turnaround_time - p.burst_time;
        }
        total_time = t;
    }
};

// ---------- SJF (non-preemptive) ----------
class SJFScheduler : public Scheduler {
public:
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps);
        std::vector<int> ord(ps.size());
        for (int i=0;i<(int)ord.size();++i) ord[i]=i;
        std::sort(ord.begin(), ord.end(), [&](int a,int b){
            if (ps[a].arrival_time!=ps[b].arrival_time) return ps[a].arrival_time<ps[b].arrival_time;
            return ps[a].id<ps[b].id;
        });
        int t=0, next=0; std::vector<int> rq;
        auto push=[&]{ while(next<(int)ord.size() && ps[ord[next]].arrival_time<=t) rq.push_back(ord[next++]); };
        auto pick=[&]{ int b=-1; for(int i=0;i<(int)rq.size();++i)
            if (b==-1 || ps[rq[i]].burst_time<ps[rq[b]].burst_time ||
               (ps[rq[i]].burst_time==ps[rq[b]].burst_time && ps[rq[i]].id<ps[rq[b]].id)) b=i; return b; };
        while(next<(int)ord.size() || !rq.empty()){
            push();
            if(rq.empty()){ int na=ps[ord[next]].arrival_time; gantt.emplace_back("IDLE",na-t); t=na; push(); }
            int pos=pick(); int idx=rq[pos]; rq.erase(rq.begin()+pos);
            gantt.emplace_back(ps[idx].id, ps[idx].burst_time);
            t+=ps[idx].burst_time;
            ps[idx].turnaround_time=t-ps[idx].arrival_time;
            ps[idx].waiting_time=ps[idx].turnaround_time-ps[idx].burst_time;
        }
        total_time=t;
    }
};

// ---------- SRTF ----------
class SRTFScheduler : public Scheduler {
public:
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps);
        int n=(int)ps.size(); std::vector<int> comp(n,-1);
        int t=0; std::string last=""; int start=-1;
        auto allDone=[&]{return !anyLeft(ps);};
        while(!allDone()){
            int best=-1;
            for(int i=0;i<n;++i) if(ps[i].arrival_time<=t && ps[i].remaining_time>0){
                if(best==-1 || ps[i].remaining_time<ps[best].remaining_time ||
                  (ps[i].remaining_time==ps[best].remaining_time && ps[i].id<ps[best].id)) best=i;
            }
            if(best==-1){ // idle
                int na=INT_MAX; for(int i=0;i<n;++i) if(ps[i].remaining_time>0) na=std::min(na, ps[i].arrival_time);
                finalizeBlock(last,start,t,gantt); gantt.emplace_back("IDLE", na-t); t=na; continue;
            }
            if(last!=ps[best].id){ finalizeBlock(last,start,t,gantt); last=ps[best].id; start=t; }
            ps[best].remaining_time--; t++;
            if(ps[best].remaining_time==0) comp[best]=t;
        }
        finalizeBlock(last,start,t,gantt);
        for(int i=0;i<n;++i){ ps[i].turnaround_time=comp[i]-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; }
        total_time=t;
    }
};

// ---------- Priority (non-preemptive, lower value = higher) ----------
class PriorityScheduler : public Scheduler {
public:
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps);
        std::vector<int> ord(ps.size()); for(int i=0;i<(int)ord.size();++i) ord[i]=i;
        std::sort(ord.begin(), ord.end(), [&](int a,int b){
            if(ps[a].arrival_time!=ps[b].arrival_time) return ps[a].arrival_time<ps[b].arrival_time;
            return ps[a].id<ps[b].id;
        });
        int t=0,next=0; std::vector<int> rq;
        auto push=[&]{ while(next<(int)ord.size() && ps[ord[next]].arrival_time<=t) rq.push_back(ord[next++]); };
        auto pick=[&]{ int b=0; for(int i=1;i<(int)rq.size();++i)
            if(ps[rq[i]].priority<ps[rq[b]].priority ||
              (ps[rq[i]].priority==ps[rq[b]].priority && ps[rq[i]].id<ps[rq[b]].id)) b=i; return b; };
        while(next<(int)ord.size() || !rq.empty()){
            push(); if(rq.empty()){ int na=ps[ord[next]].arrival_time; gantt.emplace_back("IDLE",na-t); t=na; push(); }
            int pos=pick(); int idx=rq[pos]; rq.erase(rq.begin()+pos);
            gantt.emplace_back(ps[idx].id, ps[idx].burst_time);
            t+=ps[idx].burst_time;
            ps[idx].turnaround_time=t-ps[idx].arrival_time;
            ps[idx].waiting_time=ps[idx].turnaround_time-ps[idx].burst_time;
        }
        total_time=t;
    }
};

// ---------- Round Robin ----------
class RoundRobinScheduler : public Scheduler {
    int quantum;
public:
    explicit RoundRobinScheduler(int q):quantum(q){}
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps);
        std::queue<Process*> q; int t=0; size_t idx=0;
        int last_start=-1; std::string last="";
        while(!q.empty() || idx<ps.size()){
            while(idx<ps.size() && ps[idx].arrival_time<=t) q.push(&ps[idx++]);
            if(q.empty()){ if(idx<ps.size()) t=ps[idx].arrival_time; continue; }
            Process* cur=q.front(); q.pop();
            int slice=std::min(quantum, cur->remaining_time);
            if(last!=cur->id || last_start==-1){ finalizeBlock(last,last_start,t,gantt); last=cur->id; last_start=t; }
            cur->remaining_time-=slice; t+=slice;
            while(idx<ps.size() && ps[idx].arrival_time<=t) q.push(&ps[idx++]);
            if(cur->remaining_time>0) q.push(cur);
            else { cur->turnaround_time=t-cur->arrival_time; cur->waiting_time=cur->turnaround_time-cur->burst_time; }
        }
        finalizeBlock(last,last_start,t,gantt);
        total_time=t;
    }
};

// ---------- MLQ (2 queues: high RR q=4 if priority<3, low FCFS) ----------
class MLQScheduler : public Scheduler {
public:
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps);
        std::queue<Process*> high, low; int t=0; size_t idx=0; int q=4;
        auto push=[&]{ while(idx<ps.size() && ps[idx].arrival_time<=t){
                (ps[idx].priority<3?high:low).push(&ps[idx]); ++idx; } };
        while(anyLeft(ps)){
            push();
            if(!high.empty()){
                Process* p=high.front(); high.pop(); int slice=std::min(q,p->remaining_time);
                gantt.emplace_back(p->id,slice); t+=slice; p->remaining_time-=slice; push();
                if(p->remaining_time>0) high.push(p);
                else { p->turnaround_time=t-p->arrival_time; p->waiting_time=p->turnaround_time-p->burst_time; }
                continue;
            }
            if(!low.empty()){
                Process* p=low.front(); low.pop(); int ran=0;
                while(p->remaining_time>0){ push(); if(!high.empty()) break; p->remaining_time--; t++; ran++; push(); }
                if(ran>0) gantt.emplace_back(p->id,ran);
                if(p->remaining_time==0){ p->turnaround_time=t-p->arrival_time; p->waiting_time=p->turnaround_time-p->burst_time; }
                else low.push(p);
                continue;
            }
            if(idx<ps.size()){ int na=ps[idx].arrival_time; gantt.emplace_back("IDLE",na-t); t=na; }
        }
        total_time=t;
    }
};

// ---------- MLFQ (3 queues RR with quanta 2,4,8; demote on full slice; simple aging) ----------
class MLFQScheduler : public Scheduler {
public:
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps);
        std::queue<Process*> q[3]; int quantum[3]={2,4,8}; int t=0; size_t idx=0; const int AGE=10;
        std::vector<int> last_enq(ps.size(),0), level(ps.size(),0);
        auto enq=[&](int i,int lv){ level[i]=std::max(0,std::min(2,lv)); q[level[i]].push(&ps[i]); last_enq[i]=t; };
        auto push=[&]{ while(idx<ps.size() && ps[idx].arrival_time<=t){ enq((int)idx,0); ++idx; } };
        std::vector<int> comp(ps.size(),-1);
        while(anyLeft(ps)){
            push(); int qi=-1; for(int i=0;i<3;++i) if(!q[i].empty()){qi=i; break;}
            if(qi==-1){ if(idx<ps.size()){int na=ps[idx].arrival_time; gantt.emplace_back("IDLE",na-t); t=na;} continue; }
            Process* p=q[qi].front(); q[qi].pop(); int id=(int)(p-&ps[0]);
            if(qi>0 && t-last_enq[id]>=AGE){ enq(id,qi-1); continue; }
            int ran=0; while(ran<quantum[qi] && p->remaining_time>0){
                p->remaining_time--; t++; ran++; push(); if(qi>0 && !q[0].empty()) break;
            }
            if(ran>0) gantt.emplace_back(p->id,ran);
            if(p->remaining_time==0) comp[id]=t;
            else enq(id, (ran==quantum[qi] && qi<2)? qi+1 : qi);
        }
        for(size_t i=0;i<ps.size();++i){ ps[i].turnaround_time=comp[i]-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; }
        // compute total_time as the max (arrival_time + turnaround_time) across processes
        total_time = 0;
        for (const auto& p : ps) {
            total_time = std::max(total_time, p.arrival_time + p.turnaround_time);
        }
    }
};

// ---------- Lottery (tickets 10/priority; RR quantum=4; probabilistic pick) ----------
class LotteryScheduler : public Scheduler {
public:
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps);
        std::vector<int> tickets(ps.size());
        for (size_t i=0;i<ps.size();++i){ int pr=std::max(1, ps[i].priority); tickets[i]=std::max(1,10/pr); }
        std::mt19937 gen((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
        int t=0, q=4; size_t idx=0;
        auto pushTo=[&](int tt){ if(tt>t){ gantt.emplace_back("IDLE",tt-t); t=tt; } };
        while(anyLeft(ps)){
            while(idx<ps.size() && ps[idx].arrival_time<=t) ++idx;
            std::vector<int> bag;
            for(size_t i=0;i<ps.size();++i)
                if(ps[i].remaining_time>0 && ps[i].arrival_time<=t) bag.insert(bag.end(), tickets[i], (int)i);
            if(bag.empty()){ if(idx<ps.size()) pushTo(ps[idx].arrival_time); continue; }
            std::uniform_int_distribution<> dist(0,(int)bag.size()-1);
            int i = bag[dist(gen)];
            int slice=std::min(q, ps[i].remaining_time);
            gantt.emplace_back(ps[i].id, slice); ps[i].remaining_time-=slice; t+=slice;
            if(ps[i].remaining_time==0){ ps[i].turnaround_time=t-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; }
        }
        total_time=t;
    }
};

// ---------- CFS (simplified) ----------
class CFSScheduler : public Scheduler {
public:
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps); for(auto& p: ps) p.vruntime=0.0;
        auto cmp=[](const Process* a,const Process* b){ return a->vruntime > b->vruntime; };
        std::priority_queue<Process*, std::vector<Process*>, decltype(cmp)> rq(cmp);
        int t=0; size_t idx=0; int base_slice=2; std::vector<int> comp(ps.size(),-1);
        auto push=[&]{ while(idx<ps.size() && ps[idx].arrival_time<=t) rq.push(&ps[idx++]); };
        auto weight=[&](const Process& p){ int pr=std::max(1,p.priority); return 1.0/pr; };
        while(anyLeft(ps)){
            push(); if(rq.empty()){ if(idx<ps.size()){ int na=ps[idx].arrival_time; gantt.emplace_back("IDLE",na-t); t=na; } continue; }
            Process* p=rq.top(); rq.pop(); int id=(int)(p-&ps[0]);
            int slice=std::min(base_slice, p->remaining_time);
            gantt.emplace_back(p->id,slice); p->remaining_time-=slice; t+=slice;
            double w=weight(*p); if(w<=0) w=1.0; p->vruntime += slice / w;
            push();
            if(p->remaining_time==0) comp[id]=t; else rq.push(p);
        }
        for(size_t i=0;i<ps.size();++i){ ps[i].turnaround_time=comp[i]-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; }
        total_time=t;
    }
};

// ---------- EDF (preemptive; if deadline==0 set arrival+2*burst) ----------
class EDFScheduler : public Scheduler {
public:
    void schedule(std::vector<Process>& ps, std::vector<std::pair<std::string,int>>& gantt, int& total_time) override {
        initRemaining(ps);
        for(auto& p: ps) if(p.deadline==0) p.deadline = p.arrival_time + 2*p.burst_time;
        auto cmp=[](const Process* a,const Process* b){ return a->deadline > b->deadline; };
        std::priority_queue<Process*, std::vector<Process*>, decltype(cmp)> rq(cmp);
        int t=0; size_t idx=0; std::vector<int> comp(ps.size(),-1);
        std::string last=""; int start=-1;
        auto push=[&]{ while(idx<ps.size() && ps[idx].arrival_time<=t) rq.push(&ps[idx++]); };
        auto flush=[&]{ finalizeBlock(last,start,t,gantt); last=""; start=-1; };
        while(anyLeft(ps)){
            push();
            if(rq.empty()){ if(idx<ps.size()){ flush(); gantt.emplace_back("IDLE", ps[idx].arrival_time - t); t = ps[idx].arrival_time; } continue; }
            Process* p=rq.top(); rq.pop();
            if(last!=p->id){ flush(); last=p->id; start=t; }
            p->remaining_time--; t++; push();
            if(p->remaining_time==0) comp[(int)(p-&ps[0])]=t; else rq.push(p);
        }
        finalizeBlock(last,start,t,gantt);
        for(size_t i=0;i<ps.size();++i){ ps[i].turnaround_time=comp[i]-ps[i].arrival_time; ps[i].waiting_time=ps[i].turnaround_time-ps[i].burst_time; }
        total_time=t;
    }
};

// ---------- IO & Input ----------
std::vector<Process> loadProcesses(const std::string& filename) {
    std::vector<Process> ps; std::ifstream f(filename);
    if(!f){ std::cerr<<"Error opening file: "<<filename<<"\n"; return ps; }
    std::string id; int at, bt, pri, dl;
    while(true){
        if(!(f>>id>>at>>bt>>pri)) break;
        if(f.peek()==' '||f.peek()=='\t'){ if(f>>dl) ps.push_back({id,at,bt,pri,0,0,0,dl}); else ps.push_back({id,at,bt,pri}); }
        else ps.push_back({id,at,bt,pri});
    }
    std::sort(ps.begin(), ps.end(), [](auto&a,auto&b){return a.arrival_time<b.arrival_time;});
    return ps;
}

std::vector<Process> generateRandomProcesses(int num) {
    std::vector<Process> ps;
    std::mt19937 gen((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> at(0,20), bt(1,10), pri(0,5);
    for(int i=0;i<num;++i){ ps.push_back({"P"+std::to_string(i+1), at(gen), bt(gen), pri(gen)}); }
    std::sort(ps.begin(), ps.end(), [](auto&a,auto&b){return a.arrival_time<b.arrival_time;});
    return ps;
}

// ---------- Main ----------
int main(int argc, char* argv[]) {
    std::map<std::string,std::string> args;
    for (int i = 1; i + 1 < argc; i += 2) args[argv[i]] = argv[i + 1];
    std::string sched = args["--scheduler"];
    std::string input = args["--input"];
    int quantum = args["--quantum"].empty()? 4 : std::stoi(args["--quantum"]);
    bool random = args.count("--random");

    std::vector<Process> processes;
    if (random) processes = generateRandomProcesses(10);
    else if (!input.empty()) processes = loadProcesses(input);
    else {
        processes = { {"P1",0,8,2}, {"P2",1,4,1}, {"P3",2,9,3}, {"P4",3,5,4} };
    }
    if (processes.empty()) { std::cerr<<"No processes loaded.\n"; return 1; }

    std::unique_ptr<Scheduler> scheduler;
    if (sched=="fcfs") scheduler = std::make_unique<FCFSScheduler>();
    else if (sched=="sjf") scheduler = std::make_unique<SJFScheduler>();
    else if (sched=="srtf") scheduler = std::make_unique<SRTFScheduler>();
    else if (sched=="prio" || sched=="priority") scheduler = std::make_unique<PriorityScheduler>();
    else if (sched=="rr") scheduler = std::make_unique<RoundRobinScheduler>(quantum);
    else if (sched=="mlq") scheduler = std::make_unique<MLQScheduler>();
    else if (sched=="mlfq") scheduler = std::make_unique<MLFQScheduler>();
    else if (sched=="lottery") scheduler = std::make_unique<LotteryScheduler>();
    else if (sched=="cfs") scheduler = std::make_unique<CFSScheduler>();
    else if (sched=="edf") scheduler = std::make_unique<EDFScheduler>();
    else { std::cerr<<"Unknown scheduler: "<<sched<<"\n"; return 1; }

    std::vector<std::pair<std::string,int>> gantt;
    int total_time = 0;
    scheduler->schedule(processes, gantt, total_time);
    printResults(processes, total_time, gantt);
    return 0;
}
