#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

namespace PathTrace
{

const double eps = 1e-12;
const double max_value = 1e100;

template <typename T>
class AutoDestruct
{
private:
    T * value;
public:
    AutoDestruct(T * value)
        : value(value)
    {
    }
    ~AutoDestruct()
    {
        delete value;
    }
};

}

#endif // MISC_H_INCLUDED
