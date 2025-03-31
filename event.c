#include "defs.h"
#include <stdlib.h>
#include <stdio.h>

/* Event functions */

/**
 * Initializes an `Event` structure.
 *
 * Sets up an `Event` with the provided system, resource, status, priority, and amount.
 *
 * @param[out] event     Pointer to the `Event` to initialize.
 * @param[in]  system    Pointer to the `System` that generated the event.
 * @param[in]  resource  Pointer to the `Resource` associated with the event.
 * @param[in]  status    Status code representing the event type.
 * @param[in]  priority  Priority level of the event.
 * @param[in]  amount    Amount related to the event (e.g., resource amount).
 */
void event_init(Event *event, System *system, Resource *resource, int status, int priority, int amount)
{
    event->system = system;
    event->resource = resource;
    event->status = status;
    event->priority = priority;
    event->amount = amount;
}

/* EventQueue functions */

/**
 * Initializes the `EventQueue`.
 *
 * Sets up the queue for use, initializing any necessary data (e.g., semaphores when threading).
 *
 * @param[out] queue  Pointer to the `EventQueue` to initialize.
 */
void event_queue_init(EventQueue *queue)
{
    queue->head = NULL;
    queue->size = 0;

    // Initialize the semaphore with an initial value of 1
    if (sem_init(&queue->mutex, 0, 1) != 0)
    {
        perror("Failed to initialize queue mutex");
    }
}

/**
 * Cleans up the `EventQueue`.
 *
 * Frees any memory and resources associated with the `EventQueue`.
 *
 * @param[in,out] queue  Pointer to the `EventQueue` to clean.
 */
void event_queue_clean(EventQueue *queue)
{
    if (queue == NULL)
        return;

    EventNode *current = queue->head;
    while (current != NULL)
    {
        EventNode *temp = current;
        current = current->next;
        free(temp);
    }
    queue->head = NULL;
    queue->size = 0;

    // Destroy the semaphore
    sem_destroy(&queue->mutex);
}

/**
 * Pushes an `Event` onto the `EventQueue`.
 *
 * Adds the event to the queue in a thread-safe manner, maintaining priority order (highest first).
 *
 * @param[in,out] queue  Pointer to the `EventQueue`.
 * @param[in]     event  Pointer to the `Event` to push onto the queue.
 */
void event_queue_push(EventQueue *queue, const Event *event)
{
    if (queue == NULL || event == NULL)
        return;

    // Wait for access to the queue
    sem_wait(&queue->mutex);

    // Allocate a new event node.
    EventNode *newNode = malloc(sizeof(EventNode));
    if (newNode == NULL)
    {
        perror("Failed to allocate memory for new event node");
        sem_post(&queue->mutex); // Release the lock before returning
        return;
    }
    newNode->event = *event; // Copy the event data (shallow copy)
    newNode->next = NULL;

    // If the queue is empty or the new event has higher priority than the head, insert at the beginning.
    if (queue->head == NULL || event->priority > queue->head->event.priority)
    {
        newNode->next = queue->head;
        queue->head = newNode;
        queue->size++;
        sem_post(&queue->mutex); // Release the lock
        return;
    }

    // Otherwise, traverse to find the correct insertion point.
    // We continue while the next node exists and has an equal or higher priority.
    // This ensures that for equal priority events, the older ones remain at the front.
    EventNode *current = queue->head;
    while (current->next != NULL && current->next->event.priority >= event->priority)
    {
        current = current->next;
    }
    newNode->next = current->next;
    current->next = newNode;
    queue->size++;

    sem_post(&queue->mutex); // Release the lock
    return;
}

/**
 * Pops an `Event` from the `EventQueue`.
 *
 * Removes the highest priority event from the queue in a thread-safe manner.
 *
 * @param[in,out] queue  Pointer to the `EventQueue`.
 * @param[out]    event  Pointer to the `Event` structure to store the popped event.
 * @return               Non-zero if an event was successfully popped; zero otherwise.
 */
int event_queue_pop(EventQueue *queue, Event *event)
{
    if (queue == NULL || event == NULL)
        return STATUS_EMPTY;

    sem_wait(&queue->mutex); // Wait for access to the queue

    if (queue->head == NULL)
    {
        sem_post(&queue->mutex); // Release the lock before returning
        return STATUS_EMPTY;     // No event to pop
    }

    // Sets the event data as the one from the queue head
    *event = queue->head->event;

    // Frees the old head
    EventNode *old_head = queue->head;
    queue->head = queue->head->next;
    free(old_head);

    // Decrement queue size
    queue->size--;

    sem_post(&queue->mutex); // Release the lock

    return STATUS_OK;
}