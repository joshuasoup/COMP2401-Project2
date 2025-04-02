#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Helper functions just used by this C file to clean up our code
// Using static means they can't get linked into other files

static int system_convert(System *);
static void system_simulate_process_time(System *);
static int system_store_resources(System *);

/**
 * Creates a new `System` object.
 *
 * Allocates memory for a new `System` and initializes its fields.
 * The `name` is dynamically allocated.
 *
 * @param[out] system          Pointer to the `System*` to be allocated and initialized.
 * @param[in]  name            Name of the system (the string is copied).
 * @param[in]  consumed        `ResourceAmount` representing the resource consumed.
 * @param[in]  produced        `ResourceAmount` representing the resource produced.
 * @param[in]  processing_time Processing time in milliseconds.
 * @param[in]  event_queue     Pointer to the `EventQueue` for event handling.
 */
void system_create(System **system, const char *name, ResourceAmount consumed, ResourceAmount produced, int processing_time, EventQueue *event_queue)
{
    *system = (System *)malloc(sizeof(System));
    if (*system == NULL)
    {
        perror("Failed to allocate memory for System struct");
        return;
    }

    (*system)->name = malloc(strlen(name) + 1);
    if ((*system)->name == NULL)
    {
        perror("Failed to allocate memory for name");
        return;
    }

    // copies the name string into allocated memory
    strcpy((*system)->name, name);

    (*system)->consumed = consumed;
    (*system)->produced = produced;
    (*system)->processing_time = processing_time;
    (*system)->event_queue = event_queue;
    (*system)->amount_stored = 0;
    (*system)->status = STANDARD;

    // Initializes the status mutex
    sem_init(&(*system)->status_mutex, 0, 1);
}

/**
 * Destroys a `System` object.
 *
 * Frees all memory associated with the `System`.
 *
 * @param[in,out] system  Pointer to the `System` to be destroyed.
 */
void system_destroy(System *system)
{
    if (system == NULL)
        return;

    // Destroy the status mutex before the memoery is freed
    sem_destroy(&system->status_mutex);

    free(system->name);  // Free the dynamically allocated name field
    system->name = NULL; // Set to NULL to avoid dangling pointer
    free(system);        // Free the System struct itself
    system = NULL;       // Set to NULL to avoid dangling pointer
    return;
}

/**
 * Runs the main loop for a `System`.
 *
 * This function manages the lifecycle of a system, including resource conversion,
 * processing time simulation, and resource storage. It generates events based on
 * the success or failure of these operations.
 *
 * @param[in,out] system  Pointer to the `System` to run.
 */
void system_run(System *system)
{
    Event event;
    int result_status;

    if (system->amount_stored == 0)
    {
        // Need to convert resources 
        result_status = system_convert(system);

        if (result_status != STATUS_OK)
        {
            // Lock before accessing resources for event creation
            Resource *res = system->consumed.resource;
            if (res != NULL)
            {
                sem_wait(&res->mutex);
                event_init(&event, system, res, result_status, PRIORITY_HIGH, res->amount);
                sem_post(&res->mutex);
            }
            else
            {
                event_init(&event, system, NULL, result_status, PRIORITY_HIGH, 0);
            }
            event_queue_push(system->event_queue, &event);
            // Sleep to prevent looping too frequently and spamming with events
            usleep(SYSTEM_WAIT_TIME * 5000);
        }
    }

    if (system->amount_stored > 0)
    {
        // Attempt to store the produced resources
        result_status = system_store_resources(system);

        if (result_status != STATUS_OK)
        {
            // locka before accessing resources for event creation
            Resource *res = system->produced.resource;
            if (res != NULL)
            {
                sem_wait(&res->mutex);
                event_init(&event, system, res, result_status, PRIORITY_LOW, res->amount);
                sem_post(&res->mutex);
            }
            else
            {
                event_init(&event, system, NULL, result_status, PRIORITY_LOW, 0);
            }
            event_queue_push(system->event_queue, &event);
            // Sleep to prevent looping too frequently and spamming with events
            usleep(SYSTEM_WAIT_TIME * 5000);
        }
    }
    usleep(SYSTEM_WAIT_TIME * 1000);
}

/**
 * Converts resources in a `System`.
 *
 * Handles the consumption of required resources and simulates processing time.
 * Updates the amount of produced resources based on the system's configuration.
 *
 * @param[in,out] system           Pointer to the `System` performing the conversion.
 * @param[out]    amount_produced  Pointer to the integer tracking the amount of produced resources.
 * @return                         `STATUS_OK` if successful, or an error status code.
 */
static int system_convert(System *system)
{
    int status;
    Resource *consumed_resource = system->consumed.resource;
    int amount_consumed = system->consumed.amount;

    // We can convert without consuming anything
    if (consumed_resource == NULL)
    {
        status = STATUS_OK;
    }
    else
    {
        // forces to wait for access to the resource
        sem_wait(&consumed_resource->mutex);

        // Attempt to consume the required resources
        if (consumed_resource->amount >= amount_consumed)
        {
            consumed_resource->amount -= amount_consumed;
            status = STATUS_OK;
        }
        else
        {
            status = (consumed_resource->amount == 0) ? STATUS_EMPTY : STATUS_INSUFFICIENT;
        }

        sem_post(&consumed_resource->mutex); // releases the Lock
    }
    // as long as the status passed we can start the process time
    if (status == STATUS_OK)
    {
        system_simulate_process_time(system);

        if (system->produced.resource != NULL)
        {
            system->amount_stored += system->produced.amount;
        }
        else
        {
            system->amount_stored = 0;
        }
    }

    return status;
}

