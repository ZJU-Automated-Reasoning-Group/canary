#include "WPDS.h"
#include "stringwrapper.h"

int do_tfa( bool output )
{
    wpds::wpds_key pp = str2key("pp");
    wpds::wpds_key q = str2key("q");
    wpds::wpds_key g = str2key("g");
    wpds::wpds_key gg = str2key("gg");
    wpds::wpds_key p = str2key("p");
    wpds::wpds_key accept = str2key("accept");
    wpds::wpds_key W = str2key("W");
    wpds::wpds_key X = str2key("X");
    wpds::wpds_key Y = str2key("Y");
    wpds::wpds_key Z = str2key("Z");
    wpds::wpds_key lambda = str2key("lambda");

    wpds::Semiring< StringWrapper > s(StringWrapper::one());
    wpds::CA< StringWrapper > ca(s);
    wpds::WPDS< StringWrapper > wpds(s);
#if 1
    wpds::wpds_key program = str2key("program");
#define A 1
#define B A+1
#define C B+1
#define D C+1
#define E D+1
    int h,j,k,l,m,n;
    char buf[20];
    memset(buf,0,sizeof(buf));
    buf[0] = 'a';
    wpds::wpds_key prev = g,n1 = gg,n2;
    for(h=0;h<10;h++) {
        buf[A] = '0'+h;
        for(j=0;j<10;j++) {
            buf[B] = '0'+j;
            for(k=0;k<10;k++) {
                buf[C] = '0'+k;
                for(l=0;l<10;l++) {
                    buf[D] = '0'+l;
                    for(m=0;m<10;m++) {
                        buf[E] = '0'+m;
                        for(n=0;n<10;n++) {
                            buf[E+1] = '0'+n;
                            //printf("Created string %s\n",buf);
                            //n2 = str2key(buf);
                            n2 = new_str2key(buf);
                            if( k > 7 ) {
                                wpds.add_rule(program,prev,program,n1,n2,StringWrapper::one());
                            }
                            else if( k > 4 ) {
                                wpds.add_rule(program,prev,program,n1,StringWrapper::one());
                            }
                            else {
                                wpds.add_rule(program,prev,program,StringWrapper::one());
                            }
                            prev = n1;
                            n1 = n2;
                        }
                    }
                }
            }
        }
    }
#endif

#ifdef DBGWPDS
    std::cout << wpds << std::endl;
#endif // DBGWPDS
    wpds.add_rule(p,lambda,p,W,new StringWrapper("w"));
    wpds.add_rule(p,W,p,X,new StringWrapper("x"));
    wpds.add_rule(p,X,p,Y,new StringWrapper("y"));
    wpds.add_rule(p,Y,p,Z,new StringWrapper("z"));
    wpds.add_rule(p,W,p,W,X,new StringWrapper("u"));
    // This has nothing to do w/ the above
    wpds.add_rule(p,g,accept,new StringWrapper("BYE"));
    wpds.add_rule(p,g,accept,new StringWrapper("ADIOS"));

    //std::cout << wpds << std::endl;

    ca.add(p,g,accept,new StringWrapper("F"));
    ca.add(pp,g,accept,new StringWrapper("F1"));
    ca.add(q,gg,accept,new StringWrapper("F2"));
    ca.add(p,g,q,new StringWrapper("F3"));

    if( output ) {
        std::cout << "\n--- Searching for (p,g,accept) --- \n";
        wpds::CA<StringWrapper>::catrans_t c = ca.find(p,g,accept);
        std::cout << c << std::endl;

        wpds::CA< StringWrapper >::TransListIterPair pr  = ca.match( p );
        std::cout << "\n--- Searching for (p,*,*) --- \n";
        int i = 0;
        wpds::CA< StringWrapper >::TransListIter iter = pr.first;
        for( ; iter != pr.second ; iter++ ) {
            std::cout << i++ << ")\t" << *iter << std::endl;
        }
        std::cout << "********************* F **********************\n";
        std::cout << ca << std::endl;
        wpds::CA<StringWrapper> ca2 = ca;
        std::cout << ca2 << std::endl;

    }
    clear_dict();

    return 0;
}

int main()
{
    clear_dict();
    do_tfa( false );
    clear_dict();
    do_tfa( true );
    clear_dict();
    return 0;
}
