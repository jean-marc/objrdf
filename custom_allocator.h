#ifndef CUSTOM_ALLOCATOR_H
#define CUSTOM_ALLOCATOR_H
/*
 *	create a custom vector allocator 
 *
 */
#include <vector>
#include <iterator>
using namespace std;
enum{N=1024};
template<typename T> struct pseudo_ptr{
	typedef char INDEX;
	struct pool{
		T* v;
		INDEX index;
		pool():index(0),v((T*)new char[N*sizeof(T)]){}
		static pool& get_instance(){
			static pool* p=new pool();
			return *p;
		}
		/*
 		*	very dumb, will run out of memory
 		*/ 
		pseudo_ptr allocate(size_t num){
			pseudo_ptr p(index);
			index+=num;
			return p;
		}
		void deallocate(pseudo_ptr p){}
	};
	struct pseudo_ref{
		/*const*/ INDEX index;
		explicit pseudo_ref(INDEX index):index(index){}
		pseudo_ptr operator&(){return pseudo_ptr(index);}
		operator T&() const{return *(pool::get_instance().v+index);}
		/*const*/ pseudo_ref& operator=(const T& t){
			pool::get_instance().v[index]=t;
			return *this;
		}
	};
	typedef random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef size_t difference_type;
	typedef pseudo_ptr pointer;
	typedef pseudo_ref reference;
	INDEX index;
	/*explicit*/ pseudo_ptr(INDEX index):index(index){}
	reference operator*()const{return reference(index);}
	T* operator->()const{return pool::get_instance().v+index;}
	pseudo_ptr& operator+=(const size_t s){index+=s;return *this;}
	pseudo_ptr& operator-=(const size_t s){index-=s;return *this;}
	pseudo_ptr& operator++(){++index;return *this;}
	pseudo_ptr& operator--(){--index;return *this;}
	bool operator!=(const pseudo_ptr& p)const{return index!=p.index;}
	bool operator<(const pseudo_ptr& p)const{return index<p.index;}
	bool operator==(const pseudo_ptr& p)const{return index==p.index;}
	operator bool() const{return index;}
};
template<typename T> pseudo_ptr<T> operator+(const pseudo_ptr<T>& a,size_t s){
	pseudo_ptr<T> tmp=a;
	return tmp+=s;
}
template<typename T> pseudo_ptr<T> operator-(const pseudo_ptr<T>& a,size_t s){
	pseudo_ptr<T> tmp=a;
	return tmp-=s;
}
template<typename T> ptrdiff_t operator-(const pseudo_ptr<T>& a,const pseudo_ptr<T>& b){
	return a.index-b.index;
}

template<typename T> struct custom_allocator{
	typedef T value_type;
	typedef pseudo_ptr<T> pointer;
	typedef pointer const_pointer;
	typedef typename pseudo_ptr<T>::pseudo_ref reference;
	typedef reference const_reference;
	typedef std::size_t    size_type;
	typedef std::ptrdiff_t difference_type;

	pointer address(reference value) const{return &value;}
	/*
	const_pointer address (const_reference value) const {
	   return &value;
	}
	*/
	template<class U> struct rebind{typedef custom_allocator<U> other;};
	size_type max_size() const throw() {return N;}
	pointer allocate (size_type num, const void* = 0) {
		std::cerr << "allocate " << num << " element(s)" << " of size " << sizeof(T) << std::endl;
		pointer ret = pointer::pool::get_instance().allocate(num);
		std::cerr << " allocated at: " << ret.operator->() << std::endl;
		return ret;
	}
	void construct(pointer p,T value){
		new(p.operator->())T(value);
	}
	void destroy (pointer p) {
		p->~T();
	}
	void deallocate (pointer p, size_type num) {
		std::cerr << "deallocate " << num << " element(s)" << " of size " << sizeof(T) << " at: " << p.operator->() << std::endl;
		pointer::pool::get_instance().deallocate(p);
	}
};
#endif
