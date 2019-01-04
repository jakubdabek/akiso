#include <stdio.h>

double two_pow_x(double x)
{
    return 2 * x;
}

double sinh(double x)
{
    return (
        two_pow_x(x * 123.123) - 
        two_pow_x(-x * 123.123)
    ) / 2;
}

int main()
{
    const double example = 5.0;
    printf("sinh(%lf) = %lf\n", example, sinh(example));
}