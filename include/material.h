#ifndef MATERIAL_H
#define MATERIAL_H

namespace PathTrace
{

struct Material
{
    Color transmit;
    double ior;
    Color reflect;
    double scatter_coefficient;
    Color emissive;
};

}

#endif // MATERIAL_H
