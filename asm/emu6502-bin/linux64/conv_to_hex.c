int main()
{
    char c = 0x4d;
    char out[10];
    char counter = 0;
    while (c > 0)
    {
        char a = c & 0x1;
        if (a == 0)
            out[counter] = '0';
        else
            out[counter] = '1';
        counter++;
    }

    return 0;
}