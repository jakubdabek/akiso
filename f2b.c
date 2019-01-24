#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
        exit(1);
    int precision = atoi(argv[2]);
    unsigned long long number = atoll(index(argv[1], '.') + 1);
    int len = strlen(index(argv[1], '.') + 1);
    unsigned long long limit = 1LL;
    for (int i = 0; i < len; i++)
        limit *= 10LL;

    printf("0.");
    for (int i = 0; i < precision; i++)
    {
        number <<= 1;
        if (number == 0)
            break;
        if (number >= limit)
        {
            printf("1");
            number -= limit;
        }
        else
        {
            printf("0");
        }
    }
    printf("\n");
}
