#include <stdbool.h>
#include <stdio.h>

bool is_prime(int a)
{
    if (a < 2)
        return false;
    for (
        int i = 2;
        i < a;
        i++)
    {
        if (a % i == 0)
            return false;
    }
    return true;
}

int main()
{
    for (
        int i = 1;
        i < 10000;
        i++)
    {
        if (is_prime(i))
            printf("%d\n", i);
    }
}
