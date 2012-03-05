#ifndef PSEUDO_PTR_H
#define PSEUDO_PTR_H

#include <vector>
#include <iterator>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <string.h>
#include <iostream>
#include <sstream>
#include "ifthenelse.hpp"

using namespace std;
/*
 *	simple pointer to only one type
 *	pointer to type and derived types
 *	can we cast from one to another?
 *
 *	storage: local or memory-mapped
 *
 * 	would be fairly easy to add the reference counting inside, so it would be totaly transparent
 * 	we would just have to build a container with meta information for each cell (counter+other info)
 * 	need a block allocator to use with vector
 * 	it might actually solve some problem with bool and void* casting
 *
 * 	we can also set the size of the reference counter, if the size is set to 0 then a fonction must be
 * 	provided to determine wether a cell is used or not, for instance RDF element should have an id, if
 * 	it is null we can assume that it is not used, an other approach is to provide a null element
 *
 * 	there might be a need for a self-referencing pointer from an object, is it possible?
 * 	yes but a bit awkward: pointer difference
 *
 * 	not all classes will be instantiated, would be nice to have a way to detect those so we don't
 * 	waste a pool eg Literal 
 *
 * 	can we have a safeguard mechanism to make sure we always use the same pool index?
 *
 */

/*
 * http://en.wikipedia.org/wiki/Jenkins_hash_function
 */
uint32_t jenkins_one_at_a_time_hash(const char *key, size_t len)
{
    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}
struct pool{
	struct param{
		void* v;
		size_t n;
		const size_t cell_size;
		/*
 		*	const bool writable;
 		*	when not writable the allocator should always return 0, and const pointers
 		*	should be used
 		*/ 
		param(void* v,size_t n,const size_t cell_size):v(v),n(n),cell_size(cell_size){}
	};
	/*
 	*	careful with the size of this structure as it must fit within a T object
 	*	it will cause very strange behaviour if it does not fit!
 	*/ 
	struct info{
		typedef unsigned short FIELD;
		//typedef unsigned char FIELD;
		FIELD next;
		union{
			FIELD n_cells;
			FIELD cell_size;
		};
	};
	template<typename T> struct helper{
		static_assert(sizeof(T)>=sizeof(info),"T smaller than struct info{}");
		char t[sizeof(T)];
		inline info::FIELD& next(){return static_cast<info*>((void*)this)->next;}
		inline info::FIELD& n_cells(){return static_cast<info*>((void*)this)->n_cells;}
		inline info::FIELD& cell_size(){return static_cast<info*>((void*)this)->cell_size;}
	};
	param p;
	typedef void(*DESTRUCTOR)(void*);
	const DESTRUCTOR destructor; 
	/*
 	* 	more than one pool per type possible
 	*/
	const size_t type_id;
	pool(param p,DESTRUCTOR destructor,size_t type_id):p(p),destructor(destructor),type_id(type_id){
		info& first=*static_cast<info*>(p.v);
		info& second=*static_cast<info*>(p.v+p.cell_size);
		/*if(first.next==0){
			first.next=1;
			first.n_cells=0;
		}*/
		if(first.n_cells==0){
			first.next=1;
			//only used when using allocate_t(int)
			second.next=0;
			second.cell_size=p.n-1;	
		}
		cerr<<"new pool "<<this<<" "<<(size_t)first.n_cells<<" out of "<<p.n<<" cells used type_id:"<<type_id<<endl;
	}
	/*
	size_t allocate(){
		info& first=*static_cast<info*>(p.v);
		info& current=*static_cast<info*>(p.v+first.next*p.cell_size);
		size_t i=first.next;
		if(current.next==0)
			first.next++;
		else
			first.next=current.next;
		first.n_cells++;
		cerr<<"allocate cell at index "<<i<<"{"<<(p.v+i*p.cell_size)<<"} in pool "<<this<<endl;
		return i;	
	}
	*/
	//typed version is safer and cleaner
	template<typename T> size_t allocate_t(){
		auto v=static_cast<helper<T>*>(p.v);
		if(v[0].n_cells()==p.n-1)
			throw std::runtime_error("pool full");
		auto i=v[0].next();
		v[0].next()=v[i].next() ? v[i].next() : v[0].next()+1;
		v[0].n_cells()++;
		cerr<<"allocate cell at index "<<i<<"{"<<&v[i]<<"} in pool "<<this<<endl;
		return i;	
	}
	//contiguous cells for vector<>
	template<typename T> size_t allocate_t(size_t n){
		auto v=static_cast<helper<T>*>(p.v);
		cerr<<"v[0].next():"<<v[0].next()<<endl;
		size_t i=help_allocate_t<T>(v[0].next(),n);
		cerr<<"v[0].next():"<<v[0].next()<<endl;
		return i;
	}

