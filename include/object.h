#ifndef OBJECT_H
#define OBJECT_H

#include "span.h"
#include "ray.h"

namespace PathTrace
{

class Object
{
public:
    Object();
    virtual SpanIterator * makeSpanIterator() const = 0;
    virtual Object * transform(const Matrix &m) const
    {
        return NULL;
    }
    virtual ~Object() {}
    virtual Object * duplicate() const = 0;
private:
    Object(const Object & rt); // not implemented
    const Object & operator =(const Object & rt); // not implemented
};

class TransformedObject : public Object
{
private:
    Matrix m;
    Object * o;
    class TransformedSpanIterator : public SpanIterator
    {
    private:
        Matrix m, inv;
        SpanIterator * iter;
        Span span;
        void calcSpan()
        {
            if(!isAtEnd())
            {
                span = PathTrace::transform(inv, **iter);
            }
        }
    public:
        TransformedSpanIterator(Matrix m, SpanIterator * iter)
            : m(m), inv(invert(m)), iter(iter)
        {
        }
        virtual const Span & operator *() const
        {
            return span;
        }
        virtual const Span * operator ->() const
        {
            return &span;
        }
        virtual bool isAtEnd() const
        {
            return iter->isAtEnd();
        }
        virtual void next()
        {
            iter->next();
            calcSpan();
        }
        virtual void init(const Ray & ray)
        {
            iter->init(PathTrace::transform(m, ray));
            calcSpan();
        }

        virtual ~TransformedSpanIterator()
        {
            delete iter;
        }
    };
public:
    TransformedObject(const Matrix &m, Object * o)
        : m(m), o(o)
    {
    }
    virtual SpanIterator * makeSpanIterator() const
    {
        return new TransformedSpanIterator(m, o->makeSpanIterator());
    }
    virtual Object * transform(const Matrix &m) const
    {
        return new TransformedObject(this->m.concat(m), o->duplicate());
    }
    virtual Object * duplicate() const
    {
        return new TransformedObject(m, o->duplicate());
    }
    virtual ~TransformedObject()
    {
        delete o;
    }
};

inline Object *transform(const Matrix &m, Object *o)
{
    Object *retval = o->transform(m);
    if(!retval)
        retval = new TransformedObject(m, o->duplicate());
    return retval;
}

}

#endif // OBJECT_H
