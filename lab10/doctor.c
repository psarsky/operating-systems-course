#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX_PATIENTS_IN_WAITING_ROOM 3
#define CONSULTATION_TIME_MIN 2
#define CONSULTATION_TIME_MAX 4
#define PATIENT_ARRIVAL_MIN 2
#define PATIENT_ARRIVAL_MAX 5
#define PATIENT_WALK_TIME 1

typedef struct {
    int waiting_patients;           // Number of patients in waiting room
    int patient_ids[MAX_PATIENTS_IN_WAITING_ROOM];  // IDs of patients in waiting room
    int doctor_sleeping;           // Whether doctor is sleeping
    int total_patients;           // Total number of patients
    int finished_patients;        // Number of patients who finished
    pthread_mutex_t mutex;        // Mutex for synchronization
    pthread_cond_t doctor_wake;   // Condition variable for waking doctor
    pthread_cond_t patient_done;  // Condition variable for finished patients
} Hospital;

Hospital hospital;

// Function returning current time in text format
char* get_current_time() {
    static char time_str[64];
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", 
            local->tm_hour, local->tm_min, local->tm_sec);
    return time_str;
}

// Function returning random time in given range
int random_time(int min, int max) {
    return min + rand() % (max - min + 1);
}

// Patient thread
void* patient_thread(void* arg) {
    int patient_id = *(int*)arg;
    free(arg);
    
    while (1) {
        // Random time before arriving at hospital
        int travel_time = random_time(PATIENT_ARRIVAL_MIN, PATIENT_ARRIVAL_MAX);
        printf("[%s] - Patient(%d): Going to hospital, will be there in %d s\n", 
                get_current_time(), patient_id, travel_time);
        sleep(travel_time);
        
        pthread_mutex_lock(&hospital.mutex);
        
        // Check if waiting room is full
        if (hospital.waiting_patients >= MAX_PATIENTS_IN_WAITING_ROOM) {
            pthread_mutex_unlock(&hospital.mutex);
            printf("[%s] - Patient(%d): too many patients, coming back later in %d s\n", 
                    get_current_time(), patient_id, PATIENT_WALK_TIME);
            sleep(PATIENT_WALK_TIME);
            continue;
        }
        
        // Enter waiting room
        hospital.patient_ids[hospital.waiting_patients] = patient_id;
        hospital.waiting_patients++;
        
        printf("[%s] - Patient(%d): %d patients waiting for doctor\n", 
                get_current_time(), patient_id, hospital.waiting_patients);
        
        // If I'm the third patient, wake up the doctor
        if (hospital.waiting_patients == MAX_PATIENTS_IN_WAITING_ROOM) {
            printf("[%s] - Patient(%d): waking up doctor\n", 
                    get_current_time(), patient_id);
            pthread_cond_signal(&hospital.doctor_wake);
        }
        
        // Wait for consultation - patient waits until served
        int consultation_started = 0;
        while (!consultation_started) {
            // Check if consultation started (waiting room emptied)
            if (hospital.waiting_patients == 0) {
                consultation_started = 1;
                break;
            }
            pthread_cond_wait(&hospital.patient_done, &hospital.mutex);
        }
        
        // Increment counter of finished patients
        hospital.finished_patients++;
        
        printf("[%s] - Patient(%d): finishing visit\n", 
                get_current_time(), patient_id);
        
        // If this is the last patient, wake up doctor so it can finish
        if (hospital.finished_patients >= hospital.total_patients) {
            pthread_cond_signal(&hospital.doctor_wake);
        }
        
        pthread_mutex_unlock(&hospital.mutex);
        
        // Patient finished - doesn't return to hospital
        break;
    }
    
    return NULL;
}

// Doctor thread
void* doctor_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&hospital.mutex);
        
        // Check termination condition - all patients finished
        if (hospital.finished_patients >= hospital.total_patients) {
            pthread_mutex_unlock(&hospital.mutex);
            break;
        }
        
        // Sleep until there are 3 patients
        hospital.doctor_sleeping = 1;
        printf("[%s] - Doctor: going to sleep\n", get_current_time());
        while (hospital.waiting_patients < MAX_PATIENTS_IN_WAITING_ROOM && 
                hospital.finished_patients < hospital.total_patients) {
            pthread_cond_wait(&hospital.doctor_wake, &hospital.mutex);
        }
        
        // Check termination condition again after waking up
        if (hospital.finished_patients >= hospital.total_patients) {
            pthread_mutex_unlock(&hospital.mutex);
            break;
        }
        
        hospital.doctor_sleeping = 0;
        printf("[%s] - Doctor: waking up\n", get_current_time());
        
        if (hospital.waiting_patients == MAX_PATIENTS_IN_WAITING_ROOM) {
            // Consult patients
            printf("[%s] - Doctor: consulting patients %d, %d, %d\n", 
                    get_current_time(), 
                    hospital.patient_ids[0], 
                    hospital.patient_ids[1], 
                    hospital.patient_ids[2]);
            
            // Empty waiting room
            hospital.waiting_patients = 0;
            
            pthread_mutex_unlock(&hospital.mutex);
            
            // Inform patients about consultation start
            pthread_cond_broadcast(&hospital.patient_done);
            
            // Consultation time
            int consultation_time = random_time(CONSULTATION_TIME_MIN, CONSULTATION_TIME_MAX);
            sleep(consultation_time);
            
            pthread_mutex_lock(&hospital.mutex);
            
            printf("[%s] - Doctor: going to sleep\n", get_current_time());
        }
        
        pthread_mutex_unlock(&hospital.mutex);
    }
    
    printf("[%s] - Doctor: finishing work - all patients have been served\n", 
            get_current_time());
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_patients>\n", argv[0]);
        return 1;
    }
    
    int num_patients = atoi(argv[1]);
    if (num_patients <= 0) {
        printf("Number of patients must be greater than 0\n");
        return 1;
    }

    srand(time(NULL));

    hospital.waiting_patients = 0;
    hospital.doctor_sleeping = 1;
    hospital.total_patients = num_patients;
    hospital.finished_patients = 0;
    
    pthread_mutex_init(&hospital.mutex, NULL);
    pthread_cond_init(&hospital.doctor_wake, NULL);
    pthread_cond_init(&hospital.patient_done, NULL);

    pthread_t doctor;
    pthread_t* patients = malloc(num_patients * sizeof(pthread_t));
    
    pthread_create(&doctor, NULL, doctor_thread, NULL);

    for (int i = 0; i < num_patients; i++) {
        int* patient_id = malloc(sizeof(int));
        *patient_id = i + 1;
        pthread_create(&patients[i], NULL, patient_thread, patient_id);
    }
    
    pthread_join(doctor, NULL);
    
    for (int i = 0; i < num_patients; i++) {
        pthread_join(patients[i], NULL);
    }
    
    pthread_mutex_destroy(&hospital.mutex);
    pthread_cond_destroy(&hospital.doctor_wake);
    pthread_cond_destroy(&hospital.patient_done);
    free(patients);
    
    printf("\nProgram finished - all patients have been served.\n");
    
    return 0;
}