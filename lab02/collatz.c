#include "collatz.h"

int collatz_conjecture(int input) {
    if (input % 2 == 0) 
        return input / 2;
    return 3 * input + 1;
}

int test_collatz_convergence(int input, int max_iter, int *steps) {
    int count = 0;
    while (input != 1 && count < max_iter) {
        steps[count++] = input;
        input = collatz_conjecture(input);
    }
    if (input == 1) {
        steps[count++] = 1;
        return count;
    }
    return 0;
}