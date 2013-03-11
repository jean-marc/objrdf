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
#include <math.h>
#include <set>
#include <assert.h>
#include <fstream>
#include <algorithm>
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
 * 	adjustable pool size, the store can be resized as long as S<2^n where n number of bits in pointer,
 * 	how do we resize mmap? mremap()
 *
 *	typedef pseudo_ptr_pool<pool,persistent_store,uint8_t> POOL_PTR; //saved to `db/pool'  
 *
 *
 *	 --- 	         _______[rdf::type		]								*******************
 *	| 0 |           /   ____[objrdf::superClassOf	]								*  MATCHED INDEX  *
 *	|---|          /   /												*******************		
 *	| 1 |-->[ 0 | 1 | 2 ...] pseudo_ptr<rdf::Property>     ________[base_resource::nil	]      __________[rdf::Property::get_class()	]		
 *	|---|                                                 / 				      /   _______[rdfs::Resource::get_class()	]
 *	| 2 |----------------------------------------->[ 0 | 1 | 2 ...] pseudo_ptr<rdfs::Resource>   /   /   ____[rdfs::Class::get_class()	]
 *	|---|   										    /   /   /		
 *	| 3 |------------------------------------------------------------------------------->[ 0 | 1 | 2 | 3 ...] pseudo_ptr<rdfs::Class> 
 *	|---|
 *
 *
 *	typedef pseudo_ptr_pool<pool_array,persistent_store,uint8_t> POOL_PTR_ARRAY; //saved to `db/pool_array'  
 *	 --- 
 *	| 0 |
 *	|---|   
 *	| 1 |-->[ 0 | 1 | 2 ...] pseudo_ptr_array<rdfs::subClassOf>
 *	|---|  
 *	| 2 |-->[ 0 | 1 | 2 ...] pseudo_ptr_array<objrdf::superClassOf>
 *	|---| 
 *
 *
 * 	all of this implements a http://en.cppreference.com/w/cpp/memory/allocator 
 */

/*
 * http://en.wikipedia.org/wiki/Jenkins_hash_function
 */

uint32_t jenkins_one_at_a_time_hash(const char *key, size_t len);
class param{
public:
	void* v;
	size_t n;
	/*const*/ size_t cell_size;
	const size_t max_size;//based on pointer size
	const bool writable;
	/*
	*	when not writable the allocator should always return 0, and const pointers
	*	should be used
	*/ 
	param(void* v,size_t n,const size_t cell_size,const size_t max_size,const bool writable=true);
	virtual void resize_impl(size_t n)=0;
	//should be smarter and clip to maximum value
	void resize(size_t n);
};

struct pool{
	/*
 	*	can we add the reference counting here? 
 	*/
	/*
 	*	careful with the size of this structure as it must fit within a T object
 	*	it will cause very strange behaviour if it does not fit!
 	*	the problem is that the size depends on the pointer size
 	*/ 
	struct info{
		//we are only using one field at a time except for the first cell
		typedef unsigned short FIELD;
		FIELD next;
		FIELD n_cells;
		info();//:next(0),n_cells(0){}
	};
	template<typename T> struct helper{
		static_assert(sizeof(T)>=sizeof(info),"T smaller than struct info{}");
		char t[sizeof(T)];
		inline info::FIELD& next(){return static_cast<info*>((void*)this)->next;}
		inline info::FIELD& n_cells(){return static_cast<info*>((void*)this)->n_cells;}
	};
	typedef void(*DESTRUCTOR)(void*);
	const DESTRUCTOR destructor; 
	typedef pool SELF;//to get 
	/*
 	* 	more than one pool per type possible
 	*/
	const size_t type_id;
	struct match_type_id{
		const size_t type_id;
		match_type_id(const size_t type_id);
		template<typename T> bool operator()(const T& t)const{
			cerr<<type_id<<"=="<<t->type_id<<endl;
			return t->type_id==type_id;
		}
	};
	param& p;
	pool(param& p,DESTRUCTOR destructor,size_t type_id);
	uint32_t hash() const;
	//never called
	virtual ~pool();
	//we still need this 
	size_t allocate();
	//typed version is safer and cleaner
	template<typename T> size_t allocate_t(){
		auto v=static_cast<helper<T>*>(p.v);
		cerr<<v[0].n_cells()<<" "<<p.n-1<<" "<<v[0].next()<<" "<<v[v[0].next()].next()<<endl;
		if(v[0].n_cells()==p.n-1){
			//resize the pool, uses strategy similar to vector<>: keep doubling
			cerr<<"increasing pool size from "<<p.n<<" to "<<2*p.n<<" cells "<<this<<endl;
			size_t original_size=p.n;
			cerr<<"hash:"<<hash()<<endl;
			p.resize(2*p.n);
			cerr<<"new pool size: "<<p.n<<endl;
			cerr<<"hash:"<<jenkins_one_at_a_time_hash((char*)p.v,original_size)<<endl;
			//update v
			v=static_cast<helper<T>*>(p.v);
		}
		auto i=v[0].next();
		v[0].next()=v[i].next() ? v[i].next() : v[0].next()+1;
		v[0].n_cells()++;
		cerr<<"allocate cell at index "<<i<<hex<<"\t"<<(i*p.cell_size)<<dec<<"\t{"<<&v[i]<<"} in pool "<<this<<" "<<v[0].next()<<endl;
		return i;	
	}
	//we can choose the pointer value
	//do not mix allocate_at() with allocate() !
	size_t allocate_at(size_t i);
	void deallocate(size_t i);
	template<typename T> void deallocate_t(size_t i){
		auto v=static_cast<helper<T>*>(p.v);
		cerr<<"deallocate cell at index "<<i<<" in pool "<<this<<endl;
		//keep linked list of empty cells ordered for iterator: useless!!
		if(i<v[0].next()){
			v[i].next()=v[0].next();
			v[0].next()=i;
		}else{
			v[i].next()=v[v[0].next()].next();
			v[v[0].next()].next()=i;
		}
		v[0].n_cells()--;
	}
	void* get(size_t i);
	//could use function pointer:allocate, deallocate,...
	size_t get_size()const;
	//we need to pass information about the pointer to the store
	template<typename P> static pool* get_instance(){
		typedef typename P::STORE STORE;
		static pool* p=new pool(*STORE::template go<P>(),0,0);
		return p;
	}
	/*
 	*	typed iterator: go through all the cells and skips free ones
 	*	how do we know if cell is used?	we only know which ones are free
 	*	forward iterator
 	*	not necessary when we know it is dense (no deallocation took place)
 	*	we could sort gaps when creating iterator
 	*	better approach is to test each cell with custom test function
 	*	the problem is that an empty cell still needs an iterator to the next empty cell.
 	*	only one bit would be enough
 	*/ 
	static bool TEST(const pool& p);
	//P must be a pseudo_ptr	
	template<typename P> struct iterator{
		typedef P value_type;	
		typedef P& reference;	
		typedef P* pointer;
		typedef forward_iterator_tag iterator_category;
		typedef size_t difference_type;
		typedef typename P::value_type S;
		S* v;
		info::FIELD index;//address current cell
		info::FIELD n;//number of cells to visit
		iterator(S* v,info::FIELD n):v(v),index(1),n(n){
			//why does it have to be a static function???
			while(!S::TEST(v[index])&&n) ++index;
		}		
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
		value_type operator*(){return P(index);}
	};
	template<typename T> iterator<T> begin(){
		assert(sizeof(typename T::value_type)==p.cell_size);//better: compare hashes
		return iterator<T>(static_cast<typename T::value_type*>(p.v),get_size());
	}
	template<typename T> iterator<T> end(){return iterator<T>(static_cast<typename T::value_type*>(p.v),0);}
};
/*
 *	to be used with vector allocator
 */
