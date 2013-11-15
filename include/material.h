#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"

namespace PathTrace
{

struct Material
{
    Color reflect;
    float scatter_coefficient; /// amount of scattering : (0, 1) to (specular, diffuse)
    Color emissive;
    Color transmit;
    float ior;
    float transmit_reflect_coefficient; /// (0, 1) to (reflect, transmit)
    Material(Color reflect = Color(1, 1, 1), float scatter_coefficient = 1, Color emissive = Color(0, 0, 0), Color transmit = Color(0, 0, 0), float ior = 1, float transmit_reflect_coefficient = 0)
        : reflect(reflect), scatter_coefficient(scatter_coefficient), emissive(emissive), transmit(transmit), ior(ior), transmit_reflect_coefficient(transmit_reflect_coefficient)
    {
    }
};

}

#endif // MATERIAL_H
