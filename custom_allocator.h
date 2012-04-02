#ifndef CUSTOM_ALLOCATOR_H
#define CUSTOM_ALLOCATOR_H
/*
 *	create a custom vector allocator 
 *	works only for big objects (so we can reuse cells to store information)
 *	or small objects but not too many of them
 */
#include "pseudo_ptr.h"

#include <vector>
template<
	typename T,
	typename _pointer_//=pseudo_ptr<T>
> struct custom_allocator{
	typedef T value_type;
	typedef _pointer_ pointer;
	typedef pointer const_pointer;
	//typedef typename pseudo_ptr<T>::pseudo_ref reference;
	//typedef typename pseudo_ptr<T>::reference reference;
	typedef typename pointer::reference reference;
	typedef reference const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;

	pointer address(reference value) const{return &value;}
	/*
	const_pointer address (const_reference value) const {
	   return &value;
	}
	*/
	//what is that for?
	//template<class U> struct rebind{typedef custom_allocator<U> other;};
	template<class U> struct rebind{typedef custom_allocator other;};
	size_type max_size() const throw() {return pointer::get_pool()->p.n;}
	pointer allocate (size_type num, const void* = 0) {
		std::cerr << "allocate " << num << " element(s)" << " of size " << sizeof(T) << std::endl;
		pointer ret(pointer::get_pool()->template allocate_t<T>(num));
		std::cerr << " allocated at: " << ret.operator->() << std::endl;
		return ret;
	}
	//void construct(pointer p,T value){new(p)T(value);}
	void construct(T* p,T value){new(p)T(value);}
	//void destroy (pointer p) {p->~T();}
	void destroy(T* p){p->~T();}
	void deallocate (pointer p, size_type num) {
		std::cerr << "deallocate " << num << " element(s)" << " of size " << sizeof(T) << " at: " << p.operator->() << std::endl;
		pointer::get_pool()->template deallocate_t<T>(p.index,num);
	}
};
#endif
