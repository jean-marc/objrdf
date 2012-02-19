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
//#include "objrdf.h"

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
 * 	there might be a need for a self-referencing pointer from an object, is it possible?
 * 	yes but a bit awkward: pointer difference
 *
 * 	pool is a good place to store function pointer to emulate virtual function
 * 	template<typename F> apply()
 *
 *
 */

struct pool{
	struct param{
		void* v;
		size_t n;
		const size_t cell_size;
		param(void* v,size_t n,const size_t cell_size):v(v),n(n),cell_size(cell_size){}
	};
	struct info{
		size_t next;
		size_t n_cells;
	};
	param p;
	typedef void(*DESTRUCTOR)(void*);
	const DESTRUCTOR destructor; 
	pool(param p,DESTRUCTOR destructor):p(p),destructor(destructor){
		info& first=*static_cast<info*>(p.v);
		if(first.next==0){
			first.next=1;
			first.n_cells=0;
		}
		cerr<<"new pool "<<this<<" "<<first.n_cells<<" out of "<<p.n<<" cells used"<<endl;
	}
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
	template<typename T> union helper{
		char t[sizeof(T)];//padding
		struct{
			size_t next;
			size_t n_cells;
		};
	};
	//typed version is safer and cleaner
	template<typename T> size_t allocate_t(){
		auto v=static_cast<helper<T>*>(p.v);
		size_t i=v[0].next;
		v[0].next=v[i].next ? v[i].next : v[0].next+1;
		v[0].n_cells++;
		cerr<<"allocate cell at index "<<i<<"{"<<&v[i]<<"} in pool "<<this<<endl;
		return i;	
	}
	//we can choose the pointer value
	//do not mix allocate_at with allocate !
	size_t allocate_at(size_t i){
		info& first=*static_cast<info*>(p.v);
		first.n_cells++;
		cerr<<"allocate cell at index "<<i<<" in pool "<<this<<endl;
		return i;	
	}
	void deallocate(size_t i){
		//assume only 1 cell allocated
		cerr<<"deallocate cell at index "<<i<<" in pool "<<this<<endl;
		info& first=*static_cast<info*>(p.v);
		info& current=*static_cast<info*>(p.v+i*p.cell_size);
		//we keep it in order
		if(i<first.next){
			current.next=first.next;
			first.next=i;
		}else{
			info& second=*static_cast<info*>(p.v+first.next*p.cell_size);
			current.next=second.next;
			second.next=i;
		}
		/*
		current.next=first.next;	
		first.next=i;
		*/
		first.n_cells--;
	}
	template<typename T> void deallocate_t(size_t i){
		auto v=static_cast<helper<T>*>(p.v);
		cerr<<"deallocate cell at index "<<i<<" in pool "<<this<<endl;
		if(i<v[0].next){
			v[i].next=v[0].next;
			v[0].next=i;
		}else{
			v[i].next=v[v[0].next].next;
			v[v[0].next].next=i;
		}
		v[0].n_cells--;
	}
	void* get(size_t i){
		return p.v+i*p.cell_size;
	}
	//could use function pointer:allocate, deallocate,...
	size_t get_size()const{return static_cast<info*>(p.v)->n_cells;}
	template<typename T,typename STORE> static pool* get_instance(){
		static pool* p=new pool(STORE::go(),0);
		return p;
	}
	/*
 	*	typed iterator: go through all the cells and skips free ones
 	*	how do we know if cell is used?	we only know which ones are free
 	*	forward iterator
 	*	not necessary when we know it is dense (no deallocation took place)
 	*/ 
	template<typename T> struct iterator{
		helper<T>* v;
		size_t index;//address current cell
		size_t n;//number of cells to visit
		size_t next_free;//address next free cell
		iterator(helper<T>* v,size_t n):v(v),index(1),n(n),next_free(v[0].next){
			while(index==next_free&&n){
				++index;
				next_free=v[next_free].next;
			}
		}		
		bool operator==(const iterator& p)const{return n==p.n;}
		bool operator!=(const iterator& p)const{return n!=p.n;}
		bool operator<(const iterator& p)const{return n>p.n;}
		iterator& operator++(){
			++index;
			while(index==next_free&&n){
				++index;
				next_free=v[next_free].next;
			}
			--n;
			return *this;
		}
		//we need a pseudo_ptr!
		T* operator->(){return (T*)(void*)&v[index];}		
		T& operator*(){return (T&)(void*)&v[index];}
	};
	template<typename T> iterator<T> begin(){return iterator<T>(static_cast<helper<T>*>(p.v),static_cast<helper<T>*>(p.v)[0].n_cells);}
	template<typename T> iterator<T> end(){return iterator<T>(static_cast<helper<T>*>(p.v),0);}
};
template<typename T,size_t SIZE=256> struct free_store{
	static pool::param go(){
		char* v=new char[SIZE*sizeof(T)];
		memset(v,0,SIZE*sizeof(T));
		return pool::param(v,SIZE,sizeof(T));
	}
};
template<typename T,size_t SIZE,int NAME> struct persistent_store{
	//use memory map
	static pool::param go(){
		ostringstream os;
		os<<"db/pool_"<<NAME;
		string filename=os.str();
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
	typename STORE=free_store<T>,//free store or persistent
	bool POLYMORPHISM=false//does not support derived types
> struct pseudo_ptr;
//a lot of code duplication...maybe should define base class and specialize
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
	static pseudo_ptr construct(pool::param s,pool::DESTRUCTOR d){
		pseudo_ptr p=pseudo_ptr(pool::get_instance<pool,STORE>()->allocate());
		new(p)pool(s,d);
		return p;
	}
	template<typename T> static void destructor(void* s){static_cast<T*>(s)->~T();}
	template<typename T,typename OTHER_STORE> static pseudo_ptr get(){
		static pseudo_ptr p=construct(OTHER_STORE::go(),destructor<T>);
		return p;
	}
	static value_type* get_typed_v(){return static_cast<value_type*>(pool::get_instance<pool,STORE>()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	bool operator==(const pseudo_ptr& p)const{return index==p.index;}
	bool operator!=(const pseudo_ptr& p)const{return index!=p.index;}
	operator void*() {return get_typed_v()+index;}
};
typedef pseudo_ptr<pool,free_store<pool>,false> POOL_INDEX;
template<
	typename T,
	typename STORE
> struct pseudo_ptr<T,STORE,false>{
	typedef unsigned char INDEX;//we need to define a unique type!
	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef T& reference;
	static POOL_INDEX get_pool(){return POOL_INDEX::get<T,STORE>();}
	INDEX index;
	explicit pseudo_ptr(INDEX index):index(index){}
	/*
 	* dangerous, be very careful with this
 	* 	should be used like this:
 	* 	pseudo_ptr a(new(pseudo_ptr::allocate())T(...)
 	* problem: constructors are now ambiguous, would be nice if we could name it
	*/
	//explicit pseudo_ptr(T* p):index(((void*)p-get_pool()->p.v)/sizeof(T)){}
	template<typename S,typename OTHER_STORE> pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,true>& p):index(p.index){
		//we can make it safe but should be optional 
		assert(p.pool_index==get_pool());
	} 
	//static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->allocate());}
	static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->template allocate_t<T>());}
	//equivalent to return new T(s), could use variadic template
	template<typename S> static pseudo_ptr construct(S s){
		pseudo_ptr p=allocate();
		new(p)T(s);//in-place constructor
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
	operator void*() {return index ? get_typed_v()+index:0;}
	bool operator!()const{return !index;}
};
/*
 *	can point to sub- and super-class of T
 */
template<
	typename T,
	typename STORE//should be made a template so we could compare stores
>
struct pseudo_ptr<T,STORE,true>{
	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef T& reference;
	typedef unsigned char INDEX;
	POOL_INDEX pool_index;
	INDEX index;
	static POOL_INDEX get_pool(){return POOL_INDEX::get<T,STORE>();}
	explicit pseudo_ptr(INDEX index):index(index),pool_index(get_pool()){
		print();
	}
	void print(){
		cerr<<"{"<<(unsigned int)pool_index.index<<","<<(unsigned int)index<<"}"<<endl;
	}
	/*
 	* dangerous, be very careful with this
 	* 	should be used like this:
 	* 	pseudo_ptr a(new(pseudo_ptr::allocate())T(...)
	*/
	//explicit pseudo_ptr(T* p):index(((void)p-get_pool()->p.v)/sizeof(T)),pool_index(get_pool()){}
	//static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->allocate());}
	static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->template allocate_t<T>());}
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
	template<typename A,typename B,typename C,typename D> static pseudo_ptr construct(A a,B b,C c,D d){
		pseudo_ptr p=allocate();
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
	template<typename S,typename OTHER_STORE> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,false>& p):pool_index(pseudo_ptr<S,OTHER_STORE,false>::get_pool()),index(p.index){}
	template<typename S,typename OTHER_STORE> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,true>& p):pool_index(p.pool_index),index(p.index){
		//we need a template that detects if S is not derived from T at compile time
	}
	value_type* operator->()const{return (value_type*)pool_index->get(index);}
	reference operator*()const{return *(value_type*)pool_index->get(index);}
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
	//
	operator bool() const{return index;}
	//not very efficient
	operator void*() {return index ? pool_index->get(index) : 0;}
	//better
	bool operator!()const{return !index;}
};
template<typename T,typename STORE,bool POLYMORPHISM> pseudo_ptr<T,STORE,POLYMORPHISM> operator+(const pseudo_ptr<T,STORE,POLYMORPHISM>& a,size_t s){
	pseudo_ptr<T,STORE,POLYMORPHISM> tmp=a;
	return tmp+=s;
}
template<typename T,typename STORE,bool POLYMORPHISM> pseudo_ptr<T,STORE,POLYMORPHISM> operator-(const pseudo_ptr<T,STORE,POLYMORPHISM>& a,size_t s){
	pseudo_ptr<T,STORE,POLYMORPHISM> tmp=a;
	return tmp-=s;
}
//should make sure types are the same, make sure no casting takes place, could also compare pool_index
template<typename T,typename STORE,bool POLYMORPHISM> ptrdiff_t operator-(const pseudo_ptr<T,STORE,POLYMORPHISM>& a,const pseudo_ptr<T,STORE,POLYMORPHISM>& b){
	return a.index-b.index;
}

#endif

