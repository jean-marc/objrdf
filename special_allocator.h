#ifndef SPECIAL_ALLOCATOR_H
#define SPECIAL_ALLOCATOR_H
/*
 *	simplest allocator that works
 *
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <typeinfo>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <functional>
#include "ifthenelse.hpp"
using namespace  std;
template<typename INDEX> struct _info{
	INDEX size;//size of current range in multiple of sizeof(info)
	INDEX next;//index of next available range 0 means no more range
};
template<> struct _info<void>{
};
template<
	typename _INDEX_,
	typename _PAYLOAD_,
	typename _ALLOCATOR_,//for struct pool{}
	typename _RAW_ALLOCATOR_=std::allocator<char>,
	typename _MANAGEMENT_=void,
	typename _INFO_=_info<_INDEX_>
> struct cell{
	typedef _INDEX_ INDEX;
	typedef _PAYLOAD_ PAYLOAD;
	typedef _ALLOCATOR_ ALLOCATOR;
	typedef _RAW_ALLOCATOR_ RAW_ALLOCATOR;
	typedef _MANAGEMENT_ MANAGEMENT;
	typedef _INFO_ INFO;
	MANAGEMENT management; //store metadata, not visible to payload, can also be used to detect unauthorized access, maybe would make more sense as first member
	union{
		INFO info;
		char payload[sizeof(PAYLOAD)];//will affect data alignment, we have to investigate the effect of misalignment (extra CPU + exception for special operations)
		//PAYLOAD payload;
	}body;
	typedef cell<INDEX,PAYLOAD,ALLOCATOR,_RAW_ALLOCATOR_,MANAGEMENT,_info<void>> HELPER;	
	enum{MANAGED=true};
	enum{OPTIMIZATION=false};
	enum{FACTOR=1};
	enum{MAX_SIZE=1L<<3*sizeof(INDEX)};
	static void post_allocate(cell* begin,cell* end){
		for(cell* i=begin;i<end;++i) i->management=0xf;	
	}
	static void post_deallocate(cell* begin,cell* end){
		for(cell* i=begin;i<end;++i) i->management=0x0;	
	}
};
template<
	typename _INDEX_,
	typename _PAYLOAD_,
	typename _ALLOCATOR_,//for struct pool{}
	typename _RAW_ALLOCATOR_,
	typename _INFO_
> struct cell<_INDEX_,_PAYLOAD_,_ALLOCATOR_,_RAW_ALLOCATOR_,void,_INFO_>{
	typedef _INDEX_ INDEX;
	typedef _PAYLOAD_ PAYLOAD;
	typedef _ALLOCATOR_ ALLOCATOR;
	typedef void MANAGEMENT;
	typedef _INFO_ INFO;
	typedef _RAW_ALLOCATOR_ RAW_ALLOCATOR;
	union{
		INFO info;
		char payload[sizeof(PAYLOAD)];
	}body;
	typedef cell<INDEX,PAYLOAD,ALLOCATOR,_RAW_ALLOCATOR_,void,_info<void>> HELPER;	
	enum{MANAGED=false};
	enum{OPTIMIZATION=(sizeof(INFO)>sizeof(PAYLOAD))&&(sizeof(INFO)%sizeof(PAYLOAD)==0)};
	enum{FACTOR=OPTIMIZATION ? sizeof(INFO)/sizeof(PAYLOAD) : 1};
	enum{MAX_SIZE=1L<<3*sizeof(INDEX)};
	static void post_allocate(cell*,cell*){}
	static void post_deallocate(cell*,cell*){}
};
struct pool{
	template<typename CELL> struct ptr{
		typedef typename CELL::INDEX INDEX;
		typedef typename CELL::PAYLOAD VALUE_TYPE;
		typedef typename IfThenElse<CELL::OPTIMIZATION,typename CELL::HELPER,CELL>::ResultT PAYLOAD_CELL;
		INDEX index;
		typedef ptr pointer;
		typedef VALUE_TYPE value_type;
		typedef VALUE_TYPE& reference;
		typedef ptrdiff_t difference_type;
		typedef random_access_iterator_tag iterator_category;
		ptr(INDEX index=0):index(index){}
		//value_type* operator->()const{return &pool::get_pool<CELL>()->get<CELL>(index);}
		value_type* operator->()const{return &pool::get_pool<CELL>()->get<PAYLOAD_CELL>(index);}
		//reference operator*()const{return pool::get_pool<CELL>()->get<CELL>(index);}
		reference operator*()const{return pool::get_pool<CELL>()->get<PAYLOAD_CELL>(index);}
		ptr& operator+=(INDEX s){index+=s;return *this;}
		ptr& operator++(){++index;return *this;}
		ptr& operator-=(INDEX s){index-=s;return *this;}
		ptr& operator--(){--index;return *this;}
		bool operator==(const ptr& a)const{return index==a.index;}
		bool operator!=(const ptr& a)const{return index!=a.index;}
		operator value_type*() {return index ? operator->():0;}
		//bool operator!()const{return !index;}
		//operator bool() const{return index;}
		//special operators to access cell instead of payload, will be necessary once we implement reference counting
	};
	/*
 	* 	iterators to visit pool in a generic way, we need to skip non-allocated ranges
 	*	end iterator is tricky because we don't know where the last cell is, because they are
 	*	allocated at random positions, we could keep track of last cell but it would take extra work
 	*	at each allocation/deallocation
 	*	it is very similar to ptr_d, maybe it could inherit from it, the problem is to access the MANAGEMENT member
 	*	to test if the cell is being used
 	*/ 
	template<typename CELL,bool MANAGED=CELL::MANAGED> struct cell_iterator;
	template<typename CELL> struct cell_iterator<CELL,true>{
		typedef typename CELL::INDEX INDEX;
		typedef CELL value_type;
		typedef CELL& reference;
		typedef ptrdiff_t difference_type;
		typedef forward_iterator_tag iterator_category;
		INDEX index;
		INDEX n;//number of cells to visit
		cell_iterator(INDEX index=0,INDEX n=pool::get_pool<CELL>()->get_cells<CELL>()[0].body.info.size):index(index),n(n){
			while(!(*this)->management&&n) ++index;
		}
		cell_iterator& operator++(){
			++index;
			while(!(*this)->management&&n) ++index;
			--n;
			return *this;
		}
		value_type* operator->()const{return &pool::get_pool<CELL>()->get_cells<CELL>()[index];}
		reference operator*()const{return pool::get_pool<CELL>()->get_cells<CELL>()[index];}
		bool operator==(const cell_iterator& a)const{return n==a.n;}
		bool operator!=(const cell_iterator& a)const{return n!=a.n;}
	};
	/*
 	*	allocate memory on file
 	*/ 
	struct mmap_allocator_impl{
		int fd;
		bool writable;
		void* v;
		size_t file_size;
		enum{PAGE_SIZE=4096};
		mmap_allocator_impl(string filename):writable(true){
			cerr<<"opening file `"<<filename<<"' O_RDWR"<<endl;
			fd = open(filename.c_str(), O_RDWR | O_CREAT/* | O_TRUNC*/, (mode_t)0600);
			if(fd ==-1){
				cerr<<"opening file `"<<filename<<"' O_RDONLY"<<endl;
				fd = open(filename.c_str(), O_RDONLY/* | O_TRUNC*/, (mode_t)0600);
				writable=false;
			}
			if (fd == -1) {
				cerr<<"\nError opening file `"<<filename<<"' for writing or reading"<<endl;
				exit(EXIT_FAILURE);
			}
			//set the size
			struct stat s;
			int r=fstat(fd,&s);
			if(r==-1){
				cerr<<"\ncould not stat file `"<<filename<<"'"<<endl;
				exit(EXIT_FAILURE);
			}
			file_size=0;
			if(s.st_size==0){
				//new file
				file_size=PAGE_SIZE;
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
			}else{
				file_size=s.st_size;
			}
			v = writable ? mmap((void*)NULL,file_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0) : mmap((void*)NULL,file_size,PROT_READ,MAP_SHARED,fd,0);
			cerr<<"new mapping at "<<v<<" size:"<<file_size<<endl;
			if (v == MAP_FAILED) {
				close(fd);
				cerr<<"Error mmapping the file"<<endl;
				exit(EXIT_FAILURE);
			}
		}
		//it is not a proper allocator
		char* allocate(size_t n){
			if(n>file_size){
				size_t _file_size=max<size_t>(ceil((double)(n)/PAGE_SIZE),1)*PAGE_SIZE;
				int result = lseek(fd,_file_size-1, SEEK_SET);
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
				void* _v=(char*)mremap(v,file_size,_file_size,MAP_SHARED,MREMAP_MAYMOVE);
				if (_v == MAP_FAILED) {
					close(fd);
					cerr<<"Error mremapping the file"<<endl;
					exit(EXIT_FAILURE);
				}
				v=_v;
				cerr<<"new mapping at "<<v<<" size:"<<_file_size<<endl;
				file_size=_file_size;
			}
			return (char*)v;
		}
	};
	template<typename T> struct mmap_allocator{
		typedef char* pointer;
		static mmap_allocator_impl* get_impl(){
			static mmap_allocator_impl* a=new mmap_allocator_impl(string("db/")+typeid(T).name());
			return a;
		}
		//we know that there will be only one range used at any given time
		pointer allocate(size_t n){
			return get_impl()->allocate(n);
		}
		void deallocate(pointer p,size_t n){

		}
	};
	template<typename _CELL_> struct allocator{
		typedef _CELL_ CELL;
		typedef ptr<CELL> pointer;
		typedef typename pointer::value_type value_type;
		typedef typename pointer::reference reference;
		//we have to decide if const pointers use different pool
		typedef	pointer const_pointer; 
		typedef typename const_pointer::reference const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;
		allocator(){}
		~allocator(){}
		//size_type max_size() const throw(){return 1L<<3*sizeof(typename CELL::INDEX);}
		size_type max_size() const throw(){return CELL::MAX_SIZE;}
		pointer allocate(size_type n){return pointer(pool::get_pool<CELL>()->template allocate<CELL>(max<size_t>(n/CELL::FACTOR,1))*CELL::FACTOR);}
		void deallocate(pointer p,size_type n){pool::get_pool<CELL>()->template deallocate<CELL>(p.index/CELL::FACTOR,max<size_t>(n/CELL::FACTOR,1));}
		template<class U> struct rebind{
			typedef allocator other;//should not be used
		};
		template<typename... Args> void construct(pointer p,Args... args){
			cerr<<"construct at "<<(int)p.index<<"("<<(void*)p.operator->()<<")"<<endl;
			new(p) value_type(args...);
		}
		void destroy(pointer p){
			cerr<<"destroy at "<<(int)p.index<<endl;
			p->~value_type();
		}
		/*
 		* this might cause problem, is the pointer still valid? it should be because when using vector a whole block is allocated
 		*/
		template<typename... Args> void construct(value_type* p,Args... args){
			cerr<<"construct at "<<(void*)p<<endl;
			new(p) value_type(args...);
		}
		void destroy(value_type* p){
			cerr<<"destroy at "<<(void*)p<<endl;
			p->~value_type();
		}
		//typed iterator
		typedef cell_iterator<CELL> iterator;
		iterator begin(){return iterator(1);}
		iterator end(){return iterator(1,0);}
	};
	//let's store pools in a pool...maximum 255 pools for now
	//typedef cell<uint8_t,pool,std::allocator<pool>,std::allocator<char>,char> POOL_CELL;
	typedef cell<uint8_t,pool,std::allocator<pool>,mmap_allocator<pool>,char> POOL_CELL;
	typedef allocator<POOL_CELL> POOL_ALLOCATOR;
	/*
 	*	what if the pool itself is persisted, the pointer will be wrong
 	*/
	char* buffer;
	size_t buffer_size;
	size_t cell_size;
	size_t payload_offset;
	size_t type_id;//
	/*const*/bool iterable;
	//we might also need the offset of the management data for iteration, or management should be put first
	pool(){}
	pool(char* buffer,size_t buffer_size,size_t cell_size,size_t payload_offset,size_t type_id):buffer(buffer),buffer_size(buffer_size),cell_size(cell_size),payload_offset(payload_offset),type_id(type_id){}
	/*template<typename CELL> static pool* create(char* buffer,size_t buffer_size){
		cerr<<"new pool"<<endl;
		//prepare structure if needed
		CELL *c=(CELL*)buffer;
		c[0].body.info.size=0;//new pool
		c[0].body.info.next=1;
		c[1].body.info.size=buffer_size/sizeof(CELL)-1;
		c[1].body.info.next=0;
		return new pool(buffer,buffer_size,sizeof(CELL));
	}
	*/
	template<typename CELL> static typename CELL::ALLOCATOR::pointer create(){
		std::hash<std::string> str_hash;
		cerr<<typeid(typename CELL::PAYLOAD).name()<<"\t"<<str_hash(typeid(typename CELL::PAYLOAD).name())<<endl;
		typename CELL::RAW_ALLOCATOR raw;
		size_t buffer_size=64*sizeof(CELL);
		//we might not have to allocate if it was persisted
		auto buffer=raw.allocate(buffer_size);//what about allocating CELL's instead of char?, it would have the advantage of aligning the data
		CELL *c=(CELL*)buffer;
		//check if the pool already exist
		if(c[0].body.info.next==0){
			c[0].body.info.size=0;//new pool
			c[0].body.info.next=1;
			c[1].body.info.size=buffer_size/sizeof(CELL)-1;
			c[1].body.info.next=0;
		}
		typename CELL::ALLOCATOR a;
		//test if already exist?, we must be able to iterate
		//auto i=find_if(a.begin(),a.end(),test(id))
		auto p=a.allocate(1);
		a.construct(p,buffer,buffer_size,sizeof(CELL),offsetof(CELL,body),str_hash(typeid(typename CELL::PAYLOAD).name()));
		return p;
	}
	template<typename CELL> CELL* get_cells(){
		//what if buffer has been persisted? it is probably no longer valid
		//we could do the magic here, 
		return (CELL*)buffer;
	}
	//size in cells
	size_t size() const{return buffer_size/cell_size;}
	template<typename CELL> void status(){
		CELL *c=(CELL*)buffer;
		cerr<<"pool "<<c[0].body.info.size<<"/"<<buffer_size/sizeof(CELL)<<" cell(s) "<<endl;
	}
	template<typename CELL> typename CELL::INDEX allocate(size_t n){
		CELL *c=(CELL*)buffer;
		typedef typename CELL::INDEX INDEX;
		INDEX prev=0,current=c[prev].body.info.next;
		while(current && c[current].body.info.size<n){
			cout<<"\t"<<(int)current<<endl;
			prev=current;
			current=c[prev].body.info.next;
		}
		if(current){ //we have found enough contiguous cells
			if(c[current].body.info.size==n)
				c[prev].body.info.next=c[current].body.info.next;
			else{	//create new group
				INDEX i=current+n;
				c[prev].body.info.next=i;
				c[i].body.info.size=c[current].body.info.size-n;
				c[i].body.info.next=c[current].body.info.next;
			}
			//shall we clean up this cell???
			//we could although it is not necessary, an allocator does not initialize the memory
			c[current].body.info.size=0;
			c[current].body.info.next=0;
			
			c[0].body.info.size+=n;//update total number of cells in use
			CELL::post_allocate(c+current,c+current+n);
		}else{
			//let's see how many cells we need to fulfill demand
			//need to check the maximum size addressable
			if(buffer_size/cell_size+n>CELL::MAX_SIZE) throw std::bad_alloc();
			size_t new_buffer_size=max<size_t>(buffer_size+n*cell_size,2*buffer_size);
			cerr<<this<<" increasing pool size from "<<buffer_size<<" to "<<new_buffer_size<<endl;
			//at this stage we may decide to increase the original buffer
			//we need to create new buffer, copy in the old one
			typename CELL::RAW_ALLOCATOR raw;
			auto new_buffer=raw.allocate(new_buffer_size);
			//the next 3 stages must be avoided when dealing with mmap
			memcpy(new_buffer,buffer,buffer_size);
			memset(new_buffer+buffer_size,0,new_buffer_size-buffer_size);
			raw.deallocate(buffer,buffer_size);	
			buffer=new_buffer;
			CELL *c=(CELL*)buffer;
			//add the new range
			c[buffer_size/cell_size].body.info.size=(new_buffer_size-buffer_size)/cell_size;
			c[buffer_size/cell_size].body.info.next=c[0].body.info.next;
			c[0].body.info.next=buffer_size/cell_size;	
			buffer_size=new_buffer_size;
			return allocate<CELL>(n);
		}
		cerr<<this<<" allocate "<<n<<" cell(s) at index "<<(int)current<<endl;
		return current;
	}	
	template<typename CELL> void deallocate(typename CELL::INDEX index,size_t n){
		cerr<<this<<" deallocate "<<n<<" cell(s) at index "<<(int)index<<endl;
		CELL *c=(CELL*)buffer;
		typedef typename CELL::INDEX INDEX;
		c[index].body.info.size=n;
		c[index].body.info.next=c[0].body.info.next;
		c[0].body.info.next=index;
		c[0].body.info.size-=n;//update total number of cells in use
		CELL::post_deallocate(c+index,c+index+n);
	}
	template<typename CELL> typename CELL::PAYLOAD& get(typename CELL::INDEX index){
		//cerr<<this<<" dereference cell at index "<<(int)index<<endl;
		CELL *c=(CELL*)buffer;
		return (typename CELL::PAYLOAD&)c[index].body.payload;	
	}
	template<typename CELL> typename CELL::PAYLOAD& get_cast(typename CELL::INDEX index){
		//we don't know the actual type, only cell_size and padding size
		//static_assert(sizeof(typename CELL::PAYLOAD)>cell_size,"wrong payload");
		return (typename CELL::PAYLOAD&)(*(buffer+index*cell_size+payload_offset));
	}
	template<typename CELL> static typename CELL::ALLOCATOR::pointer get_pool(){
		static auto p=create<CELL>();
		return p;
	}
};
//operators
template<typename CELL> pool::ptr<CELL> operator+(const pool::ptr<CELL>& a,size_t s){
	pool::ptr<CELL> tmp=a;
	return tmp+=s;
}
template<typename CELL> pool::ptr<CELL> operator-(const pool::ptr<CELL>& a,size_t s){
	pool::ptr<CELL> tmp=a;
	return tmp-=s;
}
template<typename CELL> ptrdiff_t operator-(const pool::ptr<CELL>& a,const pool::ptr<CELL>& b){
	return a.index-b.index;
}
template<typename CELL> struct ptr_d{
	typedef typename CELL::INDEX INDEX;
	typedef typename CELL::PAYLOAD VALUE_TYPE;
	pool::POOL_ALLOCATOR::pointer pool_ptr;
	INDEX index;
	typedef ptr_d pointer;
	typedef VALUE_TYPE value_type;
	typedef VALUE_TYPE& reference;
	typedef ptrdiff_t difference_type;
	typedef random_access_iterator_tag iterator_category;
	ptr_d(pool::POOL_ALLOCATOR::pointer pool_ptr=0,INDEX index=0):pool_ptr(pool_ptr),index(index){}
	template<typename OTHER_CELL> ptr_d(const pool::ptr<OTHER_CELL>& p):pool_ptr(pool::get_pool<OTHER_CELL>()),index(p.index){
		VALUE_TYPE* a;
		typename OTHER_CELL::PAYLOAD* b;
		a=b;
	}
	value_type* operator->()const{return &pool_ptr->get_cast<CELL>(index);}
	reference operator*()const{return pool_ptr->get_cast<CELL>(index);}
	ptr_d& operator+=(INDEX s){index+=s;return *this;}
	ptr_d& operator++(){++index;return *this;}
	ptr_d& operator-=(INDEX s){index-=s;return *this;}
	ptr_d& operator--(){--index;return *this;}
	bool operator==(const ptr_d& a)const{return pool_ptr==a.pool_ptr&&index==a.index;}
	bool operator!=(const ptr_d& a)const{return pool_ptr!=a.pool_ptr||index!=a.index;}
	operator value_type*() {return index ? operator->():0;}
};
#endif
