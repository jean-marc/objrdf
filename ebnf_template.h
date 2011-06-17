#ifndef EBNF_TEMPLATE_H
#define EBNF_TEMPLATE_H
#include <typeinfo>
#include "result.h"
/*
 *	is there anyway we could ignore the result of parsing?
 *
 */
#include "char_iterator.h"
struct nil;
struct true_p{
	static bool go(char_iterator& c){return true;}
};
struct false_p{
	static bool go(char_iterator& c){return false;}
};

template<char C> struct char_p{
	char p[2];
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
	//enum{HASH=C};
};

template<char A,char Z> struct range_p{
	template<typename T> static bool go(T& c){
		++c;
		bool r=(A<=*c)&&(*c<=Z);
		return r;
	}
	//enum{HASH=A+(Z<<sizeof(A))};
};
template<typename S,typename T>	struct seq_p{
	template<typename U> static bool go(U& c){
		return S::go(c)&&T::go(c);
	}
	//enum{HASH=S::HASH*T::HASH};
	//enum{HASH=S::HASH};
};


//template<typename S,typename T,typename U=nil,typename V=nil,typename W=nil> struct seq:seq_p<S,seq<T,U,V,W> >{};
template<typename A,typename B,typename C=nil,typename D=nil,typename E=nil,typename F=nil,typename G=nil,typename H=nil> struct seq:seq_p<A,seq<B,C,D,E,F,G,H> >{};
template<typename A,typename B> struct seq<A,B>:seq_p<A,B>{};

template<char A,char B,char C=0,char D=0,char E=0,char F=0,char G=0,char H=0> struct seq_c:seq_p<char_p<A>,seq_c<B,C,D,E,F,G,H> >{};
template<char A,char B> struct seq_c<A,B>:seq_p<char_p<A>,char_p<B> >{};


template<typename S,typename T> struct or_p{
	template<typename U> static bool go(U& c){
		int tmp=c.char_iterator::index;
		//std::cout<<"or_p "<<c.index<<" " <<*c<<std::endl;
		if(S::go(c)) return true;
		c.char_iterator::index=tmp;
		//std::cout<<"\tor_p "<<c.index<<" " <<*c<<std::endl;
		if(T::go(c)) return true;
		c.char_iterator::index=tmp;
		//std::cout<<"\t\tor_p "<<c.index<<" " <<*c<<std::endl;
		return false;
	}
	//enum{HASH=S::HASH+T::HASH};
	//enum{HASH=T::HASH};
};
//could specialize or_p<char_p,char_p>
template<typename A,typename B,typename C=nil,typename D=nil,typename E=nil,typename F=nil,typename G=nil,typename H=nil> struct choice:or_p<A,choice<B,C,D,E,F,G,H> >{};
template<typename S,typename T> struct choice<S,T>:or_p<S,T>{};

template<typename S> struct not_p{
	template<typename T> static bool go(T& c){
		return !S::go(c);
	}
	//enum{HASH=-S::HASH};
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
	//enum{HASH=-S::HASH};
};
//recursion 
template<typename S> struct kleene_p:or_p<seq_p<S,kleene_p<S> >,true_p>{}; /* a* b=(ab)|true */
template<typename S> struct plus_p:seq_p<S,or_p<plus_p<S>,true_p> >{}; /* a+ b=a(b|true) */

//common parsers
typedef or_p<char_p<' '>,or_p<char_p<'\t'>,or_p<char_p<'\r'>,char_p<'\n'> > > > white_space;

//consume whitespace
template<typename S,typename T>	struct seq_pw:seq_p<S,seq_p<kleene_p<white_space>,T> >{};
template<typename A,typename B,typename C=nil,typename D=nil,typename E=nil,typename F=nil,typename G=nil,typename H=nil> struct seqw:seq_pw<A,seqw<B,C,D,E,F,G,H> >{};
template<typename A,typename B> struct seqw<A,B>:seq_pw<A,B>{};
template<typename S> struct kleene_pw:or_p<seq_pw<S,kleene_p<S> >,true_p>{}; /* a* b=(ab)|true */
template<typename S> struct plus_pw:seq_pw<S,or_p<plus_pw<S>,true_p> >{}; /* a+ b=a(b|true) */


