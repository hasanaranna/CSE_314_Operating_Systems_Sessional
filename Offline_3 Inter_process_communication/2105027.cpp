#include <chrono>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <random>
#include <unistd.h>
#include <vector>
#include <semaphore.h>

using namespace std;

#define SLEEP_MULTIPLIER 1000 // Multiplier to convert seconds to milliseconds

int N; // Number of operatives
int M; // Unit Size
int x; // Document recreation relative time unit
int y; // Logbook entry relative time unit
int total_completed = 0; 

pthread_mutex_t output_lock; // Mutex lock for output to file to avoid interleaving
pthread_mutex_t typewriting_stations[4]; // Array of mutex locks for typewriting stations
pthread_cond_t station_available[4]; // Condition variable for typewriting stations

sem_t logbook_sem; // Semaphore for logbook access
sem_t reader_sem; // Semaphore for reader access

sem_t *unit_sem; // Semaphore for work completion of operatives in a unit

int reader_count = 0; // Counter for readers

bool station_busy[4] = {false, false, false, false}; // Array to track if typewriting stations are busy
// Timing functions
auto start_time = std::chrono::high_resolution_clock::now();

/**
 * Class representing a operative in the simulation.
 */
class Operative {
public:
  int id;              // Unique ID for each operative
  int unit;

  /**
   * Constructor to initialize a operative with a unique ID
   * @param id Operative's ID.
   */
  Operative(int id) {
    this->id = id;
    this->unit = (id - 1) / M + 1; // Calculate unit based on ID
  }

};

vector<Operative> operatives; // Vector to store all operatives
vector<Operative> leaders; // Vector to store leaders

/**
 * Initialize operatives and set the start time for the simulation.
 */
void initialize() {
    unit_sem = new sem_t[N / M]; // Create semaphore for each unit
    for (int i = 0; i < N / M; i++) {
        sem_init(&unit_sem[i], 0, 0); // Initialize each semaphore to 0
    }
    // Initialize semaphores for logbook and reader access
    sem_init(&logbook_sem, 0, 1); // Logbook semaphore initialized
    sem_init(&reader_sem, 0, 1); // Reader semaphore initialized

    for (int i = 1; i <= N; i++) {
        operatives.emplace_back(Operative(i));
        if(i % M == 0) {
            leaders.emplace_back(Operative(i)); 
        }   
    }

    // Initialize mutex lock for output to file
    pthread_mutex_init(&output_lock, NULL);
    // Initialize mutex locks for typewriting stations
    for(int i = 0; i < 4; i++) {
        pthread_mutex_init(&typewriting_stations[i], NULL);
        pthread_cond_init(&station_available[i], NULL);
    }

    start_time = chrono::high_resolution_clock::now(); // Reset start time
}

// Function to generate a Poisson-distributed random number
int get_random_number() {
  std::random_device rd;
  std::mt19937 generator(rd());

  // Lambda value for the Poisson distribution
  double lambda = 10000.234;
  std::poisson_distribution<int> poissonDist(lambda);
  return poissonDist(generator);
}

/**
 * Get the elapsed time in milliseconds since the start of the simulation.
 * @return The elapsed time in milliseconds.
 */
long long get_time() {
  auto end_time = chrono::high_resolution_clock::now();
  auto duration = chrono::duration_cast<chrono::milliseconds>(
      end_time - start_time);
  long long elapsed_time_ms = duration.count();
  return elapsed_time_ms;
}

// uses mutex lock to write output to avoid interleaving
void write_output(string output) {
  pthread_mutex_lock(&output_lock);
  cout << output;
  pthread_mutex_unlock(&output_lock);
}

/**
 * Simulate the start of document recreation by an operative and log the time.
 * @param operative Pointer to a Operative object.
 */
void start_recreation(Operative *operative) {

  write_output("Operative " + to_string(operative->id) +
               " has arrived at the typewriting station at time " +
               to_string(get_time()) +  "\n");
}

/**
 * Thread function for operative activities.
 * Simulates the operative's document recreation.
 * @param arg Pointer to a Operative object.
 */
void *operative_activities(void *arg) {
    int random_delay = get_random_number() % 100 + 1; // Random delay for each operative
    usleep(random_delay * SLEEP_MULTIPLIER); // Simulate random delay

    Operative *operative = (Operative *)arg;

    int assigned_typewriting_station = (operative->id) % 4 + 1;

    pthread_mutex_lock(&typewriting_stations[assigned_typewriting_station - 1]);
    while(station_busy[assigned_typewriting_station - 1]) {
        pthread_cond_wait(&station_available[assigned_typewriting_station - 1], 
                          &typewriting_stations[assigned_typewriting_station - 1]);
    }
    station_busy[assigned_typewriting_station - 1] = true; 
    
    start_recreation(operative); // Operative starts document recreation 
    usleep(x * SLEEP_MULTIPLIER); // Simulate document recreation time

    write_output("Operative " + to_string(operative->id) +
                 " has completed document recreation at time " + to_string(get_time()) + "\n");
    
    station_busy[assigned_typewriting_station - 1] = false;
    pthread_cond_broadcast(&station_available[assigned_typewriting_station - 1]); // Wake up all waiting operatives
    pthread_mutex_unlock(&typewriting_stations[assigned_typewriting_station - 1]);

    sem_post(&unit_sem[operative->unit - 1]); 

    return NULL;
}

