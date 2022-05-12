/* 
 * Developed by R. E. Bryant, 2017
 * Extended to store strings, 2018
 */

/*
 * This program implements a queue supporting both FIFO and LIFO
 * operations.
 *
 * It uses a singly-linked list to represent the set of queue elements
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/*
  Create empty queue.
  Return NULL if could not allocate space.
*/
queue_t *q_new()
{
    queue_t *q =  malloc(sizeof(queue_t));
    if (q == NULL) {
      return NULL;
    } 
    else {
      q->head = NULL;
      q->tail = NULL;
      q->size = 0;
    return q;
    }
}

/* Free all storage used by queue */
void q_free(queue_t *q)
{
    if (q == NULL)
    {
      return;
    }
    for (int i = 0; i < q->size; i++)
    {
      list_ele_t *p = q->head;
      q->head = q->head->next;
      if (p->value != NULL)
      {
        free(p->value);
      }
      free(p);
    }
    free(q);
}

/*
  Attempt to insert element at head of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(queue_t *q, char*s)
{
    list_ele_t *newh;
    /* What should you do if the q is NULL? */
    if(!q)
      return false;

    newh = malloc(sizeof(list_ele_t));
    if(!newh)
      return false;
    /* Don't forget to allocate space for the string and copy it */
    /* What if either call to malloc returns NULL? */
    char *value;
    value = malloc(1 + strlen(s));

    if(!value)
    {
      if(newh)
          free(newh);
          // 释放所有占用的空间(好不容易找到是这里)
      return false;
    }
   
    strcpy(value, s);
    newh->value = value;
    // newh->tail = NULL;
    newh->next = q->head;
    if(q->head == NULL)
      q->tail = newh;
    q->head = newh;
    q->size += 1;
    return true;
}


/*
  Attempt to insert element at tail of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(queue_t *q, char *s)
{
    /* You need to write the complete code for this function */
    /* Remember: It should operate in O(1) time */

    if(!q)
      return false;
    list_ele_t *newt;

    newt = malloc(sizeof(list_ele_t));
    if(!newt)
      return false;
    char *value;
    value = malloc(strlen(s) + 1);

    if(!value)
    {
      if(newt)
          free(newt);
      return false;
    }

    strcpy(value, s);

    newt->value = value;
    newt->next = NULL;
    if(q->head == NULL)
      q->head = newt;
    q->tail->next = newt;
    q->tail = newt;
    q->size += 1;

    return true;
}

/*
  Attempt to remove element from head of queue.
  Return true if successful.
  Return false if queue is NULL or empty.
  If sp is non-NULL and an element is removed, copy the removed string to *sp
  (up to a maximum of bufsize-1 characters, plus a null terminator.)
  The space used by the list element and the string should be freed.
*/
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
    /* You need to fix up this code. */
    if(!q || !q->head)
      return false;
    if(sp)
    {
      if(strlen(q->head->value) < bufsize)
        strcpy(sp, q->head->value);
      else
      {
        int i;
        for(i = 0; i < bufsize - 1; i++)
          *(sp + i) = q->head->value[i];
        *(sp + bufsize - 1) = '\0';
      }
    }

    free(q->head->value);
    list_ele_t *tmp;
    tmp = q->head;
    q->head = q->head->next;
    free(tmp);
    q->size -= 1;
    return true;
}

/*
  Return number of elements in queue.
  Return 0 if q is NULL or empty
 */
int q_size(queue_t *q)
{
    /* You need to write the code for this function */
    /* Remember: It should operate in O(1) time */
    if(q != NULL)
      return q->size;
    else return 0;
}

/*
  Reverse elements in queue
  No effect if q is NULL or empty
  This function should not allocate or free any list elements
  (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
  It should rearrange the existing ones.
 */
void q_reverse(queue_t *q)
{
    /* You need to write the code for this function */
    if(!q || !q->head)
      return;
    if(q->size == 1)
      return;

    list_ele_t *start, *end, *next_start;

    start = q->head->next;
    end = q->head;
    q->head->next = NULL;

    while(true)
    {
      next_start = start->next;
      start->next = end;
      end = start;
      start = next_start;

      if(start == NULL)
      {
        q->tail = q->head;
        q->head = end;
        return;
      }  
    }
}

