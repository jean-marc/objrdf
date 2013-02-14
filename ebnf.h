#ifndef EBNF_H
#define EBNF_H
/*
 *
 */
#include <string>
#include <iostream>
#include <vector>
using namespace std;
template<typename T> struct result{
	int id;
	T s;
	typedef vector<result> V;
	V v;	
	friend ostream& operator<<(ostream& os,const result& r){
		os<<"("<<hex<<r.id<<dec<<"\t`"<<string(r.s.begin,r.s.end)<<"'"<<endl;
		for(auto i=r.v.cbegin();i<r.v.cend();++i) os<<*i;
		return os<<")"<<endl;
	}
	result():id(0),s(T()){}
};
struct true_p{
	typedef true_p SELF;
	template<typename T,typename ITERATOR> static bool go(T& t,ITERATOR&){return true;}
};
template<char C> struct char_p{
	typedef char_p SELF;
	template<typename T,typename ITERATOR> static bool go(T& t,ITERATOR&){
		//we could also use a character that is never valid EOF it would save one comparison
		if(t.begin==t.end) return false;
		if(C==*t.begin){
			++t.begin;
			return true;
		}
		return false;
	}
};
template<char A,char Z> struct range_p{
	typedef range_p SELF;
	template<typename T,typename ITERATOR> static bool go(T& t,ITERATOR&){
		if(t.begin==t.end) return false;
		if((A<=*t.begin)&&(*t.begin<=Z)){
			++t.begin;
			return true;
		}
		return false;
	}
};
template<typename A,typename B> struct seq_p{
	typedef seq_p SELF;
	template<typename T,typename ITERATOR> static bool go(T& t,ITERATOR& r){return A::go(t,r)&&B::go(t,r);}
};
template<typename A,typename B> struct or_p{
	typedef or_p SELF;
	template<typename T,typename ITERATOR> static bool go(T& t,ITERATOR& r){
		T tmp=t;
		if(A::go(t,r)) return true;
		t=tmp;
		if(B::go(t,r)) return true;
		t=tmp;
		return false;
	}
};


template<typename... A> struct seq;
template<typename Head,typename... Tail> struct seq<Head,Tail...>:seq_p<Head,seq<Tail...>>{};
template<typename Head> struct seq<Head>:Head{};
template<char... C> struct seq_c:seq<char_p<C>...>{};

template<typename... A> struct choice;
template<typename Head,typename... Tail> struct choice<Head,Tail...>:or_p<Head,choice<Tail...>>{};
template<typename Head> struct choice<Head>:Head{};
template<char... C> struct choice_c:choice<char_p<C>...>{};

typedef choice_c<' ','\t','\n','\r'> white_space;
//typedef white_space ws;

template<typename A,typename _SELF_=typename A::SELF> struct not_p{
	typedef not_p SELF;
	template<typename T,typename ITERATOR> static bool go(T& t,ITERATOR& r){
		//return !A::go(t,r);
		if(!A::go(t,r)){
			++t.begin;//maybe 
			return true;
		}	
		return false;
	}
};
/*
//need to be specialized
template<typename A,typename B,typename C> struct not_p<A,or_p<B,C>>{
	typedef not_p<or_p<A,or_p<B,C>>> SELF;
	template<typename T,typename ITERATOR> static bool go(T& t,ITERATOR& r){
		
		return false;	
	}

};
*/
template<typename S> struct kleene_p:or_p<seq_p<S,kleene_p<S>>,true_p>{}; /* a* b=(ab)|true */
template<typename S> struct plus_p:seq_p<S,or_p<plus_p<S>,true_p>>{}; /* a+ b=a(b|true) */

template<typename... A> struct seq_t;
template<typename Head,typename... Tail> struct seq_t<Head,Tail...>:seq<Head,plus_p<white_space>,seq_t<Tail...>>{};
template<typename Head> struct seq_t<Head>:Head{};

template<typename A,typename SEPARATOR=plus_p<white_space>> struct plus_pt:seq_p<A,or_p<seq_p<SEPARATOR,plus_pt<A,SEPARATOR>>,true_p>>{};
template<typename S,typename SEPARATOR=plus_p<white_space>> struct kleene_pt:or_p<plus_pt<S,SEPARATOR>,true_p>{}; /* a* b=(ab)|true */

template<typename A,typename B> struct equality{enum{N=0};};
template<typename A> struct equality<A,A>{enum{N=1};};

