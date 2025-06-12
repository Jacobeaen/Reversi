#include <stdio.h>
#include <stdlib.h>


typedef struct {
    int cell_x[60];
    int cell_y[60];
    int size;
} moves;

typedef struct Stack {
    struct Stack *next;
    moves data;
} Stack;


Stack* push(Stack *top, moves *data) {
    Stack *new_top = (Stack *)malloc(sizeof(Stack));

    if (new_top == NULL){
        printf("Can't allocate memory!");

        return NULL;
    }

    new_top->data = *data;
    new_top->next = top;

    return new_top;
}

Stack* pop(Stack *top, moves *data) {
    if (top == NULL){
        puts("You're at bottom of the stack");
        
        return NULL;
    }

    *data = top->data;
    
    Stack *ptr = top->next;
    free(top);

    return ptr;
}

Stack* clear_stack(Stack *stack){
    if (stack == NULL)
        return NULL;

    Stack *ptr = stack;
    while (ptr != NULL){
        Stack *next = ptr->next;
        
        free(ptr);
        ptr = next;
    }

    return ptr;
}