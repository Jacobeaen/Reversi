#ifndef STACK_H
#define STACK_H

typedef struct {
    int cell_x[60];
    int cell_y[60];
    int size;
} moves;

typedef struct Stack {
    struct Stack *next;
    moves data;
} Stack;

Stack* push(Stack *top, moves *data);
Stack* pop(Stack *top, moves *data);
Stack* clear_stack(Stack *stack);

#endif