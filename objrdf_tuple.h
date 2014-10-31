/*
 *	MSVC 2010 has a pretty poor tuple implementation, limited
 *	to 10 elements and expensive to iterate and look up by type
 */
#ifndef OBJRDF_TUPLE_H
#define OBJRDF_TUPLE_H
#include <cstddef>
namespace objrdf{
	struct Nil;
	template<typename A,typename B=Nil> struct duo:B{
		typedef B NEXT;
		A a;
	};
	template<typename A> struct duo<A,Nil>{
		typedef Nil NEXT;
		A a;
	};
	template<
		typename A =Nil,typename B =Nil,typename C =Nil,typename D =Nil,typename E =Nil,typename F =Nil,typename G =Nil,typename H =Nil,typename I =Nil,typename J =Nil,
		typename AA=Nil,typename BA=Nil,typename CA=Nil,typename DA=Nil,typename EA=Nil,typename FA=Nil,typename GA=Nil,typename HA=Nil,typename IA=Nil,typename JA=Nil,
		typename AB=Nil,typename BB=Nil,typename CB=Nil,typename DB=Nil,typename EB=Nil,typename FB=Nil,typename GB=Nil,typename HB=Nil,typename IB=Nil,typename JB=Nil,
		typename AC=Nil,typename BC=Nil,typename CC=Nil,typename DC=Nil,typename EC=Nil,typename FC=Nil,typename GC=Nil,typename HC=Nil,typename IC=Nil,typename JC=Nil
	> struct tuple:duo<A,typename tuple<
				   B ,C ,D ,E ,F ,G ,H ,I ,J ,
				AA,BA,CA,DA,EA,FA,GA,HA,IA,JA,
				AB,BB,CB,DB,EB,FB,GB,HB,IB,JB,
				AC,BC,CC,DC,EC,FC,GC,HC,IC,JC
				>::IMPL>{ //implemented as duo<A,duo<B,duo<C,duo<D,Nil>>>>
		typedef duo<A,typename tuple<
				   B ,C ,D ,E ,F ,G ,H ,I ,J ,
				AA,BA,CA,DA,EA,FA,GA,HA,IA,JA,
				AB,BB,CB,DB,EB,FB,GB,HB,IB,JB,
				AC,BC,CC,DC,EC,FC,GC,HC,IC,JC
		>::IMPL> IMPL;
	};
	template<> struct tuple<Nil>{
		typedef Nil IMPL;
	};
	template<typename TUPLE> struct tuple_size:tuple_size<typename TUPLE::IMPL>{};
	template<typename A,typename B> struct tuple_size<duo<A,B>>{
		enum{value=1+tuple_size<typename duo<A,B>::NEXT>::value};
	};
	template<typename A> struct tuple_size<duo<A,Nil>>{
		enum{value=1};
	};
	template<std::size_t I,typename TUPLE> struct tuple_element:tuple_element<I,typename TUPLE::IMPL>{};
	template<typename A,typename B> struct tuple_element<0,duo<A,B>>{
		typedef A type;
		typedef duo<A,B> PATH;
	};
	template<std::size_t I,typename A,typename B> struct tuple_element<I,duo<A,B>>:tuple_element<I-1,typename duo<A,B>::NEXT>{};
	template<std::size_t I,typename TUPLE> typename tuple_element<I,TUPLE>::type& get(TUPLE& t){
		return ((typename tuple_element<I,TUPLE>::PATH&)t).a;
	};
	//non-standard	
	template<typename TUPLE,typename F> struct static_for_each_impl:static_for_each_impl<typename TUPLE::IMPL,F>{};
	template<typename A,typename B,typename F> struct static_for_each_impl<duo<A,B>,F>{
		static F go(F f){
			f.template operator()<A>();
			return static_for_each_impl<typename duo<A,B>::NEXT,F>::go(f);
		}
	};
	template<typename F> struct static_for_each_impl<Nil,F>{
		static F go(F f){return f;}
	};
	template<typename TUPLE,typename F> F static_for_each(F f){return static_for_each_impl<TUPLE,F>::go(f);}

	template<typename T,typename TUPLE> struct tuple_index:tuple_index<T,typename TUPLE::IMPL>{};
	template<typename T> struct tuple_index<T,Nil>{
		enum {value=0};
	};
	template<typename A,typename B> struct tuple_index<A,duo<A,B>>{
		enum {value=0};
	};
	template<typename T,typename A,typename B> struct tuple_index<T,duo<A,B>>{
		enum {value=1+tuple_index<T,typename duo<A,B>::NEXT>::value};
	};

	template<typename T,typename TUPLE> struct in_tuple{
		enum{value=tuple_index<T,TUPLE>::value < tuple_size<TUPLE>::value};
	};
}
#endif
	
