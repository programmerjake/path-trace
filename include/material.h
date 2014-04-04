#ifndef MATERIAL_H
#define MATERIAL_H

#include "color.h"
#include "texture.h"

namespace PathTrace
{

struct Material
{
    Texture * reflect;
    Texture * scatter_coefficient; /// amount of scattering : (0, 1) to (specular, diffuse)
    Texture * emissive;
    Texture * transmit;
    float ior;
    Texture * transmit_reflect_coefficient; /// (0, 1) to (reflect, transmit)
    Material(Texture * reflect = new ColorTexture(1), Texture * scatter_coefficient = new ColorTexture(1), Texture * emissive = new ColorTexture(0), Texture * transmit = new ColorTexture(0), float ior = 1, Texture * transmit_reflect_coefficient = new ColorTexture(0))
        : reflect(reflect), scatter_coefficient(scatter_coefficient), emissive(emissive), transmit(transmit), ior(ior), transmit_reflect_coefficient(transmit_reflect_coefficient)
    {
    }
    ~Material()
    {
        delete reflect;
        delete scatter_coefficient;
        delete emissive;
        delete transmit;
        delete transmit_reflect_coefficient;
    }
    Material * duplicate() const
    {
        return new Material(reflect->duplicate(), scatter_coefficient->duplicate(), emissive->duplicate(), transmit->duplicate(), ior, transmit_reflect_coefficient->duplicate());
    }
private:
    Material(const Material & rt); // not implemented
    const Material & operator =(const Material & rt); // not implemented
};

inline Material * transform(const Matrix & m, const Material * mat)
{
    return new Material(transform(m, mat->reflect), transform(m, mat->scatter_coefficient), transform(m, mat->emissive), transform(m, mat->transmit), mat->ior, transform(m, mat->transmit_reflect_coefficient));
}

}

#endif // MATERIAL_H
