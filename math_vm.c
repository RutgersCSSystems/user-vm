#include "my_vm.h"

double powM(double x, double y)
{
    // x ^ y
    double result = 1;
    double base = x;
    double exponent = y;

    for (exponent; exponent > 0; exponent--)
    {
        result = result * base;
    }

    return result;
}

unsigned long long logM(unsigned long long val)
{
    unsigned long long ans = 0;
    unsigned long long copyOfVal = val;

    while (copyOfVal != 1)
    {
        ans += 1;
        copyOfVal = copyOfVal / 2;
    }

    return ans;
}

double ceilM(double x)
{
    int inum = (int)x;

    if (x == (double)inum)
    {
        return inum;
    }

    return inum + 1;
}
