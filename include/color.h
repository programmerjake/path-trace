#ifndef COLOR_H_INCLUDED
#define COLOR_H_INCLUDED

namespace PathTrace
{

struct Color
{
    double r, g, b, a;
    static Color rgb(double r, double g, double b)
    {
        Color retval;
        retval.r = r;
        retval.g = g;
        retval.b = b;
        retval.a = 1;
        return retval;
    }
    static Color rgba(double r, double g, double b, double a)
    {
        Color retval;
        retval.r = r;
        retval.g = g;
        retval.b = b;
        retval.a = a;
        return retval;
    }
    const static Color Black;
    const static Color Clear;
    const static Color White;
    const static Color LightGray;
    const static Color DarkGray;
    const static Color Gray;
    const static Color Blue;
    const static Color Green;
    const static Color Yellow;
    const static Color Red;
    const static Color Magenta;
    const static Color Cyan;
    static Color gray(double v, double a = 1)
    {
        return rgba(v, v, v, a);
    }
    friend Color operator +(Color l, Color r)
    {
        Color retval = Black;
        retval.r = l.r * l.a + r.r * r.a;
        retval.g = l.g * l.a + r.g * r.a;
        retval.b = l.b * l.a + r.b * r.a;
        retval.a = 1 - (1 - l.a) * (1 - r.a);
        return retval;
    }
    Color operator -() const
    {
        return rgba(1 - r, 1 - g, 1 - b, a);
    }
    friend Color operator -(Color l, Color r)
    {
        return l + (-r);
    }
    friend Color operator *(double v, Color c)
    {
        return rgba(c.r * v, c.g * v, c.b * v, c.a);
    }
    friend Color operator *(Color c, double v)
    {
        return rgba(c.r * v, c.g * v, c.b * v, c.a);
    }
    friend Color compose(Color front, Color back)
    {
        Color retval = front * front.a;
        retval.r += back.r * (1 - front.a);
        retval.g += back.g * (1 - front.a);
        retval.b += back.b * (1 - front.a);
        retval.a = 1 - (1 - back.a) * (1 - front.a);
        return retval;
    }
};

}

#endif // COLOR_H_INCLUDED
