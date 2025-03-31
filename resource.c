#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Resource functions */

/**
 * Creates a new `Resource` object.
 *
 * Allocates memory for a new `Resource` and initializes its fields.
 * The `name` is dynamically allocated.
 *
 * @param[out] resource      Pointer to the `Resource*` to be allocated and initialized.
 * @param[in]  name          Name of the resource (the string is copied).
 * @param[in]  amount        Initial amount of the resource.
 * @param[in]  max_capacity  Maximum capacity of the resource.
 */
void resource_create(Resource **resource, const char *name, int amount, int max_capacity)
{
    // allocate the memory
    *resource = malloc(sizeof(Resource));
    if (*resource == NULL)
    {
        perror("Failed to allocate memory for Resource struct");
        return;
    }

    // allocate the memory for name and copy it
    (*resource)->name = malloc(strlen(name) + 1);
    if ((*resource)->name == NULL)
    {
        perror("Failed to allocate memory for name");
        free(*resource); //  for memory leak
        return;
    }
    strcpy((*resource)->name, name);

    (*resource)->amount = amount;
    (*resource)->max_capacity = max_capacity;

    // Initialize the semaphore with an initial value of 1
    if (sem_init(&(*resource)->mutex, 0, 1) != 0)
    {
        perror("Failed to initialize resource mutex");
        free((*resource)->name);
        free(*resource);
        *resource = NULL;
        return;
    }

    return;
}

/**
 * Destroys a `Resource` object.
 *
 * Frees all memory associated with the `Resource`.
 *
 * @param[in,out] resource  Pointer to the `Resource` to be destroyed.
 */
void resource_destroy(Resource *resource)
{
    if (resource != NULL)
    {
        // Free the dynamically allocated name field
        if (resource->name != NULL)
        {
            free(resource->name);
        }
        else
        {
            return;
        }

        // Destroy the semaphore
        sem_destroy(&resource->mutex);

        // Free the Resource structure itself
        free(resource);
    }
    else
    {
        return;
    }
}

/* ResourceAmount functions */

/**
 * Initializes a `ResourceAmount` structure.
 *
 * Associates a `Resource` with a specific `amount`.
 *
 * @param[out] resource_amount  Pointer to the `ResourceAmount` to initialize.
 * @param[in]  resource         Pointer to the `Resource`.
 * @param[in]  amount           The amount associated with the `Resource`.
 */
void resource_amount_init(ResourceAmount *resource_amount, Resource *resource, int amount)
{
    resource_amount->resource = resource;
    resource_amount->amount = amount;
}

/**
 * Initializes the `ResourceArray`.
 *
 * Allocates memory for the array of `Resource*` pointers and sets initial values.
 *
 * @param[out] array  Pointer to the `ResourceArray` to initialize.
 */
void resource_array_init(ResourceArray *array)
{
    // Dynamically allocate memory for the array of Resource* with capacity of 1
    array->resources = (Resource **)malloc(sizeof(Resource *) * 1); // Capacity 1
    if (array->resources == NULL)
    {
        // Handle memory allocation failure
        return;
    }

    // Initialize data required for the array fields
    array->capacity = 1; // Initial capacity is 1
    array->size = 0;
}

/**
 * Cleans up the `ResourceArray` by destroying all resources and freeing memory.
 *
 * Iterates through the array, calls `resource_destroy` on each `Resource`,
 * and frees the array memory.
 *
 * @param[in,out] array  Pointer to the `ResourceArray` to clean.
 */
void resource_array_clean(ResourceArray *array)
{
    if (array == NULL)
        return;
    for (int i = 0; i < array->size; i++)
    {
        resource_destroy(array->resources[i]);
    }
    free(array->resources);
    array->resources = NULL;
    array->size = 0;
    array->capacity = 0;
    return;
}

/**
 * Adds a `Resource` to the `ResourceArray`, resizing if necessary (doubling the size).
 *
 * Resizes the array when the capacity is reached and adds the new `Resource`.
 * Use of realloc is NOT permitted.
 *
 * @param[in,out] array     Pointer to the `ResourceArray`.
 * @param[in]     resource  Pointer to the `Resource` to add.
 */
void resource_array_add(ResourceArray *array, Resource *resource)
{
    // Checking to see if we need to resize
    if (array->size >= array->capacity)
    {
        // Resize the array (double the capacity)
        size_t new_capacity = array->capacity * 2;

        // allocate new memory for the resized array
        Resource **new_resources = (Resource **)malloc(sizeof(Resource *) * new_capacity);
        if (new_resources == NULL)
        {
            // Handle memory allocation failure
            return;
        }

        // copy existing elements to the new array
        for (int i = 0; i < array->size; i++)
        {
            new_resources[i] = array->resources[i];
        }

        // Free the old resources array since we no longer need it
        free(array->resources);

        // update the resources pointer to the new array and set the new capacity
        array->resources = new_resources;
        array->capacity = new_capacity;
    }

    // Add the new resource to the end of the array
    array->resources[array->size] = resource;
    // increase the size of the array to reflect the added resource
    array->size++;
}