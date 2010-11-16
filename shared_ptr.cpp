#include "shared_ptr.h"
namespace objrdf{
	template<typename T> shared_ptr<T>::shared_ptr():t(0){}
	template<typename T> shared_ptr<T>::shared_ptr(T* _t){
		t=_t;
		if(t) ++t->n;
	}
	template<typename T> inline T* shared_ptr<T>::operator->() const{return t;}
	template<typename T> inline T* shared_ptr<T>::get() const{return t;}
	template<typename T> inline T& shared_ptr<T>::operator*() const{return *t;}
	//we need this one!!!
	template<typename T> shared_ptr<T>::shared_ptr(shared_ptr const & s){
		t=s.t;
		if(t) ++(t->n);
	}
	//S must derive from T
	template<typename T> template<typename S> shared_ptr<T>::shared_ptr(const shared_ptr<S> & s){
		//t=s.t;
		t=s.operator->();
		if(t) ++(t->n);
	}
	template<typename T> shared_ptr<T>& shared_ptr<T>::operator=(const shared_ptr& s){
		//should check if it is not the same
		if(t){
			--(t->n);
			if(t->n==0) delete t;
		}
		t=s.t;
		if(t) ++(t->n);
		return *this;
	}
	template<typename T> template<typename S> shared_ptr<T>& shared_ptr<T>::operator=(const shared_ptr<S>& s){
		if(t){
			--(t->n);
			if(t->n==0) delete t;
		}
		//t=s.t;
		t=s.operator->();
		if(t) ++(t->n);
		return *this;
	}
	template<typename T> bool shared_ptr<T>::operator==(const shared_ptr<T> & s) const{return t==s.t;}
	template<typename T> bool shared_ptr<T>::operator!=(const shared_ptr<T> & s) const{return t!=s.t;}
	template<typename T> bool shared_ptr<T>::operator!() const{return t==0;}
	template<typename T> shared_ptr<T>::~shared_ptr(){
		if(t){
			--(t->n);
			if(t->n==0) delete t;
		}
	}
}