	template<typename T> size_t help_allocate_t(info::FIELD& current,size_t n){
		//we need to go through linked list until we find enough contiguous free cells
		auto v=static_cast<helper<T>*>(p.v);
		cerr<<"attempting to allocate "<<n<<" cells in pool "<<this<<" "<<current<<" size:"<<v[current].cell_size()<<" next:"<<v[current].next()<<" "<<v[0].next()<<endl;
		if(current==0){
			//no more memory
			cerr<<"allocate "<<n<<" cells at index failed in pool "<<this<<endl;
			throw std::runtime_error("pool full");
			return 0;
		}else{
			if(v[current].cell_size()<n)
				return help_allocate_t<T>(v[current].next(),n);
			else if(v[current].cell_size()==n){
				size_t tmp=current;
				current=v[current].next();//cell is destroyed
				v[0].n_cells()+=n;
				cerr<<"allocate "<<n<<" cells at index "<<tmp<<"{"<<&v[tmp]<<"} in pool "<<this<<endl;
				return tmp;
			}else{
				//create new cell but should test if merge possible
				v[current+n].cell_size()=v[current].cell_size()-n;
				v[current+n].next()=v[current].next();
				cerr<<current+n<<" size:"<<v[current+n].cell_size()<<" next:"<<v[current+n].next()<<endl;	
				size_t tmp=current;
				current=current+n;//cell is destroyed
				v[0].n_cells()+=n;
				cerr<<"allocate "<<n<<" cells at index "<<tmp<<"{"<<&v[tmp]<<"} in pool "<<this<<endl;
				return tmp;
			}
		}
	}
	//it is expected that n is the original allocated size
	template<typename T> void deallocate_t(size_t index,size_t n){
		auto v=static_cast<helper<T>*>(p.v);
		cerr<<"deallocate "<<n<<" cells at index "<<index<<" in pool "<<this<<endl;
		//easy way
		//create cell
		v[index].next()=v[0].next();
		v[index].cell_size()=n;
		v[0].next()=index;
		v[0].n_cells()-=n;
	}
	//we can choose the pointer value
	//do not mix allocate_at() with allocate() !
	size_t allocate_at(size_t i){
		info& first=*static_cast<info*>(p.v);
		//that is not correct: keeps increasing counter even if previously occupied
		//should only allowed on non persistent store
		first.n_cells++;
		cerr<<"allocate cell at set index "<<i<<"{"<<p.v+i*p.cell_size<<"} in pool "<<this<<endl;
		return i;	
	}
	void deallocate(size_t i){
		//assume only 1 cell allocated
		cerr<<"deallocate cell at index "<<i<<" in pool "<<this<<endl;
		info& first=*static_cast<info*>(p.v);
		info& current=*static_cast<info*>(p.v+i*p.cell_size);
		//keep linked list of empty cells ordered for iterator
		//it does not work, not really in order, will cause problems in iterator!
		if(i<first.next){
			current.next=first.next;
			first.next=i;
		}else{
			info& second=*static_cast<info*>(p.v+first.next*p.cell_size);
			current.next=second.next;
			second.next=i;
		}
		first.n_cells--;
	}
	template<typename T> void deallocate_t(size_t i){
		auto v=static_cast<helper<T>*>(p.v);
		cerr<<"deallocate cell at index "<<i<<" in pool "<<this<<endl;
		//keep linked list of empty cells ordered for iterator
		if(i<v[0].next()){
			v[i].next()=v[0].next();
			v[0].next()=i;
		}else{
			v[i].next()=v[v[0].next()].next();
			v[v[0].next()].next()=i;
		}
		v[0].n_cells()--;
	}
	void* get(size_t i){
		return p.v+i*p.cell_size;
	}
	//could use function pointer:allocate, deallocate,...
	size_t get_size()const{return static_cast<info*>(p.v)->n_cells;}
	/*
	static size_t& get_counter(){
		static size_t c=1;//needs to start at 1
		return c;
	}
	template<typename T> static size_t get_type_id(){
		static size_t id=get_counter()++;
		return id;
	}
	*/
	template<typename T,typename STORE> static pool* get_instance(){
		//static pool* p=new pool(STORE::template go<T>(),0,get_type_id<T>());
		static pool* p=new pool(STORE::template go<T>(),0,0);
		return p;
	}
	/*
 	*	typed iterator: go through all the cells and skips free ones
 	*	how do we know if cell is used?	we only know which ones are free
 	*	forward iterator
 	*	not necessary when we know it is dense (no deallocation took place)
 	*	we could sort gaps when creating iterator
 	*	better approach is to test each cell with custom test function
 	*/ 
	//T must be a pseudo_ptr	
	//if not the right size it should 
	template<typename T> struct iterator{
		typedef T value_type;	
		typedef T& reference;	
		typedef typename T::value_type S;
		S* v;
		info::FIELD index;//address current cell
		info::FIELD n;//number of cells to visit
		iterator(S* v,info::FIELD n):v(v),index(1),n(n){while(!S::TEST(v[index])&&n) ++index;}		
		bool operator==(const iterator& p)const{return n==p.n;}
		bool operator!=(const iterator& p)const{return n!=p.n;}
		bool operator<(const iterator& p)const{return n>p.n;}
		iterator& operator++(){
			++index;
			while(!S::TEST(v[index])&&n) ++index;
			--n;
			return *this;
		}
		iterator& operator+(info::FIELD k){
			//not very efficient
			for(int i=0;i<k;++i)
				++(*this);
			return *this;
		}
		value_type operator*(){return T(index);}
	};
	template<typename T> iterator<T> begin(){return iterator<T>(static_cast<typename T::value_type*>(p.v),static_cast<helper<typename T::value_type>*>(p.v)[0].n_cells());}
	template<typename T> iterator<T> end(){return iterator<T>(static_cast<typename T::value_type*>(p.v),0);}
};

