#ifndef CUSTOM_ALLOCATOR_H
#define CUSTOM_ALLOCATOR_H
/*
 *	create a custom vector allocator 
 *	works only for big objects (so we can reuse cells to store information)
 *	or small objects but not too many of them
 */
#include "pseudo_ptr.h"

#include <vector>
/*
 * generic allocator, we need to let it know if we are going to use 
 * many-cell or single cell allocation, would be nice if we could use a single
 * implementation for both
 *
 *
 */
namespace local{
	template<typename A,typename B> struct equality{enum{VALUE=0};};
	template<typename A> struct equality<A,A>{enum{VALUE=1};};
}
template<
	typename VALUE_TYPE,
	typename _STORE_,
	typename INDEX=uint16_t
> struct pool_allocator{
	typedef _STORE_ STORE;
	typedef pseudo_ptr<VALUE_TYPE,STORE,false,INDEX> pointer;
	typedef pseudo_ptr<const VALUE_TYPE,STORE,false,INDEX> const_pointer;
	//can be used to reference any subclass of VALUE_TYPE
	typedef pseudo_ptr<VALUE_TYPE,STORE,true,INDEX> derived_pointer;
	typedef pseudo_ptr<const VALUE_TYPE,STORE,true,INDEX> const_derived_pointer;
	typedef VALUE_TYPE value_type;
	typedef size_t size_type;//for now
	template<typename T> static void destructor(void* s){
		static_cast<T*>(s)->~T();
	}
	static POOL_PTR get_index(){
		static auto p=POOL_PTR::help<pointer>(); 
		//static auto p=POOL_PTR::help<value_type>(); 
		//static auto p=_get_index();
		return p; 
	}
	static POOL_PTR _get_index(){
		auto i=find_if(POOL_PTR::get_pool()->template begin<POOL_PTR>(),POOL_PTR::get_pool()->template end<POOL_PTR>(),pool::match_type_id(POOL_PTR::get_type_id<value_type>()));
		POOL_PTR p=(i==POOL_PTR::get_pool()->template end<POOL_PTR>()) ? POOL_PTR::allocate() : *i;
		//awkward will need to be fixed
		new(p) typename POOL_PTR::value_type(*STORE::template go<pseudo_ptr<value_type,STORE,false,INDEX>>(),destructor<value_type>,POOL_PTR::get_type_id<value_type>());
		cerr<<"pool of `"<<name<value_type>::get()<<"' at index "<<(int)p.index<<" vs "<<(int)POOL_PTR::help<value_type>().index<<endl; 
		return p;
	}
	pointer allocate(size_type n/*, std::allocator<void>::const_pointer hint = 0*/ ){
		return pointer::get_pool()->template allocate_t<VALUE_TYPE>();
	}
	void deallocate(pointer p, size_type n){
		pointer::get_pool()->template deallocate_t<VALUE_TYPE>(p.index);
	}
	//template<typename... Args> void construct(pointer p,Args&&... args){
	template<typename... Args> void construct(pointer p,Args... args){
		new(p) VALUE_TYPE(args...);
	}
	template<typename... Args> pointer construct_r(pointer p,Args... args){
		new(p) VALUE_TYPE(args...);
		return p;
	}
	void destroy(pointer p){
		p->~VALUE_TYPE();
	}
	//add functions to navigate pool iterators, begin,end,cbegin,cend
	typedef typename pool::iterator<pointer> iterator;
	iterator begin(){return get_index()->template begin<pointer>();}
	iterator end(){return get_index()->template end<pointer>();}
	//should only allocate resources using persistent_store
	struct persistent{	
		static int go(){
			pool_allocator a;
			for(auto i=a.begin();i!=a.end();++i) value_type::do_index(*i);
			return 0;
		}
	};
	struct other{
		static int go(){return 0;}
	};
	static int _index(){return IfThenElse<local::equality<STORE,persistent_store>::VALUE,persistent,other>::ResultT::go();}
};



#ifdef JJJJJJJJJ
template<
	typename VALUE_TYPE,
	/* the pool should not depend on that */
	bool CAN_BE_DERIVED=false,
	typename STORE=free_store,
	typename POINTER=pseudo_ptr<VALUE_TYPE,CAN_BE_DERIVED,STORE>,
	typename CONST_POINTER=pseudo_ptr<const VALUE_TYPE,CAN_BE_DERIVED,STORE>
