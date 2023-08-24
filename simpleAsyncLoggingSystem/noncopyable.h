#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

class noncopyable{
    public:
        noncopyable(const noncopyable &) = delete;
        void operator = (const noncopyable & ) = delete;
    protected:
        //  protected的目的是让继承的派生类能顺利构造对象，让用户无法构造该类的对象
        noncopyable() = default;
        ~noncopyable() = default;
};


#endif