template<typename A,typename B> struct store{};
template<typename A,typename B> struct in_store{enum{N=0};};
template<typename A> struct in_store<A,A>{enum{N=1};};
template<typename A,typename B> struct in_store<A,store<A,B>>{enum{N=1};};
template<typename A,typename B,typename C> struct in_store<A,store<B,C>>:in_store<A,C>{};

struct NIL;
template<
	typename S,
	typename SELF=typename S::SELF,
	typename STORE=NIL,
	int STOP=in_store<S,STORE>::N
> struct find_event{
	enum{N=0};
	enum{UID=0x1111111};
};
template<typename S> struct event{
	typedef event SELF;
	template<typename T,typename ITERATOR> static bool go(T& t,ITERATOR& r){
		auto begin=t.begin;
		result<T> tmp;
		back_insert_iterator<typename result<T>::V> i(tmp.v);
		if(S::go(t,i)){
			cerr<<"\t("<<hex<<find_event<event>::UID<<dec<<" `"<<string(begin,t.begin)<<"')"<<endl;
			tmp.id=find_event<event>::UID;
			tmp.s=T(begin,t.begin);
			*r=tmp;
			++r;
			return true;
		}
		return false;
	}
	template<typename T> static result<T> go(T& t){
		result<T> rs;
		back_insert_iterator<typename result<T>::V> i(rs.v);
		go(t,i);
		return rs;
	}
};

template<
	typename S,
	typename SELF,
	typename STORE
> struct find_event<S,SELF,STORE,1>{
	enum{N=0};
	enum{UID=0x33333333};
};

template<
	typename S,
	char A,
	typename STORE
> struct find_event<S,char_p<A>,STORE,0>{
	enum{N=0};
	enum{UID=A};
};
template<
	typename S,
	typename A,
	typename B,
	typename GUARD
> struct find_event<S,seq_p<A,B>,GUARD,0>{
	enum{N=find_event<A,typename A::SELF,store<S,GUARD>>::N+find_event<B,typename B::SELF,store<S,GUARD>>::N};
	enum{UID=find_event<A,typename A::SELF,store<S,GUARD>>::UID^find_event<B,typename B::SELF,store<S,GUARD>>::UID<<2};
};
template<
	typename S,
	typename A,
	typename B,
	typename GUARD
> struct find_event<S,or_p<A,B>,GUARD,0>{
	enum{N=find_event<A,typename A::SELF,store<S,GUARD>>::N+find_event<B,typename B::SELF,store<S,GUARD>>::N};
	enum{UID=find_event<A,typename A::SELF,store<S,GUARD>>::UID^find_event<B,typename B::SELF,store<S,GUARD>>::UID<<3};
};
template<
	typename S,
	typename A,
	typename GUARD
> struct find_event<S,event<A>,GUARD,0>{
	enum{N=1|find_event<A,typename A::SELF,store<S,GUARD>>::N<<4};
	enum{UID=find_event<A,typename A::SELF,store<S,GUARD>>::UID<<1};
};

template<typename ITERATOR> struct range{
	ITERATOR begin;
	ITERATOR end;
	range(ITERATOR begin,ITERATOR end):begin(begin),end(end){}
	range():begin(0),end(0){}
};
template<typename T> range<T> get_range(T begin,T end){return range<T>(begin,end);}

struct c_iterator{
	struct buffer{
		enum{N=17};
		enum{MASK=N-2};
		int n;//reference counting
		char buffer[N];
		char get(int i)const{return buffer[i&MASK];}
		void set(int i,char c){buffer[i&MASK]=c;}
	};
	istream& is;
	buffer* b;
	int n_read,index;
	c_iterator(istream& is):is(is){
		b=new buffer();
		b->n=1;
	}
	c_iterator(const c_iterator& c):is(c.is){
		b=c.b;
		++b->n;
	}
	void operator=(const c_iterator& c){
		if(b!=c.b){
			--b->n;
			if(b->n==0) delete b;
			b=c.b;
		}
		++b->n;
	}
	~c_iterator(){
		--b->n;
		if(b->n==0) delete b;
	}
	c_iterator& operator++(){
		++index;
		if(index>n_read){
			b->set(index,is.get());		
			++n_read;
		}
		return *this;
	}
	bool operator==(const c_iterator& c) const{
		return index==c.index;
	}
	char operator*(){
		return b->get(index);
	}
};
#endif
