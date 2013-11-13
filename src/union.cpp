#include "union.h"

namespace PathTrace
{

Union::Union(Object * a, Object * b)
    : a(a), b(b)
{
}

Union::~Union()
{
    delete a;
    delete b;
}

namespace
{

class UnionSpanIterator : public SpanIterator
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

    UnionSpanIterator(SpanIterator * spanIteratorA, SpanIterator * spanIteratorB)
        : spanIteratorA(*spanIteratorA), spanIteratorB(*spanIteratorB), aEnded(false), bEnded(false), ended(false)
    {
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
            if(aEnded)
            {
                if(bEnded)
                {
                    ended = true;
                    return;
                }
                resultSpan = spanB;
                nextB();
                return;
            }
            if(bEnded)
            {
                resultSpan = spanA;
                nextA();
                return;
            }
            if(spanA.end < spanB.start) // if a is completely before b
            {
                resultSpan = spanA;
                nextA();
                return;
            }
            if(spanB.end < spanA.start) // if b is completely before a
            {
                resultSpan = spanB;
                nextB();
                return;
            }
            if(spanA.start < spanB.start)
            {
                if(spanA.end < spanB.end)
                {
                    spanA.copyEndFromEnd(spanB);
                }
                nextB();
            }
            else
            {
                if(spanA.end > spanB.end)
                {
                    spanB.copyEndFromEnd(spanA);
                }
                nextA();
            }
        }
    }

    virtual ~UnionSpanIterator()
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

SpanIterator * Union::makeSpanIterator(const Ray & ray) const
{
    return new UnionSpanIterator(a->makeSpanIterator(ray), b->makeSpanIterator(ray));
}

}
