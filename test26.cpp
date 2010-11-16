/*
 *	investigate scheme to name template parameters so that order and number does not matter
 */
#include "objrdf.h"
using namespace objrdf;

template<typename T> struct prop{};
template<const char*> struct name{};
template<const char*> struct ns{};
template<const char*> struct com{};

PROPERTY(p,int);
char jm[]="jm";
char vero[]="vero";




char EMPTY[]="";

template<const char* T> struct tmp{
	static char* STR;
};
template<const char* T> const char tmp<T>::STR=T;


template<typename T> struct get_name{static const char* STR;};
template<typename T> const char* get_name<T>::STR=EMPTY;
template<const char* C> struct get_name<name<C> >{static const char* STR;};
template<const char* C> const char* get_name<name<C> >::STR=C;
template<typename A,typename B> struct get_name<duo<A,B> >{static const char* STR;};
template<typename A,typename B> const char* get_name<duo<A,B> >::STR=(get_name<A>::STR==EMPTY) ? get_name<B>::STR : get_name<A>::STR;

template<typename T> struct get_ns{static const char* STR;};
template<typename T> const char* get_ns<T>::STR=EMPTY;
template<const char* C> struct get_ns<ns<C> >{static const char* STR;};
template<const char* C> const char* get_ns<ns<C> >::STR=C;
template<typename A,typename B> struct get_ns<duo<A,B> >{static const char* STR;};
template<typename A,typename B> const char* get_ns<duo<A,B> >::STR=(get_ns<A>::STR==EMPTY) ? get_ns<B>::STR : get_ns<A>::STR;

template<typename T> struct get_com{static const char* STR;};
template<typename T> const char* get_com<T>::STR=EMPTY;
template<const char* C> struct get_com<com<C> >{static const char* STR;};
template<const char* C> const char* get_com<com<C> >::STR=C;
template<typename A,typename B> struct get_com<duo<A,B> >{static const char* STR;};
template<typename A,typename B> const char* get_com<duo<A,B> >::STR=(get_com<A>::STR==EMPTY) ? get_com<B>::STR : get_com<A>::STR;

template<typename T> struct get_prop{typedef NIL VALUE;};
template<typename T> struct get_prop<prop<T> >{typedef T VALUE;};
template<typename A,typename B> struct get_prop<duo<A,B> >{
	typedef typename IfThenElse<equality<typename get_prop<A>::VALUE,NIL>::VALUE,typename get_prop<B>::VALUE,typename get_prop<A>::VALUE>::ResultT VALUE;
};

template<
	typename T_0,
	typename T_1=NIL,
	typename T_2=NIL,
	typename T_3=NIL,
	typename TMP=typename tuple<T_0,T_1,T_2,T_3>::SELF
> struct r{//:resource<get_ns<TMP>::STR,get_name<TMP>::STR,get_prop<TMP> >{};
	static void test(){cout<<"namespace: "<<get_ns<TMP>::STR<<"\tname:"<<get_name<TMP>::STR<<endl;}

};

typedef r<
	/*ns<jm>,*/
	name<vero>,
	name<jm>,
	prop<p> 
> JM;
		
	

int main(){
	JM::test();
	typedef name<tmp<jm>::STR> k;
}