template<typename S,typename T=nil> struct event{
	//maybe we could pass index to main buffer instead of copying onto string, even better we could have istream available
	//enum{HASH=S::HASH};
	template<typename U> static bool go(U& c){
		int begin=(c.char_iterator::index+1)&char_iterator::MASK;
		if(S::go(c)){
			int end=(c.char_iterator::index+1)&char_iterator::MASK;
			std::string s;
			//could be optimized
			if(begin<=end)
				s=std::string(c.buffer+begin,c.buffer+end);
			else
				s=std::string(c.buffer+begin,c.buffer+char_iterator::N)+std::string(c.buffer,c.buffer+end);
			//should return a bool to exit early
			c.callback(event(),s);
			return true;
		}
		return false;
	}
};
#ifdef TTTTTTT
template<typename S,typename T=nil> struct event_1:S{
	static char_iterator::ID id;
};
#else
struct _help_{
	int& depth;
	_help_(int& depth):depth(depth){++depth;}
	~_help_(){--depth;}
};
ostream& operator<<(ostream& os,char_iterator::R& vv){
	return os<<get<0>(vv)<<"\t"<<get<1>(vv)<<"\t"<<get<2>(vv);
}
//ostream& operator<<(ostream& os,ch
typedef std::pair<char_iterator::ID,string> PARSE_RES;
typedef _result_<PARSE_RES> PARSE_RES_TREE;
ostream& operator<<(ostream& os,PARSE_RES& r){
	return os<<r.first<<"\t"<<r.second;
}
void print(ostream& os,PARSE_RES_TREE& p,int depth){
	string s(depth,'\t');
	os<<p.t.first<<"\t"<<s<<p.t.second<<endl;
	for(auto i=p.v.begin();i<p.v.end();++i) print(os,*i,depth+1);
	
}
ostream& operator<<(ostream& os,PARSE_RES_TREE& p){
	print(os,p,0);
	return os;
};
template<typename S,int ID> struct event_1{
	//maybe we could pass index to main buffer instead of copying onto string, even better we could have istream available
	/*
 	* switch id{
 	*	case token_0::id:
 	*	case token_1::id:
 	*	...
 	* }
 	* unique id for each event, would be nice to hash the class (sizeof, typeof,...) so we could use enum
 	* problem: recursion breaks it
 	* sizeof could work if we add members char[N] but same problem with recursion
 	* we could also have a template attribute, but a bit awkward because we need to keep track of all events
 	*/
	enum{id=ID};
	//static size_t const _id=some_hash();
	//static const type_info _id=typeid(S); 
	//enum{HASH=S::HASH};
	template<typename U> static bool go(U& c){
		const bool is_top=c.char_iterator::depth==0;
		_help_ h(c.char_iterator::depth);
		int begin=(c.char_iterator::index+1)&char_iterator::MASK;
		//we could augment type at this stage and steal all callbacks, but it means copying char_iterator: difficult
		int n=c.char_iterator::v.size();
		if(S::go(c)){
			int end=(c.char_iterator::index+1)&char_iterator::MASK;
			std::string s;
			//could be optimized
			if(begin<=end)
				s=std::string(c.buffer+begin,c.buffer+end);
			else
				s=std::string(c.buffer+begin,c.buffer+char_iterator::N)+std::string(c.buffer,c.buffer+end);
			c.char_iterator::v.push_back(char_iterator::R(id,s,c.char_iterator::depth));
			if(is_top){
				//still a bit annoying: comes in sequentially 
				/*
				for(auto i=c.char_iterator::v.begin();i<c.char_iterator::v.end();++i){
					c.callback(i->first,i->second);
				}
				*/
				//create tree from vector
				PARSE_RES_TREE t;
				for(auto i=c.char_iterator::v.rbegin();i<c.char_iterator::v.rend();++i) 
					t.add(PARSE_RES(get<0>(*i),get<1>(*i)),get<2>(*i)-1);
				c.char_iterator::v.clear();
				//send tree
				return c.callback(t.v[0]);
				//send the whole vector
				//bool r=c.callback(c.char_iterator::v);
			}
			return true;
		}else{
			c.char_iterator::v.resize(n);
			if(is_top){
				c.char_iterator::v.clear();
			}
			return false;
		}
	}
};
//template<typename S> char_iterator::ID event_1<S>::id=new int;
#endif
template<typename S,typename T,void (T::*fun)()> struct test{
	/*
 	*	could work but notation heavy because of pointer to member function  
 	*/
	template<typename U> static bool go(U& c){
		(c.*fun)();
		//c.callback(id,s);
		return true;
	}
};
#endif