> struct pool_allocator{
	typedef VALUE_TYPE value_type;
	typedef POINTER pointer;
	typedef CONST_POINTER const_pointer;
	//we could defined pointer for polymorphism
	typedef pseudo_ptr<VALUE_TYPE,true,STORE> pointer_derived;
	typedef pseudo_ptr<const VALUE_TYPE,true,STORE> const_pointer_derived;
	typedef typename pointer::reference reference;
	typedef typename const_pointer::reference const_reference;
	//to be improved
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;
	/*
 	*	what pool are we using?
 	*	we need to store all the pools in a vector but the index needs to be conserved
 	*	between runs, we also have to provide a file name for persistent storage 
 	*	name	index
 	*	pool_0	1
 	*	pool_1	2
 	*	....
 	*
 	*/ 
	static pool* get_pool();
	pointer address(reference value) const{return &value;}
	template<class U> struct rebind{typedef pool_allocator<U,CAN_BE_DERIVED,STORE,POINTER,PSEUDO_POINTER> other;};//not correct!!!!
	size_type max_size() const throw() {return get_pool()->p.n;}
	pointer allocate(size_type num, const void* = 0) {
		return pointer(get_pool()->template allocate_t<VALUE_TYPE>(num));
	}
	void construct(pointer p,T value){new(p)VALUE_TYPE(value);}
	void destroy(pointer p) {p->~VALUE_TYPE();}
	void deallocate (pointer p, size_type num) {
		get_pool()->template deallocate_t<VALUE_TYPE>(p.index,num);
	}
	bool operator==(const custom_allocator& c) const {return true;}
};
//specialize for pool of pools
template<> struct pool_allocator<pool,false,persistent_store>{
	
};
struct help{
	typedef vector<pool,pool_allocator<pool,persistent_store>> POOLS;
	static POOLS& get(){
		static POOLS* p=new POOLS();
		return *p;
	}
	template<typename ALLOCATOR> static POOLS::iterator get_pool(){
		/*
 		*	look for a pool of ALLOCATOR::VALUE_TYPE, if not present append to vector
 		*/ 
		auto i=find_if(get().begin(),get().end(),comp<VALUE_TYPE>());
		if(i!=get().end()) return i;
		get().push_back(pool(ALLOCATOR::STORE::template go<pseudo_ptr<VALUE_TYPE,OTHER_STORE,false,uint16_t>>(),destructor<VALUE_TYPE>,get_type_id<VALUE_TYPE>()));

	}

};

#endif

template<
	typename T,
	typename _pointer_,//=pseudo_ptr_array<T>
	typename _const_pointer_=_pointer_
> struct custom_allocator{
	typedef T value_type;
	typedef _pointer_ pointer;
	//this causes problem
	typedef _const_pointer_ const_pointer;
	typedef typename pointer::reference reference;
	typedef typename const_pointer::reference const_reference;
	//not correct: should use _pointer_::INDEX
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;
	//typedef typename pointer::INDEX size_type;
	//typedef typename pointer::INDEX difference_type;

	pointer address(reference value) const{return &value;}
	/*
	const_pointer address (const_reference value) const {
	   return &value;
	}
	*/
	/*
 	* The member template class rebind provides a way to obtain an allocator for a different type.
	* in case of basic_string<> we have
	*	typedef typename _Alloc::template rebind<char>::other _Raw_bytes_alloc;
	*	void* __place = _Raw_bytes_alloc(__alloc).allocate(__size);
	*	_Rep *__p = new (__place) _Rep;
	*	...
	*	_Raw_bytes_alloc(__a).deallocate(reinterpret_cast<char*>(this), __size);
	* where _Rep is a structure used for string management
	* problem here: deallocate expect a pointer not a char*, there is a conversion from char* to pointer using pointer
	* arithmetic:
	*	index=((void*)p-get_pool()->p.v)/sizeof(T)
	*
	* but it requires get_pool()->p.v to be constant which is not guaranteed if the pool has been resized, let's live with that for now
	*/
	template<class U> struct rebind{typedef custom_allocator<U,_pointer_,_const_pointer_> other;};//not correct!!!!
	size_type max_size() const throw() {
		return pointer::get_pool()->p.n;//this is not correct!
	}
	pointer allocate (size_type num, const void* = 0) {
		std::cerr<<"custom_allocator::allocate "<<num<<" element(s) of size "<<sizeof(T)<<std::endl;
		pointer ret(pointer::get_pool()->template allocate_t<T>(num));
		std::cerr<<"custom_allocator allocated at: "<<(void*)ret.operator->()<<std::endl;
		return ret;
	}
	//void construct(pointer p,T value){new(p)T(value);}
	void construct(T* p,T value){
		new(p)T(value);
	}
	//void destroy (pointer p) {p->~T();}
	void destroy(T* p){p->~T();}
	void deallocate (pointer p, size_type num) {
		std::cerr<<"custom_allocator::deallocate "<<num<<" element(s) of size "<<sizeof(T)<<" at: "<<(void*)p.operator->()<<std::endl;
		pointer::get_pool()->template deallocate_t<T>(p.index,num);
	}
	bool operator==(const custom_allocator& c) const {return true;}
};
#endif
