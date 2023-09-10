#include "fib.h"

int fib(int n)
{
    if (n < 2) return 1;
    else return fib(n - 1) + fib(n - 2);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    if (n < 0)
    {
        fprintf(stderr, "Invalid input. Please enter a non-negative integer.\n");
    }
    else
    {
        int result = fib(n);
        printf("%d\n", result); // Print the result to standard output
    }

    return 0;
}