struct pool_array{
	struct info{
		//we are only using one field at a time except for the first cell
		typedef unsigned short FIELD;
		//typedef unsigned char FIELD;
		FIELD next;
		union{
			FIELD n_cells;
			FIELD cell_size;
		};
	};
	/*
 	*	it should be possible to inflate the size of the payload and then request less memory
 	*
 	*/ 
	template<typename T> struct helper{
		static_assert(sizeof(T)>=sizeof(info),"T smaller than struct info{}");
		char t[sizeof(T)];
		inline info::FIELD& next(){return static_cast<info*>((void*)this)->next;}
		inline info::FIELD& n_cells(){return static_cast<info*>((void*)this)->n_cells;}
		inline info::FIELD& cell_size(){return static_cast<info*>((void*)this)->cell_size;}
	};
	param& p;
	typedef void(*DESTRUCTOR)(void*);
	const DESTRUCTOR destructor; 
	typedef pool_array SELF;//to get 
	/*
 	* 	more than one pool per type possible
 	*/
	const size_t type_id;
	pool_array(param& p,DESTRUCTOR destructor,size_t type_id);
	static bool TEST(const pool_array& p);
	uint32_t hash() const;
	//never called
	virtual ~pool_array();
	template<typename T> size_t allocate_t(size_t n){
		//T might be smaller than info
		enum{TEST=sizeof(T)<sizeof(info)};
		typedef typename IfThenElse<TEST,info,T>::ResultT TMP;
		n=ceil(float(n*sizeof(T))/sizeof(TMP));
		auto v=static_cast<helper<TMP>*>(p.v);
		size_t i=help_allocate_t<TMP>(v[0].next(),n);
		if(i==0){
			cerr<<"pool_array increasing pool size from "<<p.n<<" to "<<2*p.n<<" cells "<<this<<endl;
			p.resize(2*p.n);
			//to be verified!!!!
			auto v=static_cast<helper<TMP>*>(p.v);
			v[p.n/2].cell_size()=p.n/2;
			v[p.n/2].next()=v[0].next();
			v[0].next()=p.n/2;
			return allocate_t<TMP>(n);//careful with infinite loop!
		}
		return i*sizeof(TMP)/sizeof(T);
	}

