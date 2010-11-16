#ifndef EBNF_TEMPLATE_H
#define EBNF_TEMPLATE_H
/*
 *	is there anyway we could ignore the result of parsing?
 *
 */
#include "char_iterator.h"
struct nil;
struct true_p{static bool go(char_iterator& c){return true;}};
struct false_p{static bool go(char_iterator& c){return false;}};
template<char C> struct char_p{
	template<typename T> static bool go(T& c){//so we can augment the type for callbacks
		//std::cout<<"char_p<"<<C<<">"<<std::endl;
		//++c;
		//std::cout<<c.index<<endl;
		//c.char_iterator::operator++();
		c.increment();//there is ambiguity here !!!!!
		//std::cout<<c.index<<endl;
		bool r=((*c)==C);
		//std::cout<<"char_p<"<<C<<"> "<<*c<<std::endl;
		return r;
	}
};
//template<> struct char_p<'N'>{};
template<typename T> struct string_p{

};


template<char A,char Z> struct range_p{
	template<typename T> static bool go(T& c){
		++c;
		bool r=(A<=*c)&&(*c<=Z);
		return r;
	}
};
template<typename S,typename T>	struct seq_p{
	template<typename U> static bool go(U& c){
		return S::go(c)&&T::go(c);
	}
};
//template<typename S,typename T,typename U=nil,typename V=nil,typename W=nil> struct seq:seq_p<S,seq<T,U,V,W> >{};
template<typename A,typename B,typename C=nil,typename D=nil,typename E=nil,typename F=nil,typename G=nil,typename H=nil> struct seq:seq_p<A,seq<B,C,D,E,F,G,H> >{};
template<typename A,typename B> struct seq<A,B>:seq_p<A,B>{};

template<char A,char B,char C=0,char D=0,char E=0,char F=0,char G=0,char H=0> struct seq_c:seq_p<char_p<A>,seq_c<B,C,D,E,F,G,H> >{};
template<char A,char B> struct seq_c<A,B>:seq_p<char_p<A>,char_p<B> >{};


template<typename S,typename T> struct or_p{
	template<typename U> static bool go(U& c){
		int tmp=c.index;
		//std::cout<<"or_p "<<c.index<<" " <<*c<<std::endl;
		if(S::go(c)) return true;
		c.index=tmp;
		//std::cout<<"\tor_p "<<c.index<<" " <<*c<<std::endl;
		if(T::go(c)) return true;
		c.index=tmp;
		//std::cout<<"\t\tor_p "<<c.index<<" " <<*c<<std::endl;
		return false;
	}
};
//could specialize or_p<char_p,char_p>

//template<typename S,typename T,typename U=nil,typename V=nil,typename W=nil> struct choice:or_p<S,choice<T,U,V,W> >{};
template<typename A,typename B,typename C=nil,typename D=nil,typename E=nil,typename F=nil,typename G=nil,typename H=nil> struct choice:or_p<A,choice<B,C,D,E,F,G,H> >{};
template<typename S,typename T> struct choice<S,T>:or_p<S,T>{};

template<typename S> struct not_p{
	template<typename T> static bool go(T& c){
		return !S::go(c);
	}
};
//there is a problem with not_p<or_p<...> > because it will not read in anything
//it does not work with choice!
template<typename S,typename T> struct not_p<or_p<S,T> >{
	template<typename U> static bool go(U& c){
		if(!or_p<S,T>::go(c)){
			++c;//assume we read one char at a time
			return true;
		}
		return false;
	}
};
template<typename S> struct kleene_p:or_p<seq_p<S,kleene_p<S> >,true_p>{}; /* a* b=(ab)|true */
template<typename S> struct plus_p:seq_p<S,or_p<plus_p<S>,true_p> >{}; /* a+ b=a(b|true) */
template<typename S,typename T=nil> struct event{
	//maybe we could pass index to main buffer instead of copying onto string, even better we could have istream available
	template<typename U> static bool go(U& c){
		int begin=(c.index+1)&char_iterator::MASK;
		if(S::go(c)){
			int end=(c.index+1)&char_iterator::MASK;
			std::string s;
			//could be optimized
			if(begin<=end)
				s=std::string(c.buffer+begin,c.buffer+end);
			else
				s=std::string(c.buffer+begin,c.buffer+char_iterator::N)+std::string(c.buffer,c.buffer+end);
			c.callback(event(),s);
			return true;
		}
		return false;
	}
};
#endif