struct empty_store{//for classes with no instance
	enum{N=0};
};

template<size_t SIZE=20000> struct free_store{
	enum{N=1};
	template<typename T> static pool::param go(){
		char* v=new char[SIZE*sizeof(T)];
		memset(v,0,SIZE*sizeof(T));
		return pool::param(v,SIZE,sizeof(T));
	}
};
template<typename T> struct name;//{static const string get(){return "none";}};
template<size_t SIZE=20000> struct persistent_store{
	enum{N=2};
	//use memory map
	//how to add support for read-only file?
	template<typename T> static pool::param go(){
		/*
 		* should add ascii file with version information, that number could come from git
		* to build compatible executable
		*/ 
		string filename="db/"+name<typename T::SELF>::get();//prevents from using with other type!
		//string filename="db/"+name<T>::get();//prevents from using with other type!
		//string filename="db/none";//prevents from using with other type!
		cerr<<"opening file `"<<filename<<"'"<<endl;
		int fd = open(filename.c_str(), O_RDWR | O_CREAT/* | O_TRUNC*/, (mode_t)0600);
		if (fd == -1) {
			cerr<<"\nError opening file `"<<filename<<"' for writing"<<endl;
			exit(EXIT_FAILURE);
		}
		//set the size
		struct stat s;
		int r=fstat(fd,&s);
		if(r==-1){
			cerr<<"\ncould not stat file `"<<filename<<"'"<<endl;
			exit(EXIT_FAILURE);
		}
		size_t file_size=max<size_t>(4096,sizeof(T)*SIZE);
		if(s.st_size<file_size){
			int result = lseek(fd,file_size-1, SEEK_SET);
			if (result == -1) {
				close(fd);
				cerr<<"Error calling lseek() to 'stretch' the file"<<endl;
				exit(EXIT_FAILURE);
			}
			result = write(fd, "", 1);
			if (result != 1) {
				close(fd);
				cerr<<"Error writing last byte of the file"<<endl;
				exit(EXIT_FAILURE);
			}
		}
		char* v = (char*)mmap((void*)NULL,file_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (v == MAP_FAILED) {
			close(fd);
			cerr<<"Error mmapping the file"<<endl;
			exit(EXIT_FAILURE);
		}
		return pool::param(v,file_size/sizeof(T),sizeof(T));
	}
};
template<
	typename T,
	//maybe store SHOULD not be a template parameter, maybe a constructor parameter instead
	typename STORE=free_store<>,//free store or persistent
	bool POLYMORPHISM=false//does not support derived types
> struct pseudo_ptr;
/*
 *	we can add container:
 *	struct container{
 *		M meta;
 *		T payload;
 *	};
 *	meta could be reference counter or other data
 *
 */
template<typename T> struct const_test{enum{N=0};};
template<typename T> struct const_test<const T>{enum{N=1};};

template<
	typename STORE
> 
struct pseudo_ptr<pool,STORE,false>{
	typedef pool value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef pool& reference;
	typedef unsigned char INDEX;//we don't expect more than 255 pools, that is 255 types
	INDEX index;
	explicit pseudo_ptr(INDEX index):index(index){}
	static pseudo_ptr allocate(){return pseudo_ptr(pool::get_instance<pool,STORE>()->template allocate_t<pool>());}
	static pseudo_ptr allocate_at(INDEX i){return pseudo_ptr(pool::get_instance<pool,STORE>()->template allocate_at(i));}
	static pseudo_ptr construct(pool::param s,pool::DESTRUCTOR d,size_t type_id){
		pseudo_ptr p=allocate();
		new(p)pool(s,d,type_id);
		return p;
	}
	static pseudo_ptr construct_at(INDEX i,pool::param s,pool::DESTRUCTOR d,size_t type_id){
		pseudo_ptr p=allocate_at(i);
		new(p)pool(s,d,type_id);
		return p;
	}
	template<typename T> static void destructor(void* s){static_cast<T*>(s)->~T();}
	template<typename T> static INDEX get_type_id(){
		string n=name<typename T::SELF>::get();
		uint32_t h=jenkins_one_at_a_time_hash(n.c_str(),n.size());
		return h&0xff;//hopefully no collision
	}
	//should also take constness into account
	template<typename T,typename OTHER_STORE> static INDEX get_pool_id(){
		string n=name<typename T::SELF>::get();
		uint32_t h=jenkins_one_at_a_time_hash(n.c_str(),n.size()-OTHER_STORE::N*const_test<T>::N);
		return h&0xff;//hopefully no collision
	}
	template<typename T,typename OTHER_STORE> static pseudo_ptr get(){
		/*
		*	we need a hash based on T and OTHER_STORE so that it does not depend on the order in
		*	which pools are created, one way it to persist this pool
		*
		*/ 
		//static pseudo_ptr p=construct(OTHER_STORE::template go<T>(),destructor<T>,pool::get_type_id<T>());
		static pseudo_ptr p=construct_at(get_pool_id<T,OTHER_STORE>(),OTHER_STORE::template go<T>(),destructor<T>,get_type_id<T>());
		return p;
	}
	static value_type* get_typed_v(){return static_cast<value_type*>(pool::get_instance<pool,STORE>()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	bool operator==(const pseudo_ptr& p)const{return index==p.index;}
	bool operator!=(const pseudo_ptr& p)const{return index!=p.index;}
	operator value_type*() {return get_typed_v()+index;}
};
typedef pseudo_ptr<pool,free_store<>,false> POOL_INDEX;

template<
	typename T
> struct pseudo_ptr<T,empty_store,false>{
	static POOL_INDEX get_pool(){
		static POOL_INDEX p=POOL_INDEX::allocate();
		return p;
	}
}; 

template<typename S,typename T> struct is_derived{enum{value=false};};
template<typename S> struct is_derived<S,S>{enum{value=true};};	
/*
 *	this pointer is smaller than the polymorphic one, 
 *	use when the class is not up/down cast
 */
template<
	typename T,
	typename _STORE_
> struct pseudo_ptr<T,_STORE_,false>{
	typedef _STORE_ STORE;
	/*
 	* we need to define a unique type to avoid ambiguity between constructors
 	* 	pseudo_ptr(INDEX index)
 	* 	pseudo_ptr(T* p)
 	* e.g:
 	* 	struct unique{
 	* 		INDEX i;
 	* 	};
 	*/
	//typedef unsigned char INDEX;
	typedef unsigned short INDEX;

	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef T& reference;
	static POOL_INDEX get_pool(){return POOL_INDEX::get<T,STORE>();}
	INDEX index;
	//explicit pseudo_ptr(INDEX index):index(index){}
	pseudo_ptr(INDEX index=0):index(index){}
	/*
 	* dangerous, be very careful with this
 	* 	should be used like this:
 	* 	pseudo_ptr a(new(pseudo_ptr::allocate())T(...)
 	* problem: constructors are now ambiguous, would be nice if we could name it
	*/
	//explicit pseudo_ptr(T* p):index(((void*)p-get_pool()->p.v)/sizeof(T)){}
	/*
 	*	only makes sense if S and T are related, not clear if they should use same store
 	*
 	*/
	template<typename S,typename OTHER_STORE> pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,true>& p):index(p.index){
		//we can make it safe but should be optional 
		static_assert(is_derived<S,T>::value||is_derived<T,S>::value,"types are not related");
		assert(p.pool_index==get_pool());
	} 
	//cast away constness, should be used with care
	pseudo_ptr(const pseudo_ptr<const T,STORE>& p):index(p.index){}
	pseudo_ptr& operator=(const pseudo_ptr& p){
		index=p.index;
		return *this;
	}
	//static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->allocate());}
	static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->template allocate_t<T>());}
	//equivalent to return new T(s), could use variadic template
	template<typename A> static pseudo_ptr construct(A a){
		pseudo_ptr p=allocate();
		new(p)T(a);//in-place constructor
		return p;
	} 
	template<typename A,typename B> static pseudo_ptr construct(A a,B b){
		pseudo_ptr p=allocate();
		new(p)T(a,b);//in-place constructor
		return p;
	} 
	template<typename A,typename B,typename C> static pseudo_ptr construct(A a,B b,C c){
		pseudo_ptr p=allocate();
		new(p)T(a,b,c);//in-place constructor
		return p;
	} 
	//void deallocate(){get_pool()->deallocate(index);}
	void deallocate(){get_pool()->template deallocate_t<T>(index);}
	void destroy(){
		(*this)->~T();//in-place destructor
		deallocate();		
	}
	static value_type* get_typed_v(){return static_cast<value_type*>(get_pool()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	pseudo_ptr& operator+=(const size_t s){index+=s;return *this;}
	pseudo_ptr& operator-=(const size_t s){index-=s;return *this;}
	pseudo_ptr& operator++(){++index;return *this;}
	pseudo_ptr& operator--(){--index;return *this;}
	bool operator!=(const pseudo_ptr& p)const{return index!=p.index;}
	bool operator<(const pseudo_ptr& p)const{return index<p.index;}
	bool operator==(const pseudo_ptr& p)const{return index==p.index;}
	//
	bool operator==(const void* p)const{return (get_typed_v())+index==p;}
	//
	//never called!
	operator bool() const{return index;}
	//problem: not equal to zero even when index=0
	operator value_type*() {return index ? get_typed_v()+index:0;}
	bool operator!()const{return !index;}
};
/*
 *	can point to sub- and super-class of T
 */
template<
	typename T,
	typename _STORE_
>
struct pseudo_ptr<T,_STORE_,true>{
	typedef _STORE_ STORE;
	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef T& reference;
	typedef unsigned short INDEX;
	POOL_INDEX pool_index;
	INDEX index;
	static POOL_INDEX get_pool(){return POOL_INDEX::get<T,STORE>();}
	explicit pseudo_ptr(INDEX index=0):index(index),pool_index(get_pool()){}
	void print(ostream& os){
		os<<"{"<<(unsigned int)pool_index.index<<","<<(unsigned short)index<<"}"<<endl;
	}
	/*
 	* dangerous, be very careful with this
 	* 	should be used like this:
 	* 	pseudo_ptr a(new(pseudo_ptr::allocate())T(...)
	*/
	//explicit pseudo_ptr(T* p):index(((void)p-get_pool()->p.v)/sizeof(T)),pool_index(get_pool()){}
	//static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->allocate());}
	static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->template allocate_t<T>());}
	static pseudo_ptr allocate_at(size_t i){return pseudo_ptr(get_pool()->allocate_at(i));}
	template<typename A> static pseudo_ptr construct(A a){
		pseudo_ptr p=allocate();
		new(p)T(a);//in-place constructor
		return p;
	}
	/*
 	*	could avoid all that by using std::tuples
 	*/ 
	template<typename A,typename B> static pseudo_ptr construct(A a,B b){
		pseudo_ptr p=allocate();
		new(p)T(a,b);//in-place constructor
		return p;
	}
	template<typename A,typename B,typename C> static pseudo_ptr construct(A a,B b,C c){
		pseudo_ptr p=allocate();
		new(p)T(a,b,c);//in-place constructor
		return p;
	}
	//maybe should only be for pseudo_ptr<const T>
	template<typename A,typename B,typename C> static pseudo_ptr construct_at(size_t i,A a,B b,C c){
		pseudo_ptr p=allocate_at(i);
		new(p)T(a,b,c);//in-place constructor
		return p;
	}
	template<typename A,typename B,typename C,typename D> static pseudo_ptr construct(A a,B b,C c,D d){
		pseudo_ptr p=allocate();
		new(p)T(a,b,c,d);//in-place constructor
		return p;
	}
	template<typename A,typename B,typename C,typename D> static pseudo_ptr construct_at(size_t i,A a,B b,C c,D d){
		pseudo_ptr p=allocate_at(i);
		new(p)T(a,b,c,d);//in-place constructor
		return p;
	}
	void deallocate(){pool_index->deallocate(index);}
	void destroy(){
		//a bit more complicated: it could be any type, we need to store a function pointer somewhere
		cerr<<pool_index->destructor<<endl;
		pool_index->destructor(*this);
		deallocate();
	}
	template<
		typename S,
		typename OTHER_STORE
	> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,false>& p):pool_index(pseudo_ptr<S,OTHER_STORE,false>::get_pool()),index(p.index){
		enum{a=is_derived<typename S::SELF,typename T::SELF>::value};
		enum{b=is_derived<typename T::SELF,typename S::SELF>::value};
		//so we get explicit error message
		typedef typename IfThenElse<a||b,pseudo_ptr,T>::ResultT::STORE A;
		typedef typename IfThenElse<a||b,pseudo_ptr,S>::ResultT::STORE B;
		static_assert(is_derived<typename S::SELF,typename T::SELF>::value||is_derived<typename T::SELF,typename S::SELF>::value,"types are not related");
	}
	template<
		typename S,
		typename OTHER_STORE
	> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,true>& p):pool_index(p.pool_index),index(p.index){
		enum{a=is_derived<typename S::SELF,typename T::SELF>::value};
		enum{b=is_derived<typename T::SELF,typename S::SELF>::value};
		//so we get explicit error message
		typedef typename IfThenElse<a||b,pseudo_ptr,T>::ResultT::STORE A;
		typedef typename IfThenElse<a||b,pseudo_ptr,S>::ResultT::STORE B;
		static_assert(is_derived<typename S::SELF,typename T::SELF>::value||is_derived<typename T::SELF,typename S::SELF>::value,"types are not related");
	}
	value_type* operator->()const{return (value_type*)pool_index->get(index);}
	reference operator*()const{return *(value_type*)pool_index->get(index);}
	/*
 	* shall we override operator->*() to use pointer to member functions?
 	* we need to know the return type of the function
 	*/
	//template<typename F> void operator->*(F f){static_cast<value_type*>(pool_index->get(index))->*f;}
	//does it even make sense?
	/*
	pseudo_ptr& operator+=(const size_t s){index+=s;return *this;}
	pseudo_ptr& operator-=(const size_t s){index-=s;return *this;}
	pseudo_ptr& operator++(){++index;return *this;}
	pseudo_ptr& operator--(){--index;return *this;}
	*/
	bool operator!=(const pseudo_ptr& p)const{return pool_index!=p.pool_index || index!=p.index;}
	bool operator<(const pseudo_ptr& p)const{return pool_index==p.pool_index && index<p.index;}
	bool operator==(const pseudo_ptr& p)const{return pool_index==p.pool_index && index==p.index;}
	//
	bool operator==(const void* p)const{return pool_index->get(index)==p;}
	//never used
	operator bool() const{return index;}
	//not very efficient
	operator value_type*() {return index ? (value_type*)pool_index->get(index) : 0;}
	//better
	bool operator!()const{return !index;}
};
template<typename T,typename STORE,bool POLYMORPHISM> pseudo_ptr<T,STORE,POLYMORPHISM> operator+(const pseudo_ptr<T,STORE,POLYMORPHISM>& a,size_t s){
	pseudo_ptr<T,STORE,POLYMORPHISM> tmp=a;
	return tmp+=s;
}
//template<typename T,typename STORE,bool POLYMORPHISM> pseudo_ptr<T,STORE,POLYMORPHISM> operator-(const pseudo_ptr<T,STORE,POLYMORPHISM>& a,size_t s){
template<typename T,typename STORE> pseudo_ptr<T,STORE,false> operator-(const pseudo_ptr<T,STORE,false>& a,size_t s){
	pseudo_ptr<T,STORE,false> tmp=a;
	return tmp-=s;
}
//should make sure types are the same, make sure no casting takes place, could also compare pool_index
template<typename T,typename STORE,bool POLYMORPHISM> ptrdiff_t operator-(const pseudo_ptr<T,STORE,POLYMORPHISM>& a,const pseudo_ptr<T,STORE,POLYMORPHISM>& b){
	return a.index-b.index;
}
template<
	typename T,
	typename _STORE_