	template<typename T> size_t help_allocate_t(info::FIELD& current,size_t n){
		//we need to go through linked list until we find enough contiguous free cells
		auto v=static_cast<helper<T>*>(p.v);
		cerr<<"helper size:"<<sizeof(helper<T>)<<endl;
		cerr<<"attempting to allocate "<<n<<" cells in pool "<<this<<" address:"<<current<<" size:"<<v[current].cell_size()<<" next cell:"<<v[current].next()<<" "<<v[0].next()<<endl;
		if(current==0){
			//no more memory
			cerr<<"allocate "<<n<<" cells at index failed in pool "<<this<<endl;
			//throw std::runtime_error("pool full");
			return 0;
		}else{
			if(v[current].cell_size()<n){
				//infinite loop if v[current].next=current
				//return help_allocate_t<T>(v[current].next(),n);
				if(v[current].next()==current) {
					cerr<<"should not be here..."<<endl;
					//exit(1);
				}
				return v[current].next()==current ? 0 : help_allocate_t<T>(v[current].next(),n);
			}else if(v[current].cell_size()==n){
				size_t tmp=current;
				//something fishy here
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
		enum{TEST=sizeof(T)<sizeof(info)};
		typedef typename IfThenElse<TEST,info,T>::ResultT TMP;
		n=ceil(float(n*sizeof(T))/sizeof(TMP));
		auto v=static_cast<helper<TMP>*>(p.v);
		index=index*sizeof(T)/sizeof(TMP);//what about padding!!!!
		cerr<<"deallocate "<<n<<" cells at index "<<index<<" in pool "<<this<<endl;
		//easy way
		//create cell
		v[index].next()=v[0].next();
		v[index].cell_size()=n;
		v[0].next()=index;
		v[0].n_cells()-=n;
		//v[0].n_cells()-=v[index].cell_size();
	}
	void* get(size_t i);
	//could use function pointer:allocate, deallocate,...
	size_t get_size()const;
	//we need to pass information about the pointer to the store
	template<typename P> static pool_array* get_instance(){
		typedef typename P::STORE STORE;
		static pool_array* p=new pool_array(*STORE::template go<P>(),0,0);
		return p;
	}

};
struct empty_store:public param{//for classes with no instance
	enum{N=0};
	//it needs to have a minimum size
	empty_store();
	template<typename P> static param* go(){return new empty_store();}
	virtual void resize_impl(size_t);
};
class free_store:public param{
public:
	enum{N=1};
	enum{SIZE=256};//minimum size required for 8bit hash
	free_store(void* v,size_t n,const size_t cell_size,const size_t max_size);
	template<typename P> static free_store* go(){
		char* v=new char[SIZE*sizeof(typename P::value_type)];
		memset(v,0,SIZE*sizeof(typename P::value_type));
		cerr<<"max_size:"<<sizeof(typename P::INDEX)<<"\t"<<(1L<<(sizeof(typename P::INDEX)<<3))<<endl;
		return new free_store(v,SIZE,sizeof(typename P::value_type),1L<<(sizeof(typename P::INDEX)<<3));
	}
	//_n new number of cells
	virtual void resize_impl(size_t _n);
};
template<typename T> struct name:name<typename T::SELF>{};
template<> struct name<char>{static const string get(){return "string";}};
template<> struct name<pool>{static const string get(){return "pool";}};
template<> struct name<pool_array>{static const string get(){return "pool_array";}};

class persistent_store:public param{
public:
	enum{N=2};
	enum{SIZE=32};	
	//enum{SIZE=256};	
	enum{PAGE_SIZE=4096};
	//use memory map
	//how to add support for read-only file?
	int fd;
	size_t file_size;//not really needed, we could just stat
	persistent_store(void* v,size_t n,const size_t cell_size,const size_t max_size,const bool writable,int fd,size_t file_size);
	~persistent_store();
	template<typename P> static persistent_store* go(){
		/*
 		* should add ascii file with version information, that number could come from git
		* to build compatible executable
		*/ 
		bool writable=true;
		char* db=getenv("OBJRDF_DB");
		string db_path=db? db : "db";
		string filename=db_path+"/"+name<typename P::value_type>::get();//prevents from using with other type!
		cerr<<"opening file `"<<filename<<"' O_RDWR"<<endl;
		int fd = open(filename.c_str(), O_RDWR | O_CREAT/* | O_TRUNC*/, (mode_t)0600);
		if(fd ==-1){
			cerr<<"opening file `"<<filename<<"' O_RDONLY"<<endl;
			fd = open(filename.c_str(), O_RDONLY/* | O_TRUNC*/, (mode_t)0600);
			writable=false;
		}
		/*
 		*	should still work if file read-only
 		*/ 
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
		size_t file_size=0;
		if(s.st_size==0){
			//new file
			size_t n_page=max<size_t>(ceil((double)(sizeof(typename P::value_type)*SIZE)/PAGE_SIZE),1);
			//makes sure it is a multiple of page size 4096
			file_size=n_page*PAGE_SIZE;
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
		void* v = writable ? mmap((void*)NULL,file_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0) : mmap((void*)NULL,file_size,PROT_READ,MAP_SHARED,fd,0);
		cerr<<"new mapping at "<<v<<" size:"<<file_size<<endl;
		if (v == MAP_FAILED) {
			close(fd);
			cerr<<"Error mmapping the file"<<endl;
			exit(EXIT_FAILURE);
		}
		size_t n_cell=file_size/sizeof(typename P::value_type);
		cerr<<"max_size:"<<sizeof(typename P::INDEX)<<"\t"<<(1L<<(sizeof(typename P::INDEX)<<3))<<endl;
		return new persistent_store(v,n_cell,sizeof(typename P::value_type),1L<<(sizeof(typename P::INDEX)<<3),writable,fd,file_size);
	}
	virtual void resize_impl(size_t _n);
};
template<typename T> struct const_test{enum{N=0};};
template<typename T> struct const_test<const T>{enum{N=1};};

template<
	typename T,
	typename STORE=typename T::STORE,
	bool POLYMORPHISM=false,	//does not support derived types
	typename _INDEX_=uint16_t	//can address 2^16 objects
	//,typename CONTAINER=T		//if we want to add reference counting
> struct pseudo_ptr;


template<
	typename POOL,
	typename _STORE_,
	typename _INDEX_
> 
struct pseudo_ptr_pool{
	typedef _STORE_ STORE;//not used!
	typedef POOL value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr_pool pointer;
	typedef POOL& reference;
	typedef _INDEX_ INDEX;
	INDEX index;
	explicit pseudo_ptr_pool(INDEX index):index(index){}
	static pool* get_pool(){return pool::get_instance<pseudo_ptr_pool>();}
	static pseudo_ptr_pool allocate(){return pseudo_ptr_pool(get_pool()->template allocate_t<POOL>());}
	static pseudo_ptr_pool allocate_at(INDEX i){return pseudo_ptr_pool(get_pool()->template allocate_at(i));}
	static pseudo_ptr_pool construct(param& s,typename POOL::DESTRUCTOR d,size_t type_id){
		pseudo_ptr_pool p=allocate();
		new(p)POOL(s,d,type_id);
		return p;
	}
	/*
	static pseudo_ptr_pool construct_at(INDEX i,param& s,typename POOL::DESTRUCTOR d,size_t type_id){
		pseudo_ptr_pool p=allocate_at(i);
		new(p)POOL(s,d,type_id);
		return p;
	}
	*/
	template<typename T> static void destructor(void* s){
		static_cast<T*>(s)->~T();
	}
	typedef set<INDEX> SET;
	static SET& get_s(){
		static SET s;
		return s;
	}
	/*
 	*  it would be nice the class's name because it will be hard to rename later, ...
 	*  problem: can't use that with POD (char,...)
 	*/ 
	template<typename T> static size_t get_type_id(){
		string n=name<T>::get();
		uint32_t h=jenkins_one_at_a_time_hash(n.c_str(),n.size());
		cerr<<"type_id hash:`"<<n<<"' ->"<<h<<endl;
		return h;
	}
	/*
	template<typename VALUE_TYPE> static pseudo_ptr_pool find(){
		typedef typename VALUE_TYPE::STORE OTHER_STORE;
		auto i=find_if(get_pool()->template begin<pseudo_ptr_pool>(),get_pool()->template end<pseudo_ptr_pool>(),pool::match_type_id(get_type_id<VALUE_TYPE>()));
		pseudo_ptr_pool p=(i==get_pool()->template end<pseudo_ptr_pool>()) ? allocate() : *i;
		cerr<<"query pool index for type `"<<name<VALUE_TYPE>::get()<<"' "<<(int)p.index<<endl;
		//problem here could also be pseudo_ptr_array, need to investigate
		//what if OTHER_STORE=empty_store, we should just skip
		//even if we find it we need to override it because of stale pointers
		new(p)POOL(*OTHER_STORE::template go<pseudo_ptr<VALUE_TYPE,OTHER_STORE,false,uint16_t>>(),destructor<VALUE_TYPE>,get_type_id<VALUE_TYPE>());
		return p;
	}
 	//	this is used when creating a new class to find out what the index should be,
 	//	awkward because the only information is the VALUE_TYPE, is there instances where it is called before the pool
 	//	is created????
	template<typename VALUE_TYPE> static pseudo_ptr_pool help(){
		cerr<<"query pool index for type `"<<name<VALUE_TYPE>::get()<<"'"<<endl;
		static pseudo_ptr_pool p=find<VALUE_TYPE>();
		return p;
	}
	template<typename P> static pseudo_ptr_pool get(){
		//cerr<<"get_pool: "<<get_pool()<<endl;
		//we need to separate the pseudo_ptr from pseudo_ptr_array!!!
		typedef typename P::value_type VALUE_TYPE;
		typedef typename P::STORE OTHER_STORE;
		//static pseudo_ptr_pool p=help<T,OTHER_STORE>();
		static pseudo_ptr_pool p=help<VALUE_TYPE>();
		//static pseudo_ptr_pool p=find<VALUE_TYPE>();
		return p;
	}
	*/
	template<typename POINTER> static pseudo_ptr_pool find(){
		typedef typename POINTER::STORE OTHER_STORE;
		auto i=find_if(get_pool()->template begin<pseudo_ptr_pool>(),get_pool()->template end<pseudo_ptr_pool>(),pool::match_type_id(get_type_id<typename POINTER::value_type>()));
		pseudo_ptr_pool p=(i==get_pool()->template end<pseudo_ptr_pool>()) ? allocate() : *i;
		new(p)POOL(*OTHER_STORE::template go<POINTER>(),destructor<typename POINTER::value_type>,get_type_id<typename POINTER::value_type>());
		//a this stage we can go through all elements in pool and index them
		//for(auto i=p->template begin<POINTER>();i!=p->template end<POINTER>();++i) POINTER::value_type::do_index(*i);
		return p;
	}
	template<typename P> static pseudo_ptr_pool help(){
		static pseudo_ptr_pool p=find<P>();
		return p;
	}
	template<typename P> static pseudo_ptr_pool get(){
		static pseudo_ptr_pool p=help<P>();
		return p;
	}
	static value_type* get_typed_v(){return static_cast<value_type*>(get_pool()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	bool operator==(const pseudo_ptr_pool& p)const{return index==p.index;}
	bool operator!=(const pseudo_ptr_pool& p)const{return index!=p.index;}
	operator value_type*() {return get_typed_v()+index;}
};


typedef pseudo_ptr_pool<
	pool,
	persistent_store,//we need to save index for consistency
	uint8_t//we don't expect more than 255 pools, that is 255 types
> POOL_PTR;  //could we have more than one of those???, yes if we defined another type of pool

typedef pseudo_ptr_pool<
	pool_array,
	persistent_store,//we need to save index for consistency
	uint8_t
> POOL_PTR_ARRAY;

/*
 *	we can add container:
 *	struct container{
 *		M meta;
 *		T payload;
 *	};
 *	meta could be reference counter or other data
 *
 */

/*
 *	problem that the pool might be queried and crashes
 *	we could define an empty pool just for that purpose
 *	or use the free store with minimal size, more risk of 
 *	hash collision
 *
 */
/*
template<
	typename T,
	typename _INDEX_
> struct pseudo_ptr<T,empty_store,false,_INDEX_>{
	static POOL_PTR get_pool(){
		static POOL_PTR p=POOL_PTR::allocate();
		return p;
	}
}; 
*/
template<typename S,typename T> struct is_derived{enum{value=false};};
template<typename S> struct is_derived<S,S>{enum{value=true};};	
/*
 *	this pointer is smaller than the polymorphic one, 
 *	use when the class is not up/down cast
 */
template<
	typename T,
	typename _STORE_,
	typename _INDEX_
> struct pseudo_ptr<T,_STORE_,false,_INDEX_>{
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
	typedef _INDEX_ INDEX;
	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef T& reference;
	static POOL_PTR get_pool(){return POOL_PTR::get<pseudo_ptr>();}
	INDEX index;
	//explicit pseudo_ptr(INDEX index):index(index){}
	pseudo_ptr(INDEX index=0):index(index){}
	pseudo_ptr(INDEX index,POOL_PTR):index(index){}
	void _print(ostream& os) const{
		os<<'a'<<hex<<(unsigned int)index;
	}
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
	template<
		typename S,
		typename OTHER_STORE,
		typename OTHER_INDEX
	> pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,true,OTHER_INDEX>& p):index(p.index){
		static_assert(sizeof(INDEX)>=sizeof(OTHER_INDEX),"cast to a smaller type");
		value_type *a;
		S *b;
		a=static_cast<value_type*>(b);	
		assert(p.pool_ptr==get_pool()||index==0);
	} 
	//is there reference counting?
	~pseudo_ptr(){

	}
	//static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->allocate());}
	static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->template allocate_t<T>());}
	//equivalent to return new T(s), could use variadic template
	template<typename... Args> static pseudo_ptr construct(Args... args){
		pseudo_ptr p=allocate();
		new(p) T(args...);
		T::do_index(p);//
		return p;
	}
	//void deallocate(){get_pool()->deallocate(index);}
	void deallocate(){get_pool()->template deallocate_t<T>(index);}
	void destroy(){
		(*this)->~T();//in-place destructor
		deallocate();		
	}
	//might have to put offset here if inside container
	static value_type* get_typed_v(){return static_cast<value_type*>(get_pool()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	pseudo_ptr& operator+(size_t s){
		index+=s;
		return *this;
	}
	bool operator!=(const pseudo_ptr& p)const{return index!=p.index;}
	bool operator<(const pseudo_ptr& p)const{return index<p.index;}
	bool operator==(const pseudo_ptr& p)const{return index==p.index;}
	//
	//bool operator==(const void* p)const{return (get_typed_v())+index==p;}
	//
	//never called!
	//operator bool() const{return index;}
	//problem: not equal to zero even when index=0
	operator value_type*() {return index ? get_typed_v()+index:0;}
	bool operator!()const{return !index;}
};
/*
 *	can point to T or sub-class of T
 */
template<
	typename T,
	typename _STORE_,
	typename _INDEX_
>
struct pseudo_ptr<T,_STORE_,true,_INDEX_>{
	typedef _STORE_ STORE;
	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef T& reference;
	typedef _INDEX_ INDEX;
	POOL_PTR pool_ptr;
	INDEX index;
	static POOL_PTR get_pool(){return POOL_PTR::get<pseudo_ptr>();}
	//explicit pseudo_ptr(INDEX index=0):index(index),pool_ptr(get_pool()){}
	//why explicit?
	explicit pseudo_ptr(INDEX index=0,POOL_PTR pool_ptr=get_pool()):index(index),pool_ptr(pool_ptr){}
	void print(ostream& os){
		os<<"{"<<(unsigned int)pool_ptr.index<<","<<(unsigned short)index<<"}"<<endl;
	}
	void _print(ostream& os) const{
		os<<'a'<<hex<<(unsigned int)pool_ptr.index<<(unsigned int)index;
	}
	/*
	* dangerous, be very careful with this
	* 	should be used like this:
	* 	pseudo_ptr a(new(pseudo_ptr::allocate())T(...)
	*/
	//explicit pseudo_ptr(T* p):index(((void)p-get_pool()->p.v)/sizeof(T)),pool_ptr(get_pool()){}
	//static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->allocate());}
	static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->template allocate_t<T>());}//how come depends on T?
	static pseudo_ptr allocate_at(size_t i){return pseudo_ptr(get_pool()->allocate_at(i));}//how come does not depend on T???
	//would be good if it could create any type, not just T, there might be a way
	template<typename... Args> static pseudo_ptr construct(Args... args){
		pseudo_ptr p=allocate();
		new(p) T(args...);
		T::do_index(p);
		return p;
	}
	//maybe should only be for pseudo_ptr<const T>
	template<typename... Args> static pseudo_ptr construct_at(size_t i,Args... args){
		pseudo_ptr p=allocate_at(i);
		new(p) T(args...);
		T::do_index(p);
		return p;
	}
	//check if pool does reference counting
	~pseudo_ptr(){

	}
	void deallocate(){pool_ptr->deallocate(index);}
	void destroy(){
		//a bit more complicated: it could be any type, we need to store a function pointer somewhere
		pool_ptr->destructor(*this);
		deallocate();
	}
	template<
		typename S,
		typename OTHER_STORE,
		typename OTHER_INDEX
	> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,false,OTHER_INDEX>& p):pool_ptr(p.get_pool()),index(p.index){
		static_assert(sizeof(INDEX)>=sizeof(OTHER_INDEX),"cast to a smaller type");
		//enforce standard C pointer conversion rules
		value_type *a;
		S *b;
		a=static_cast<value_type*>(b);	
	}
	template<
		typename S,
		typename OTHER_STORE,
		typename OTHER_INDEX
	> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,true,OTHER_INDEX>& p):pool_ptr(p.pool_ptr),index(p.index){
		static_assert(sizeof(INDEX)>=sizeof(OTHER_INDEX),"cast to a smaller type");
		//enforce standard C pointer conversion rules
		value_type *a;
		S *b;
		a=static_cast<value_type*>(b);	
		//not complete because p could be derived from S
	}
	value_type* operator->()const{return (value_type*)pool_ptr->get(index);}
	reference operator*()const{return *(value_type*)pool_ptr->get(index);}
	bool operator!=(const pseudo_ptr& p)const{return pool_ptr!=p.pool_ptr || index!=p.index;}
	bool operator<(const pseudo_ptr& p)const{return pool_ptr==p.pool_ptr && index<p.index;}
	bool operator==(const pseudo_ptr& p)const{return pool_ptr==p.pool_ptr && index==p.index;}
	//
	//bool operator==(const void* p)const{return pool_ptr->get(index)==p;}
	//never used
	operator bool() const{return index;}
	//not very efficient
	operator value_type*() {return index ? (value_type*)pool_ptr->get(index) : 0;}
	//better
	bool operator!()const{return !index;}
};
template<
	typename T,
	typename _STORE_,
	typename _INDEX_
