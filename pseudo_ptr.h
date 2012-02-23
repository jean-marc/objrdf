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
 * 	not all classes will be instantiated, would be nice to have a way to detect those so we don't
 * 	waste a pool
 *
 */
template<bool A> struct _assert_;
template<> struct _assert_<true>{};
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
 	*/ 
	struct info{
		typedef unsigned short FIELD;
		FIELD next;
		union{
			FIELD n_cells;
			FIELD cell_size;
		};
	};
	//template<typename T> struct helper:T{
	template<typename T> struct helper{
		char t[sizeof(T)];
		inline info::FIELD& next(){return static_cast<info*>((void*)this)->next;}
		inline info::FIELD& n_cells(){return static_cast<info*>((void*)this)->n_cells;}
		inline info::FIELD& cell_size(){return static_cast<info*>((void*)this)->cell_size;}
	};
	param p;
	typedef void(*DESTRUCTOR)(void*);
	const DESTRUCTOR destructor; 
	/*
 	*	we need a block of free memory to store function pointers 
 	*/
	pool(param p,DESTRUCTOR destructor):p(p),destructor(destructor){
		info& first=*static_cast<info*>(p.v);
		info& second=*static_cast<info*>(p.v+p.cell_size);
		/*
		if(first.next==0){
			first.next=1;
			first.n_cells=0;
		}
		*/
		if(first.n_cells==0){
			first.next=1;
			second.next=0;
			second.cell_size=p.n-1;	
		}
		cerr<<"new pool "<<this<<" "<<first.n_cells<<" out of "<<p.n<<" cells used"<<endl;
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
		auto i=v[0].next();
		v[0].next()=v[i].next() ? v[i].next() : v[0].next()+1;
		v[0].n_cells()++;
		cerr<<"allocate cell at index "<<i<<"{"<<&v[i]<<"} in pool "<<this<<endl;
		return i;	
	}
	//contiguous cells for vector<>
	template<typename T> size_t allocate_t(size_t n){
		//typedef _assert_<sizeof(T)>=sizeof(info)> T; VERY strange behaviour, to be analyzed
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
		v[0].n_cells()-=n;//why does it matter?
	}
	//we can choose the pointer value
	//do not mix allocate_at() with allocate() !
	size_t allocate_at(size_t i){
		info& first=*static_cast<info*>(p.v);
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
		//it does not work, not really in order
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
	template<typename T,typename STORE> static pool* get_instance(){
		static pool* p=new pool(STORE::template go<T>(),0);
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
		info::FIELD index;//address current cell
		info::FIELD n;//number of cells to visit
		info::FIELD next_free;//address next free cell
		iterator(helper<T>* v,info::FIELD n):v(v),index(1),n(n),next_free(v[0].next()){
			while(index==next_free&&n){
				++index;
				next_free=v[next_free].next();
			}
		}		
		bool operator==(const iterator& p)const{return n==p.n;}
		bool operator!=(const iterator& p)const{return n!=p.n;}
		bool operator<(const iterator& p)const{return n>p.n;}
		iterator& operator++(){
			++index;
			while(index==next_free&&n){
				++index;
				next_free=v[next_free].next();
			}
			--n;
			return *this;
		}
		//we need a pseudo_ptr!
		T* operator->(){return (T*)(void*)&v[index];}		
		T& operator*(){return (T&)(void*)&v[index];}
	};
	template<typename T> iterator<T> begin(){return iterator<T>(static_cast<helper<T>*>(p.v),static_cast<helper<T>*>(p.v)[0].n_cells());}
	template<typename T> iterator<T> end(){return iterator<T>(static_cast<helper<T>*>(p.v),0);}
};
template<size_t SIZE=256> struct free_store{
	template<typename T> static pool::param go(){
		char* v=new char[SIZE*sizeof(T)];
		memset(v,0,SIZE*sizeof(T));
		return pool::param(v,SIZE,sizeof(T));
	}
};
template<typename T> struct name;//{static const string get(){return "none";}};
template<size_t SIZE> struct persistent_store{
	//use memory map
	//how to add support for read-only file?
	template<typename T> static pool::param go(){
		/*
 		* should add ascii file with version information, that number could come from git
		* to build compatible executable
		*/ 
		string filename="db/"+name<typename T::SELF>::get();
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
//we need a const version of this
template<
	typename T,
	//maybe store SHOULD not be a template parameter, maybe a constructor parameter instead
	typename STORE=free_store<>,//free store or persistent
	bool POLYMORPHISM=false//does not support derived types
> struct pseudo_ptr;
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
		//pseudo_ptr p=pseudo_ptr(pool::get_instance<pool,STORE>()->allocate());
		pseudo_ptr p=pseudo_ptr(pool::get_instance<pool,STORE>()->template allocate_t<pool>());
		new(p)pool(s,d);
		return p;
	}
	template<typename T> static void destructor(void* s){static_cast<T*>(s)->~T();}
	template<typename T,typename OTHER_STORE> static pseudo_ptr get(){
		static pseudo_ptr p=construct(OTHER_STORE::template go<T>(),destructor<T>);
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
	typename T,
	typename STORE
> struct pseudo_ptr<T,STORE,false>{
	/*
 	* we need to define a unique type to avoid ambiguity between constructors
 	* 	pseudo_ptr(INDEX index)
 	* 	pseudo_ptr(T* p)
 	* e.g:
 	* 	struct unique{
 	* 		INDEX i;
 	* 	};
 	*/
	typedef pseudo_ptr<pool,free_store<>,false> POOL_INDEX;
	typedef unsigned char INDEX;

	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef T& reference;
	static POOL_INDEX get_pool(){return POOL_INDEX::get<T,STORE>();}
	INDEX index;
	//explicit pseudo_ptr(INDEX index):index(index){}
	pseudo_ptr(INDEX index):index(index){}
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
	pseudo_ptr& operator=(const pseudo_ptr& p){
		index=p.index;
		return *this;
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
	operator value_type*() {return index ? get_typed_v()+index:0;}
	bool operator!()const{return !index;}
};
/*
 *	can point to sub- and super-class of T
 */
template<
	typename T,
	typename STORE
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
		//print();
	}
	void print(ostream& os){
		os<<"{"<<(unsigned int)pool_index.index<<","<<(unsigned int)index<<"}"<<endl;
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
	template<typename S,typename OTHER_STORE> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,false>& p):pool_index(pseudo_ptr<S,OTHER_STORE,false>::get_pool()),index(p.index){}
	template<typename S,typename OTHER_STORE> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,true>& p):pool_index(p.pool_index),index(p.index){
		//we need a template that detects if S is not derived from T at compile time
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
template<typename T,typename STORE,bool POLYMORPHISM> pseudo_ptr<T,STORE,POLYMORPHISM> operator-(const pseudo_ptr<T,STORE,POLYMORPHISM>& a,size_t s){
	pseudo_ptr<T,STORE,POLYMORPHISM> tmp=a;
	return tmp-=s;
}
//should make sure types are the same, make sure no casting takes place, could also compare pool_index
template<typename T,typename STORE,bool POLYMORPHISM> ptrdiff_t operator-(const pseudo_ptr<T,STORE,POLYMORPHISM>& a,const pseudo_ptr<T,STORE,POLYMORPHISM>& b){
	return a.index-b.index;
}

#endif