/**
 * Simulates the processing time for a `System`.
 *
 * Adjusts the processing time based on the system's current status (e.g., SLOW, FAST)
 * and sleeps for the adjusted time to simulate processing.
 *
 * @param[in] system  Pointer to the `System` whose processing time is being simulated.
 */
static void system_simulate_process_time(System *system)
{
    int adjusted_processing_time;
    int current_status;

    // Get current status with mutex protection
    sem_wait(&system->status_mutex);
    current_status = system->status;
    sem_post(&system->status_mutex);

    // Adjust based on the current system status modifier
    switch (current_status)
    {
    case SLOW:
        adjusted_processing_time = system->processing_time * 2;
        break;
    case FAST:
        adjusted_processing_time = system->processing_time / 2;
        break;
    default:
        adjusted_processing_time = system->processing_time;
    }

    // Sleep
    usleep(adjusted_processing_time * 1000);
}

/**
 * Stores produced resources in a `System`.
 *
 * Attempts to add the produced resources to the corresponding resource's amount,
 * considering the maximum capacity. Updates the `produced_resource_count` to reflect
 * any leftover resources that couldn't be stored.
 *
 * @param[in,out] system                   Pointer to the `System` storing resources.
 * @param[in,out] produced_resource_count  Pointer to the integer value of how many resources need to be stored, updated with the amount that could not be stored.
 * @return                                 `STATUS_OK` if all resources were stored, or `STATUS_CAPACITY` if not all could be stored.
 */
static int system_store_resources(System *system)
{
    Resource *produced_resource = system->produced.resource;
    int available_space, amount_to_store;

    // We can always proceed if there's nothing to store
    if (produced_resource == NULL || system->amount_stored == 0)
    {
        system->amount_stored = 0;
        return STATUS_EMPTY;
    }

    amount_to_store = system->amount_stored;

    // wait for access to the resource
    sem_wait(&produced_resource->mutex);

    // find the available space
    available_space = produced_resource->max_capacity - produced_resource->amount;

    if (available_space >= amount_to_store)
    {
        // Store all produced resources
        produced_resource->amount += amount_to_store;
        system->amount_stored = 0;
    }
    else if (available_space > 0)
    {
        // Store as much as poassibel
        produced_resource->amount += available_space;
        system->amount_stored = amount_to_store - available_space;
    }

    sem_post(&produced_resource->mutex);

    if (system->amount_stored != 0)
    {
        return STATUS_CAPACITY;
    }

    return STATUS_OK;
}

/**
 * Initializes the `SystemArray`.
 *
 * Allocates memory for the array of `System*` pointers of capacity 1 and sets up initial values.
 *
 * @param[out] array  Pointer to the `SystemArray` to initialize.
 */
void system_array_init(SystemArray *array)
{
    // Dynamically allocate memory for the array of Resource* with capacity of 1
    array->systems = malloc(sizeof(System *) * 1); // Capacity 1
    if (array->systems == NULL)
        return;

    // initialize data required for the array fields
    array->capacity = 1; // Initial capacity is 1
    array->size = 0;
}

/**
 * Cleans up the `SystemArray` by destroying all systems and freeing memory.
 *
 * Iterates through the array, cleaning any memory for each System pointed to by the array.
 *
 * @param[in,out] array  Pointer to the `SystemArray` to clean.
 */
void system_array_clean(SystemArray *array)
{
    if (array == NULL)
        return;
    for (int i = 0; i < array->size; i++)
    {
        system_destroy(array->systems[i]);
    }
    free(array->systems);
    array->systems = NULL;
    array->size = 0;
    array->capacity = 0;
    return;
}

/**
 * Adds a `System` to the `SystemArray`, resizing if necessary (doubling the size).
 *
 * Resizes the array when the capacity is reached and adds the new `System`.
 * Use of realloc is NOT permitted.
 *
 * @param[in,out] array   Pointer to the `SystemArray`.
 * @param[in]     system  Pointer to the `System` to add.
 */
void system_array_add(SystemArray *array, System *system)
{
    // Checking to see if we need to resize
    if (array->size >= array->capacity)
    {
        // Resize the array (double the capacity)
        size_t new_capacity = array->capacity * 2;

        // allocate new memory for the resized array
        System **new_systems = malloc(sizeof(Resource *) * new_capacity);
        if (new_systems == NULL)
        {
            // Handle memory allocation failure
            return;
        }

        // copy existing elements to the new array
        for (int i = 0; i < array->size; ++i)
        {
            new_systems[i] = array->systems[i];
        }

        // Free the old systems array since we no longer need it
        free(array->systems);

        // update the systems pointer to the new array and set the new capacity
        array->systems = new_systems;
        array->capacity = new_capacity;
    }

    // Add the new resource to the end of the array
    array->systems[array->size] = system;
    // increase the size of the array to reflect the added system
    array->size++;
}

/**
 * Thread function for running a System.
 *
 * This function is passed to pthread_create and executes the System's
 * run function in a loop until the System's status is set to TERMINATE.
 *
 * @param system Pointer to the System to run (cast from void*)
 * @return Always returns NULL
 */
void *system_thread(void *arg)
{
    System *system = (System *)arg;
    int current_status;
    while (1)
    {
        // Check status with mutex protection
        sem_wait(&system->status_mutex);
        current_status = system->status; // SAVE the status while holding the mutex
        sem_post(&system->status_mutex);

        // Exit condition
        if (current_status == TERMINATE)
        {
            break;
        }

        system_run(system);
    }

    return NULL;
}