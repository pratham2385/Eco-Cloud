#pragma once
#include <string>

using namespace std;

enum ProcessState { NEW, READY, RUNNING, WAITING, TERMINATED, GREEN_WAIT };

struct PCB {
    std::string job_id;   // e.g., "JOB_001"
    int arrival_time;
    int burst_time;
    int priority;         // Standard OS priority
    ProcessState state;
    
    // Eco-Cloud Research Attributes
    double estimated_energy; 
    double carbon_footprint;

    // Constructor
    PCB(string id, int arr, int burst, int prio, double energy) {
        job_id = id;
        arrival_time = arr;
        burst_time = burst;
        priority = prio;
        estimated_energy = energy;
        carbon_footprint = 0.0;
        state = NEW;
    }
};