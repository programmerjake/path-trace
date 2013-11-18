#include "difference.h"

namespace PathTrace
{

Difference::Difference(Object * a, Object * b)
    : a(a), b(b)
{
}

Difference::~Difference()
{
    delete a;
    delete b;
}

namespace
{

class DifferenceSpanIterator : public SpanIterator
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

    DifferenceSpanIterator(SpanIterator * spanIteratorA, SpanIterator * spanIteratorB)
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
            if(aEnded)
            {
                ended = true;
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
                nextB();
            }
            else if(spanA.start < spanB.start)
            {
                if(spanA.end < spanB.end)
                {
                    spanA.copyEndFromStart(spanB);
                    resultSpan = spanA;
                    nextA();
                    return;
                }
                resultSpan = spanA;
                resultSpan.copyEndFromStart(spanB);
                spanA.copyStartFromEnd(spanB);
                nextB();
                return;
            }
            else
            {
                if(spanA.end > spanB.end)
                {
                    spanA.copyEndFromStart(spanB);
                    nextB();
                    continue;
                }
                nextA();
            }
        }
    }

    virtual ~DifferenceSpanIterator()
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

SpanIterator * Difference::makeSpanIterator() const
{
    return new DifferenceSpanIterator(a->makeSpanIterator(), b->makeSpanIterator());
}

}
