// src/os_engine/os_core.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "PCB.h"

#define QUEUE_CAPACITY 5
#define TOTAL_JOBS 8

// --- SHARED RESOURCES ---
PCB ready_queue[QUEUE_CAPACITY];
int queue_front = 0;
int queue_rear = 0;
int jobs_produced = 0;
int jobs_consumed = 0;

// Shared Energy Budget (Cooperating Processes)
int global_energy_budget = 1000; // Watts

// --- CONCURRENCY PRIMITIVES ---
pthread_mutex_t queue_mutex;    // Protects the Ready Queue
pthread_mutex_t energy_mutex;   // Protects the Global Energy Budget (Critical Section)
sem_t empty_slots;              // Semaphore tracking empty slots in queue
sem_t full_slots;               // Semaphore tracking filled slots in queue

// --- SYSTEM CALL SIMULATION ---
void sim_request_resource(PCB* job, int watts_needed) {
    job->mode = KERNEL_MODE; // Switch to Kernel Mode for hardware access
    printf("[SYSTEM CALL] %s requesting %d Watts (Mode: KERNEL)\n", job->job_id, watts_needed);
    
    // CRITICAL SECTION: Modifying shared energy
    pthread_mutex_lock(&energy_mutex);
    if (global_energy_budget >= watts_needed) {
        global_energy_budget -= watts_needed;
        printf("   ->[HARDWARE] Energy allocated. Remaining Budget: %d W\n", global_energy_budget);
    } else {
        printf("   -> [HARDWARE] WARNING: Power Grid Overload! Cannot allocate energy.\n");
    }
    pthread_mutex_unlock(&energy_mutex);
    
    job->mode = USER_MODE; // Switch back to User Mode
}

// --- PRODUCER: JOB GENERATOR ---
void* job_generator(void* arg) {
    for (int i = 1; i <= TOTAL_JOBS; i++) {
        // Create a new PCB
        PCB new_job;
        sprintf(new_job.job_id, "JOB_%03d", i);
        new_job.burst_time = (rand() % 3) + 1; // 1 to 3 seconds
        new_job.state = STATE_NEW;
        new_job.mode = USER_MODE;

        // Wait for an empty slot in the Ready Queue
        sem_wait(&empty_slots);
        
        // Lock Queue to insert
        pthread_mutex_lock(&queue_mutex);
        
        new_job.state = STATE_READY;
        ready_queue[queue_rear] = new_job;
        queue_rear = (queue_rear + 1) % QUEUE_CAPACITY;
        printf("[PRODUCER] Admitted %s to Ready Queue.\n", new_job.job_id);
        
        pthread_mutex_unlock(&queue_mutex);
        
        // Signal that a slot is full
        sem_post(&full_slots);
        
        sleep(1); // Simulate time between job arrivals
    }
    return NULL;
}

// --- CONSUMER: CPU CORE ---
void* cpu_core(void* core_id) {
    int id = *((int*)core_id);
    
    while (1) {
        // Check if all jobs are done
        pthread_mutex_lock(&queue_mutex);
        if (jobs_consumed >= TOTAL_JOBS) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        pthread_mutex_unlock(&queue_mutex);

        // Wait for a job to exist in the queue
        sem_wait(&full_slots);
        
        // Lock Queue to remove job
        pthread_mutex_lock(&queue_mutex);
        PCB current_job = ready_queue[queue_front];
        queue_front = (queue_front + 1) % QUEUE_CAPACITY;
        jobs_consumed++;
        pthread_mutex_unlock(&queue_mutex);
        
        // Signal that an empty slot is available
        sem_post(&empty_slots);

        // --- CONTEXT SWITCH OVERHEAD ---
        printf("\n[CORE %d] Context Switching to %s... (Saving Registers)\n", id, current_job.job_id);
        usleep(500000); // 0.5 sec context switch penalty

        // --- EXECUTION ---
        current_job.state = STATE_RUNNING;
        printf("[CORE %d] Executing %s [State: %s]\n", id, current_job.job_id, getStateName(current_job.state));
        
        // Simulate System Call
        sim_request_resource(&current_job, 50); 
        
        sleep(current_job.burst_time); // Simulate CPU processing
        
        current_job.state = STATE_TERMINATED;
        printf("[CORE %d] Finished %s [State: %s]\n", id, current_job.job_id, getStateName(current_job.state));
    }
    return NULL;
}

// --- MAIN FUNCTION ---
int main() {
    printf("==================================================\n");
    printf(" ECO-CLOUD OS KERNEL: POSIX MULTITHREADING TEST \n");
    printf("==================================================\n\n");

    // Initialize Mutexes and Semaphores
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_mutex_init(&energy_mutex, NULL);
    sem_init(&empty_slots, 0, QUEUE_CAPACITY); // Initially, all slots are empty
    sem_init(&full_slots, 0, 0);               // Initially, 0 slots are full

    // Create Threads
    pthread_t producer_thread;
    pthread_t cpu_thread_1, cpu_thread_2;
    int core_1 = 1, core_2 = 2;

    pthread_create(&producer_thread, NULL, job_generator, NULL);
    pthread_create(&cpu_thread_1, NULL, cpu_core, &core_1);
    pthread_create(&cpu_thread_2, NULL, cpu_core, &core_2);

    // Wait for threads to finish
    pthread_join(producer_thread, NULL);
    pthread_join(cpu_thread_1, NULL);
    pthread_join(cpu_thread_2, NULL);

    // Cleanup
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&energy_mutex);
    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);

    printf("\n==================================================\n");
    printf(" SYSTEM SHUTDOWN: All jobs processed.\n");
    printf("==================================================\n");

    return 0;
}