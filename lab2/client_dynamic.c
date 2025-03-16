#include "collatz.h"
#include <stdio.h>
#include <dlfcn.h>

#define MAX_ITER 1000

int main() {
    int numbers[] = {6, 11, 19, 27, 50};
    int num_tests = sizeof(numbers) / sizeof(int);
    int steps[MAX_ITER];

    printf("\nDynamic loading:\n");
    void *handle;
    int (*test_collatz_convergence)(int input, int max_iter, int *steps);

    handle = dlopen("./collatzlib_shared.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading library: %s\n", dlerror());
        return 1;
    }

    test_collatz_convergence = dlsym(handle, "test_collatz_convergence");
    if (!test_collatz_convergence) {
        fprintf(stderr, "Error finding function.\n");
        dlclose(handle);
        return 1;
    }

    for (int i = 0; i < num_tests; i++) {
        int num_steps = test_collatz_convergence(numbers[i], MAX_ITER, steps);
        if (num_steps) {
            printf("%d converges to 1 in %d steps: ", numbers[i], num_steps);
            for (int j = 0; j < num_steps; j++) {
                printf("%d ", steps[j]);
            }
            printf("\n");
        } else {
            printf("%d does not converge to 1 in %d iterations.\n", numbers[i], MAX_ITER);
        }
    }

    dlclose(handle);

    return 0;
}
