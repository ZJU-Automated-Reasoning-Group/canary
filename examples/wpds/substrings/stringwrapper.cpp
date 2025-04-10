#include "stringwrapper.h"

StringWrapper::StringWrapper( const std::string& s,unsigned int c )
    : str(s),count(c) {}
StringWrapper::StringWrapper( const char* s,unsigned int c)
    : str(std::string(s)),count(c) {}

StringWrapper *StringWrapper::one()
{
    return &one_;
}
StringWrapper *StringWrapper::zero()
{
    return &zero_;
}

StringWrapper *StringWrapper::extend( StringWrapper *sw )
{
    StringWrapper *r;
    if( equal(zero()) || sw->equal(zero()) )
        r = zero();
    else if( equal(one()) )
        r = sw;
    else if( sw->equal(one()) )
        r = this;
    else
        r = new StringWrapper( (str+sw->str) );
    return r;
}

StringWrapper *StringWrapper::combine( StringWrapper *sw )
{
    StringWrapper *r;
    if( equal(zero()) )
        r = sw;
    else if( sw->equal(zero()) )
        r = this;
    else if( equal(one()) || sw->equal(one()) )
        r = one();
    else
        r = new StringWrapper( longest_prefix(sw) );
    return r;
}

StringWrapper *StringWrapper::quasiOne()
{
    return one();
}

bool StringWrapper::equal( StringWrapper *sw )
{
    return (str == sw->str);
}

std::ostream& StringWrapper::print( std::ostream& o) const
{
    return (o << str);
}
std::string StringWrapper::longest_prefix( StringWrapper * sw )
{
    unsigned int i=0;
    std::string s;
    while((i < str.length())&&(i < sw->str.length())&&(sw->str[i]==str[i]))
        s += str[i++];
    return s;
}

StringWrapper StringWrapper::one_("ONE",1);
StringWrapper StringWrapper::zero_("",1);

std::ostream& operator<<( std::ostream& o,const StringWrapper& sw )
{
    return sw.print(o);
}