/**
 * Thread function for leader activities.
 * Simulates the leader's logbook entry and coordination of operatives.
 * @param arg Pointer to a Operative object (leader).
 */
void *leader_activities(void *arg) {
    Operative *leader = (Operative *)arg;

    for(int i = 0; i < M; i++) {
        sem_wait(&unit_sem[leader->unit -1]); // Wait for all operatives in the unit to finish
    }

    write_output("Unit " + to_string(leader->unit) +
                 " has completed document recreation phase at time " + to_string(get_time()) + "\n");
    
    // Logbook entry by the leader
    
    sem_wait(&logbook_sem); // Wait for access to the logbook
    
    usleep(y * SLEEP_MULTIPLIER); // Simulate logbook entry
    total_completed++; // Increment the total completed operations
    write_output("Unit " + to_string(leader->unit) +
                    " has completed intelligence distribution at time " + to_string(get_time()) + "\n");

    sem_post(&logbook_sem); // Release access to the logbook
    

    return NULL;
    
}

/**
 * Thread function for intelligence staff activities.
 * Simulates the intelligence staff's review of logbook entries.
 * @param arg Pointer to an integer representing the staff ID.
 */
void *intelligence_staff_activities(void *arg) {
    int staff_id = *((int *)arg);
    // write_output("Intelligence Staff " + to_string(staff_id) +
    //              " has arrived at the logbook at time " + to_string(get_time()) + "\n");
    while (true) {
        int random_delay = get_random_number() % 2 +2 ; // Random delay for each intelligence staff
        usleep(random_delay * SLEEP_MULTIPLIER); // Simulate random delay
        sem_wait(&reader_sem);
        reader_count++;
        if (reader_count == 1) {
            sem_wait(&logbook_sem); // Lock the logbook for reading
        }
        sem_post(&reader_sem); // Release access to the reader semaphore
        write_output("Intelligence Staff " + to_string(staff_id) +
                     " began reviewing logbook at time " + to_string(get_time()) + ". Operations completed = " + 
                     to_string(total_completed) + "\n");
        usleep(10 * SLEEP_MULTIPLIER); // Simulate reading time
        sem_wait(&reader_sem);
        reader_count--;
        if (reader_count == 0) {
            sem_post(&logbook_sem); // Unlock the logbook after reading
        }
        sem_post(&reader_sem); // Release access to the reader semaphore
    }
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Usage: ./a.out <input_file> <output_file>" << std::endl;
        return 0;
    }

    // File handling for input and output redirection
    ifstream inputFile(argv[1]);
    streambuf *cinBuffer = cin.rdbuf(); // Save original cin buffer
    cin.rdbuf(inputFile.rdbuf()); // Redirect cin to input file

    ofstream outputFile(argv[2]);
    streambuf *coutBuffer = cout.rdbuf(); // Save original cout buffer
    cout.rdbuf(outputFile.rdbuf()); // Redirect cout to output file

    // Read input values
    cin >> N >> M;
    cin >> x >> y;

    pthread_t operative_threads[N]; // Array to hold student threads
    pthread_t leader_threads[N / M]; // Array to hold leader threads
    pthread_t intelligence_staff_threads[2]; // Array to hold intelligence staff threads

    int intelligence_staff_ids[2] = {1, 2}; // IDs for intelligence staff threads

    initialize(); // Initialize operatives and mutex lock

    for (int i = 0; i < N; i++) {
        pthread_create(&operative_threads[i], NULL, operative_activities,
                        &operatives[i]);
    }

    for(int i = 0; i < N / M; i++) {
        pthread_create(&leader_threads[i], NULL, leader_activities,
                        &leaders[i]);
    }

    for(int i = 0; i < 2; i++) {
        pthread_create(&intelligence_staff_threads[i], NULL, intelligence_staff_activities,
                        &intelligence_staff_ids[i]);
    }

    // Wait for all operative threads to finish
    for (int i = 0; i < N; i++) {
        pthread_join(operative_threads[i], NULL);
    }

    // Wait for all leader threads to finish
    for (int i = 0; i < N / M; i++) {
        pthread_join(leader_threads[i], NULL);
    }

    // cancel all intelligence staff threads
    for (int i = 0; i < 2; i++) {
        pthread_cancel(intelligence_staff_threads[i]);
    }

    // destroy semaphores and mutex locks
    for (int i = 0; i < N / M; i++) {
        sem_destroy(&unit_sem[i]);
    }
    delete[] unit_sem; // Free the dynamically allocated memory for unit_sem        
    sem_destroy(&logbook_sem); 
    sem_destroy(&reader_sem); 
    pthread_mutex_destroy(&output_lock); 
    for(int i = 0; i < 4; i++) {
        pthread_mutex_destroy(&typewriting_stations[i]); 
        pthread_cond_destroy(&station_available[i]); 
    }

    // Restore cin and cout to their original states (console)
    cin.rdbuf(cinBuffer);
    cout.rdbuf(coutBuffer);

    // close the input file
    inputFile.close();
    // Close the output file
    outputFile.close();

    return 0;

}