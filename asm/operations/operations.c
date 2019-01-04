#include <stdio.h>

float do_oper(float left, float right, char oper)
{
    switch (oper)
    {
    case '+':
        return left + right;
    case '-':
        return left - right;
    case '*':
        return left * right;
    case '/':
        return left / right;
    default:
        return 0.0f / 0.0f;
    }
}

int main()
{
    float left, right;
    char oper;
    scanf("%f %c %f", &left, &oper, &right);
    float result = do_oper(left, right, oper);
    printf("result = %f\n", result);
}