> struct pseudo_ptr<const T,_STORE_,false,_INDEX_>{
	typedef _STORE_ STORE;
	typedef random_access_iterator_tag iterator_category;
	typedef const T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef const T& reference;
	typedef _INDEX_ INDEX;
	//should it use its own pool?, that is the safest 
	//static POOL_PTR get_pool(){return POOL_PTR::get<pseudo_ptr>();}//will create new pool
	static POOL_PTR get_pool(){return POOL_PTR::get<pseudo_ptr<T,_STORE_,false,_INDEX_>>();}
	INDEX index;
	pseudo_ptr(INDEX index=0):index(index){}
	pseudo_ptr(INDEX index,POOL_PTR):index(index){}
	template<
		typename S,
		typename OTHER_STORE,
		typename OTHER_INDEX
	> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,true,OTHER_INDEX>& p):index(p.index){
		assert((p.pool_ptr==get_pool())||(index==0));
		value_type *a;
		S *b;
		a=static_cast<value_type*>(b);	
	}

	//we don't need this
	pseudo_ptr& operator=(const pseudo_ptr& p){
		index=p.index;
		return *this;
	}
	static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->template allocate_t<T>());}
	static pseudo_ptr allocate_at(size_t i){return pseudo_ptr(get_pool()->template allocate_at(i));}
	template<typename... Args> static pseudo_ptr construct(Args... args){
		pseudo_ptr p=allocate();
		new(p) T(args...);
		//T::do_index(p);
		return p;
	}
	template<typename... Args> static pseudo_ptr construct_at(size_t i,Args... args){
		pseudo_ptr p=allocate_at(i);
		new(p) T(args...);
		//T::do_index(p);
		return p;
	}
	static T* get_typed_v(){return static_cast<T*>(get_pool()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	bool operator!=(const pseudo_ptr& p)const{return index!=p.index;}
	bool operator<(const pseudo_ptr& p)const{return index<p.index;}
	bool operator==(const pseudo_ptr& p)const{return index==p.index;}
	//used by new
	operator T*() {return index ? get_typed_v()+index:0;}
};
//does it make sense with POLYMORPHISM=true?
//yes
template<
	typename T,
	typename _STORE_,
	typename _INDEX_
> struct pseudo_ptr<const T,_STORE_,true,_INDEX_>{
	typedef _STORE_ STORE;
	typedef random_access_iterator_tag iterator_category;
	typedef const T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef const T& reference;
	typedef _INDEX_ INDEX;
	POOL_PTR pool_ptr;
	INDEX index;
	static POOL_PTR get_pool(){return POOL_PTR::get<pseudo_ptr>();}
	//explicit pseudo_ptr(INDEX index=0):index(index),pool_ptr(get_pool()){}
	//why explicit?
	explicit pseudo_ptr(INDEX index=0,POOL_PTR pool_ptr=get_pool()):index(index),pool_ptr(pool_ptr){}
	static pseudo_ptr allocate(){return pseudo_ptr(get_pool()->template allocate_t<T>());}
	template<
		typename S,
		typename OTHER_STORE,
		typename OTHER_INDEX
	> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,true,OTHER_INDEX>& p):pool_ptr(p.pool_ptr),index(p.index){
		static_assert(sizeof(INDEX)>=sizeof(OTHER_INDEX),"cast to a smaller type");
		enum{a=is_derived<typename S::SELF,typename T::SELF>::value};
		enum{b=is_derived<typename T::SELF,typename S::SELF>::value};
		//so we get explicit error message
		typedef typename IfThenElse<a||b,pseudo_ptr,T>::ResultT::STORE A;
		typedef typename IfThenElse<a||b,pseudo_ptr,S>::ResultT::STORE B;
		static_assert(is_derived<typename S::SELF,typename T::SELF>::value||is_derived<typename T::SELF,typename S::SELF>::value,"types are not related");
	}
	template<
		typename S,
		typename OTHER_STORE,
		typename OTHER_INDEX
	> 
	pseudo_ptr(const pseudo_ptr<S,OTHER_STORE,false,OTHER_INDEX>& p):pool_ptr(pseudo_ptr<S,OTHER_STORE,false,OTHER_INDEX>::get_pool()),index(p.index){
		static_assert(sizeof(INDEX)>=sizeof(OTHER_INDEX),"cast to a smaller type");
		enum{a=is_derived<typename S::SELF,typename T::SELF>::value};
		enum{b=is_derived<typename T::SELF,typename S::SELF>::value};
		//so we get explicit error message
		typedef typename IfThenElse<a||b,pseudo_ptr,T>::ResultT::STORE A;
		typedef typename IfThenElse<a||b,pseudo_ptr,S>::ResultT::STORE B;
		static_assert(is_derived<typename S::SELF,typename T::SELF>::value||is_derived<typename T::SELF,typename S::SELF>::value,"types are not related");
	}

	template<typename A> static pseudo_ptr construct(A a){
		pseudo_ptr p=allocate();
		//problem here p is cast to const void*
		new(p)T(a);//in-place constructor
		//T::do_index(p);
		return p;
	} 
	//have to check this	
	operator T*() {return index ? (T*)pool_ptr->get(index) : 0;}
	value_type* operator->()const{return (value_type*)pool_ptr->get(index);}
	reference operator*()const{return *(value_type*)pool_ptr->get(index);}
	bool operator==(const pseudo_ptr& p)const{return pool_ptr==p.pool_ptr && index==p.index;}
};
	
