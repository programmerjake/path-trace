#include "intersection.h"

namespace PathTrace
{

Intersection::Intersection(Object * a, Object * b)
    : a(a), b(b)
{
}

Intersection::~Intersection()
{
    delete a;
    delete b;
}

namespace
{

class IntersectionSpanIterator : public SpanIterator
{
public:
    void nextA()
    {
        assert(!aEnded);
        if(!spanIteratorA)
        {
            aEnded = true;
        }
        else
        {
            spanA = *spanIteratorA;
            spanIteratorA++;
        }
    }

    void nextB()
    {
        assert(!bEnded);
        if(!spanIteratorB)
        {
            bEnded = true;
        }
        else
        {
            spanB = *spanIteratorB;
            spanIteratorB++;
        }
    }

    IntersectionSpanIterator(SpanIterator * spanIteratorA, SpanIterator * spanIteratorB)
        : spanIteratorA(*spanIteratorA), spanIteratorB(*spanIteratorB)
    {
        ended = true;
    }

    virtual void init(const Ray & ray)
    {
        spanIteratorA.init(ray);
        spanIteratorB.init(ray);
        ended = false;
        aEnded = false;
        bEnded = false;
        nextA();
        nextB();
        next();
    }

    const Span & operator *() const
    {
        return resultSpan;
    }

    const Span * operator ->() const
    {
        return &resultSpan;
    }

    bool isAtEnd() const
    {
        return ended;
    }

    void next()
    {
        while(true)
        {
            if(aEnded || bEnded)
            {
                ended = true;
                return;
            }
            if(spanA.end < spanB.start) // if a is completely before b
            {
                nextA();
                continue;
            }
            if(spanB.end < spanA.start) // if b is completely before a
            {
                nextB();
                continue;
            }
            if(spanA.start < spanB.start)
            {
                if(spanA.end < spanB.end)
                {
                    spanA.copyStartFromStart(spanB);
                    resultSpan = spanA;
                    nextA();
                    return;
                }
                resultSpan = spanB;
                nextB();
                return;
            }
            else
            {
                if(spanB.end < spanA.end)
                {
                    spanB.copyStartFromStart(spanA);
                    resultSpan = spanB;
                    nextB();
                    return;
                }
                resultSpan = spanA;
                nextA();
                return;
            }
        }
    }

    virtual ~IntersectionSpanIterator()
    {
        delete &spanIteratorA;
        delete &spanIteratorB;
    }

private:
    SpanIterator & spanIteratorA;
    SpanIterator & spanIteratorB;
    Span spanA, spanB, resultSpan;
    bool ended, aEnded, bEnded;
};

}

SpanIterator * Intersection::makeSpanIterator() const
{
    return new IntersectionSpanIterator(a->makeSpanIterator(), b->makeSpanIterator());
}

}