> struct pseudo_ptr<const T,_STORE_,false>{
	typedef _STORE_ STORE;
	typedef random_access_iterator_tag iterator_category;
	typedef const T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef const T& reference;
	typedef unsigned short INDEX;
	//should it use its own pool?, that is the safest 
	//static POOL_INDEX get_pool(){return POOL_INDEX::get<const T,STORE>();}
	static POOL_INDEX get_pool(){return POOL_INDEX::get<T,STORE>();}
	INDEX index;
	pseudo_ptr(INDEX index=0):index(index){}
	//we don't need this
	pseudo_ptr& operator=(const pseudo_ptr& p){
		index=p.index;
		return *this;
	}
	static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->template allocate_t<T>());}
	static pseudo_ptr allocate_at(size_t i){return pseudo_ptr(get_pool()->template allocate_at(i));}
	template<typename A> static pseudo_ptr construct(A a){
		pseudo_ptr p=allocate();
		new(p)T(a);//in-place constructor
		return p;
	} 
	template<typename A,typename B> static pseudo_ptr construct(A a,B b){
		pseudo_ptr p=allocate();
		new(p)T(a,b);//in-place constructor
		return p;
	} 
	template<typename A,typename B,typename C> static pseudo_ptr construct(A a,B b,C c){
		pseudo_ptr p=allocate();
		new(p)T(a,b,c);//in-place constructor
		return p;
	} 
	template<typename A> static pseudo_ptr construct_at(size_t i,A a){
		pseudo_ptr p=allocate_at(i);
		new(p)A(a);//in-place constructor
		return p;
	} 
	template<typename A,typename B> static pseudo_ptr construct_at(size_t i,A a,B b){
		pseudo_ptr p=allocate(i);
		new(p)T(a,b);//in-place constructor
		return p;
	}
	template<typename A,typename B,typename C> static pseudo_ptr construct_at(size_t i,A a,B b,C c){
		pseudo_ptr p=allocate_at(i);
		new(p)T(a,b,c);//in-place constructor
		return p;
	}
	template<typename A,typename B,typename C,typename D> static pseudo_ptr construct_at(size_t i,A a,B b,C c,D d){
		pseudo_ptr p=allocate_at(i);
		new(p)T(a,b,c,d);//in-place constructor
		return p;
	}
	static T* get_typed_v(){return static_cast<T*>(get_pool()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	//do we ever use those operators? only custom allocator
	pseudo_ptr& operator+=(const size_t s){index+=s;return *this;}
	pseudo_ptr& operator-=(const size_t s){index-=s;return *this;}
	pseudo_ptr& operator++(){++index;return *this;}
	pseudo_ptr& operator--(){--index;return *this;}
	bool operator!=(const pseudo_ptr& p)const{return index!=p.index;}
	bool operator<(const pseudo_ptr& p)const{return index<p.index;}
	bool operator==(const pseudo_ptr& p)const{return index==p.index;}
	operator T*() {return index ? get_typed_v()+index:0;}

};
//iterators to examine a pool
template<typename T> pool::iterator<T> begin(){return T::get_pool()->begin<T>();}
template<typename T> pool::iterator<T> end(){return T::get_pool()->end<T>();}


#endif

