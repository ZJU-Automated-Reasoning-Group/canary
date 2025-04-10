#ifndef STRING_WRAPPER_H_
#define STRING_WRAPPER_H_
#include <string>
#include <iostream>
#include "common.h"

class StringWrapper
{
    friend std::ostream& operator<<( std::ostream&,const StringWrapper& );
    public:
        StringWrapper( const std::string&,unsigned int c=0 );
        StringWrapper( const char* ,unsigned int c=0 );
        ~StringWrapper() {
            //std::cerr << "~SW{ " << str << ",cnt=" << count << " }\n";
        }

        static StringWrapper* WPDS_CALL one();
        static StringWrapper* WPDS_CALL zero();

        StringWrapper *extend( StringWrapper * );
        StringWrapper *combine( StringWrapper * );
        StringWrapper *quasiOne();
        bool equal( StringWrapper * );
        std::ostream& print( std::ostream& o) const;

        std::string longest_prefix( StringWrapper * );
    private:
        std::string str;
        static StringWrapper one_;
        static StringWrapper zero_;

    public:
        // For ref counting
        unsigned int count;
};
#endif  // STRING_WRAPPER_H_
