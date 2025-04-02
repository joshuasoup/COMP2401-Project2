#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

void load_data(Manager *manager);

int main(void)
{
    Manager manager;
    manager_init(&manager);
    load_data(&manager);

    // Create thread IDs
    pthread_t manager_tid;
    pthread_t *system_tids = malloc(sizeof(pthread_t) * manager.system_array.size);

    if (!system_tids)
    {
        perror("Failed to allocate memory for thread IDs");
        return 1;
    }

    // Start manager thread
    if (pthread_create(&manager_tid, NULL, manager_thread, &manager) != 0)
    {
        perror("Failed to create manager thread");
        free(system_tids);
        return 1;
    }

    // Start system threads
    for (int i = 0; i < manager.system_array.size; ++i)
    {
        if (pthread_create(&system_tids[i], NULL, system_thread, manager.system_array.systems[i]) != 0)
        {
            perror("Failed to create system thread");
            // In a real application, we would need to handle this failure better
        }
    }

    // Wait for manager thread to complete
    pthread_join(manager_tid, NULL);

    // Wait for all system threads to complete
    for (int i = 0; i < manager.system_array.size; ++i)
    {
        pthread_join(system_tids[i], NULL);
    }

    // Free the thread ID array
    free(system_tids);

    manager_clean(&manager);
    return 0;
}

/**
 * Loads sample data for the simulation.
 *
 * Calls all of the functions required to create resources and systems and add them to the Manager's data.
 *
 * @param[in,out] manager  Pointer to the `Manager` to populate with resource and system data.
 */
void load_data(Manager *manager)
{
    // Create resources
    Resource *fuel, *oxygen, *energy, *distance;
    resource_create(&fuel, "Fuel", 1000, 1000);
    resource_create(&oxygen, "Oxygen", 20, 50);
    resource_create(&energy, "Energy", 30, 50);
    resource_create(&distance, "Distance", 0, 1000);

    resource_array_add(&manager->resource_array, fuel);
    resource_array_add(&manager->resource_array, oxygen);
    resource_array_add(&manager->resource_array, energy);
    resource_array_add(&manager->resource_array, distance);

    // Create systems
    System *propulsion_system, *life_support_system, *crew_capsule_system, *generator_system;
    ResourceAmount consume_fuel, produce_distance;
    resource_amount_init(&consume_fuel, fuel, 5);
    resource_amount_init(&produce_distance, distance, 25);
    system_create(&propulsion_system, "Propulsion", consume_fuel, produce_distance, 50, &manager->event_queue);

    ResourceAmount consume_energy, produce_oxygen;
    resource_amount_init(&consume_energy, energy, 7);
    resource_amount_init(&produce_oxygen, oxygen, 4);
    system_create(&life_support_system, "Life Support", consume_energy, produce_oxygen, 10, &manager->event_queue);

    ResourceAmount consume_oxygen, produce_nothing;
    resource_amount_init(&consume_oxygen, oxygen, 1);
    resource_amount_init(&produce_nothing, NULL, 0);
    system_create(&crew_capsule_system, "Crew", consume_oxygen, produce_nothing, 2, &manager->event_queue);

    ResourceAmount consume_fuel_for_energy, produce_energy;
    resource_amount_init(&consume_fuel_for_energy, fuel, 5);
    resource_amount_init(&produce_energy, energy, 10);
    system_create(&generator_system, "Generator", consume_fuel_for_energy, produce_energy, 20, &manager->event_queue);

    system_array_add(&manager->system_array, propulsion_system);
    system_array_add(&manager->system_array, life_support_system);
    system_array_add(&manager->system_array, crew_capsule_system);
    system_array_add(&manager->system_array, generator_system);
}