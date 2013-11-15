#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

namespace PathTrace
{

const float eps = 1e-3;
const float max_value = 1e20;

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