template<
	typename T,
	typename STORE,
	bool POLYMORPHISM=false,
	typename INDEX=uint16_t
> struct pseudo_ptr_array;
//no polymorphism for now
template<
	typename T,
	typename _STORE_,
	typename _INDEX_
> struct pseudo_ptr_array<T,_STORE_,false,_INDEX_>{
	typedef _STORE_ STORE;
	typedef _INDEX_ INDEX;
	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr_array pointer;
	typedef T& reference;
	static POOL_PTR_ARRAY get_pool(){return POOL_PTR_ARRAY::get<pseudo_ptr_array>();}
	INDEX index;
	pseudo_ptr_array(INDEX index=0):index(index){}
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
	template<typename S,typename OTHER_STORE,typename OTHER_INDEX> pseudo_ptr_array(const pseudo_ptr_array<S,OTHER_STORE,true,OTHER_INDEX>& p):index(p.index){
		static_assert(sizeof(INDEX)>=sizeof(OTHER_INDEX),"cast to a smaller type");
		//we can make it safe but should be optional 
		static_assert(is_derived<S,T>::value||is_derived<T,S>::value,"types are not related");
		assert(p.pool_ptr==get_pool());
	} 
	//cast away constness, should be used with care
	pseudo_ptr_array(const pseudo_ptr_array<const T,STORE,false,INDEX>& p):index(p.index){}
	pseudo_ptr_array& operator=(const pseudo_ptr_array& p){
		index=p.index;
		return *this;
	}
	static pseudo_ptr_array allocate(){return pseudo_ptr_array(get_pool()->template allocate_t<T>());}
	static pseudo_ptr_array allocate(size_t n){return pseudo_ptr_array(get_pool()->template allocate_t<T>(n));}
	void deallocate(){get_pool()->template deallocate_t<T>(index);}
	void deallocate(size_t n){get_pool()->template deallocate_t<T>(index,n);}
	void destroy(){
		(*this)->~T();//in-place destructor
		deallocate();		
	}
	static value_type* get_typed_v(){return static_cast<value_type*>(get_pool()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	pseudo_ptr_array& operator+=(const size_t s){index+=s;return *this;}
	pseudo_ptr_array& operator-=(const size_t s){index-=s;return *this;}
	pseudo_ptr_array& operator++(){++index;return *this;}
	pseudo_ptr_array& operator--(){--index;return *this;}
	bool operator!=(const pseudo_ptr_array& p)const{return index!=p.index;}
	bool operator<(const pseudo_ptr_array& p)const{return index<p.index;}
	bool operator==(const pseudo_ptr_array& p)const{return index==p.index;}
	//
	bool operator==(const void* p)const{return (get_typed_v())+index==p;}
	//
	//never called! and causes ambiguity when compiling
	//operator bool() const{return index;}
	//problem: not equal to zero even when index=0
	operator value_type*() {return index ? get_typed_v()+index:0;}
	bool operator!()const{return !index;}
};
template<
	typename T,
	typename STORE,
	bool POLYMORPHISM,
	typename INDEX
> pseudo_ptr_array<T,STORE,POLYMORPHISM,INDEX> operator+(const pseudo_ptr_array<T,STORE,POLYMORPHISM,INDEX>& a,size_t s){
	pseudo_ptr_array<T,STORE,POLYMORPHISM,INDEX> tmp=a;
	return tmp+=s;
}
template<
	typename T,
	typename STORE,
	typename INDEX
> pseudo_ptr_array<T,STORE,false,INDEX> operator-(const pseudo_ptr_array<T,STORE,false,INDEX>& a,size_t s){
	pseudo_ptr_array<T,STORE,false,INDEX> tmp=a;
	return tmp-=s;
}

template<
	typename T,
	typename STORE,
	bool POLYMORPHISM,
	typename INDEX> 
ptrdiff_t operator-(const pseudo_ptr_array<T,STORE,POLYMORPHISM,INDEX>& a,const pseudo_ptr_array<T,STORE,POLYMORPHISM,INDEX>& b){
	return a.index-b.index;
}
template<
	typename _STORE_,
	typename _INDEX_
> struct pseudo_ptr_array<char,_STORE_,false,_INDEX_>{
	typedef _STORE_ STORE;
	typedef _INDEX_ INDEX;
	typedef random_access_iterator_tag iterator_category;
	typedef char value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr_array pointer;
	typedef char& reference;
	static POOL_PTR_ARRAY get_pool(){return POOL_PTR_ARRAY::get<pseudo_ptr_array>();}
	INDEX index;
	pseudo_ptr_array(INDEX index=0):index(index){}
	//dangerous!!!
	//pseudo_ptr_array(char* p):index(p-(char*)get_pool()->p.v){}
	//cast away constness, should be used with care
	pseudo_ptr_array(const pseudo_ptr_array<const char,STORE,false,INDEX>& p):index(p.index){}
	pseudo_ptr_array& operator=(const pseudo_ptr_array& p){
		index=p.index;
		return *this;
	}
	static pseudo_ptr_array allocate(size_t n){
		return pseudo_ptr_array(get_pool()->template allocate_t<char>(n));
	}
	void deallocate(size_t n){
		get_pool()->template deallocate_t<char>(index,n);
	}
	void destroy(){
		deallocate();		
	}
	static value_type* get_typed_v(){return static_cast<value_type*>(get_pool()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	pseudo_ptr_array& operator+=(const size_t s){index+=s;return *this;}
	pseudo_ptr_array& operator-=(const size_t s){index-=s;return *this;}
	pseudo_ptr_array& operator++(){++index;return *this;}
	pseudo_ptr_array& operator--(){--index;return *this;}
	bool operator!=(const pseudo_ptr_array& p)const{return index!=p.index;}
	bool operator<(const pseudo_ptr_array& p)const{return index<p.index;}
	bool operator==(const pseudo_ptr_array& p)const{return index==p.index;}
	//
	bool operator==(const void* p)const{return (get_typed_v())+index==p;}
	operator value_type*() {return index ? get_typed_v()+index:0;}
	bool operator!()const{return !index;}
};
template<
	typename _STORE_,
	typename _INDEX_
> struct pseudo_ptr_array<const char,_STORE_,false,_INDEX_>{
	typedef _STORE_ STORE;
	typedef _INDEX_ INDEX;
	typedef random_access_iterator_tag iterator_category;
	typedef char value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr_array pointer;
	typedef char& reference;
	//static POOL_PTR_ARRAY get_pool(){return POOL_PTR_ARRAY::get<pseudo_ptr_array>();}
	static POOL_PTR_ARRAY get_pool(){return POOL_PTR_ARRAY::get<pseudo_ptr_array<char,_STORE_,false,_INDEX_>>();}
	INDEX index;
	pseudo_ptr_array(INDEX index=0):index(index){}
	//dangerous!!!
	pseudo_ptr_array(char* p):index(p-(char*)get_pool()->p.v){}
	//cast away constness, should be used with care
	pseudo_ptr_array(const pseudo_ptr_array<const char,STORE,false,INDEX>& p):index(p.index){}
	pseudo_ptr_array& operator=(const pseudo_ptr_array& p){
		index=p.index;
		return *this;
	}
	static pseudo_ptr_array allocate(size_t n){
		//allocate more than needed
		return pseudo_ptr_array(get_pool()->template allocate_t<char[4]>(n));
	}
	void deallocate(size_t n){
		get_pool()->template deallocate_t<char[4]>(index,n);
	}
	void destroy(){
		deallocate();		
	}
	static value_type* get_typed_v(){return static_cast<value_type*>(get_pool()->p.v);} 
	value_type* operator->()const{return get_typed_v()+index;}
	reference operator*()const{return *(get_typed_v()+index);}
	pseudo_ptr_array& operator+=(const size_t s){index+=s;return *this;}
	pseudo_ptr_array& operator-=(const size_t s){index-=s;return *this;}
	pseudo_ptr_array& operator++(){++index;return *this;}
	pseudo_ptr_array& operator--(){--index;return *this;}
	bool operator!=(const pseudo_ptr_array& p)const{return index!=p.index;}
	bool operator<(const pseudo_ptr_array& p)const{return index<p.index;}
	bool operator==(const pseudo_ptr_array& p)const{return index==p.index;}
	//
	bool operator==(const void* p)const{return (get_typed_v())+index==p;}
	operator value_type*() {return index ? get_typed_v()+index:0;}
	bool operator!()const{return !index;}
};

//iterators to examine a pool
template<typename T> pool::iterator<T> begin(){return T::get_pool()->begin<T>();}
template<typename T> pool::iterator<T> end(){return T::get_pool()->end<T>();}
template<typename ITERATOR> struct skip_iterator{
	const size_t increment;
	ITERATOR i;
	size_t index;
	skip_iterator(const ITERATOR& i,const size_t increment):i(i),increment(increment),index(0){}
	skip_iterator& operator++(){
		i+=increment;
		++index;
		return *this;
	}
	skip_iterator& operator+=(size_t n){
		i+=n*increment;
		index+=n;
		return *this;
	}
	
};
//P must be a pseudo_ptr	
template<typename P> struct generic_iterator{
	typedef P value_type;	
	typedef P& reference;	
	typedef typename P::value_type S;
	POOL_PTR pool_ptr;
	typedef skip_iterator<void*> SKIP;
	SKIP s;
	int n;//number of cells to visit
	generic_iterator(POOL_PTR pool_ptr,int _n):pool_ptr(pool_ptr),s(pool_ptr->p.v,pool_ptr->p.cell_size),n(_n){
		++s;//skip first cell
		if(n)
			while(!S::TEST(*static_cast<S*>(s.i))) ++s; //awkward because v is void*
		--n;
		cerr<<"first: "<<this->n<<endl;
	}		
	//only used as end iterator
	generic_iterator(POOL_PTR pool_ptr):pool_ptr(pool_ptr),s(pool_ptr->p.v,pool_ptr->p.cell_size),n(-1){}
	bool operator==(const generic_iterator& p)const{return n==p.n;}
	bool operator!=(const generic_iterator& p)const{return n!=p.n;}
	bool operator<(const generic_iterator& p)const{return n>p.n;}
	generic_iterator& operator++(){
		++s;
		if(n) 
			while(!S::TEST(*static_cast<S*>(s.i))) ++s;
		--n;
		return *this;
	}
	value_type operator*(){
		return P(s.index,pool_ptr);
	}
};

#endif

