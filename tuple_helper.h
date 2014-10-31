#ifndef TUPLE_HELPER_H
#define TUPLE_HELPER_H
/*
 *	extra function to help tuple handling
 *	let's put in different namespace to be safe
 */
namespace objrdf{
#ifdef __GNUG__
	template<typename _T,typename _Tp> struct tuple_index{
		enum{value=0};//we have reached end of tuple shouldn't the value be >0???
	};
	template<typename _T,typename _Head,typename... _Tail> struct tuple_index<_T, std::tuple<_Head, _Tail...> >{
		enum{value=1+tuple_index<_T,std::tuple<_Tail...> >::value};
	};
	template<typename _Head, typename... _Tail> struct tuple_index<_Head, std::tuple<_Head, _Tail...> >{
		enum{value=0};
	};
	template<typename _Tp,typename F> struct static_for_each_impl{
		static F go(F f){return f;}//no-op
	};
	template<typename _Head,typename... _Tail,typename F> struct static_for_each_impl<std::tuple<_Head,_Tail...>,F>{
		static F go(F f){
			f.template operator()<_Head>();
			return static_for_each_impl<std::tuple<_Tail...>,F>::go(f);
		}
	};
	template<typename _Tp,typename F> F static_for_each(F f){return static_for_each_impl<_Tp,F>::go(f);}
	template<
		typename _T,
		typename _Tp
	> struct in_tuple{
		enum{value=tuple_index<_T,_Tp>::value < std::tuple_size<_Tp>::value};
	};
#else
#if 0
	/*
 	*	this does not well in MSVC because of recursive template instantiation
 	*	note: there might be smarter way to iterate through tuple but the 10 element
 	*	limit is a major problem
 	*/ 
	template<
		typename _T,
		typename _Tp
	> struct tuple_index;/*{
		enum{value=0};//we have reached end of tuple shouldn't the value be >0???
	};*/
	template<typename _T,
		typename A,typename B,typename C,typename D,typename E,
		typename F,typename G,typename H,typename I,typename J
	> struct tuple_index<_T,tuple<A,B,C,D,E,F,G,H,I,J>>{
		enum{value=1+tuple_index<_T,tuple<B,C,D,E,F,G,H,I,J,tr1::_Nil>>::value};
	};
	template<
		typename _T
	> struct tuple_index<_T,tuple<tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil>>{
		enum{value=0};
	};
	template<
		typename A,typename B,typename C,typename D,typename E,
		typename F,typename G,typename H,typename I,typename J
	> struct tuple_index<A,tuple<A,B,C,D,E,F,G,H,I,J>>{
		enum{value=0};
	};
	template<typename _Tp,typename F> struct static_for_each_impl;/*{
		static F go(F f){return f;}//no-op
	};*/
	template<
		typename A,typename B,typename C,typename D,typename E,
		typename F,typename G,typename H,typename I,typename J,
		typename FUNC
	> struct static_for_each_impl<tuple<A,B,C,D,E,F,G,H,I,J>,FUNC>{
		static FUNC go(FUNC f){
			//f.template operator()<A>();
			f.operator()<A>();
			return static_for_each_impl<tuple<B,C,D,E,F,G,H,I,J,tr1::_Nil>,FUNC>::go(f);
			//return f;
		}
	};
	template<
		typename FUNC
	> struct static_for_each_impl<tuple<tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil,tr1::_Nil>,FUNC>{
		static FUNC go(FUNC f){return f;}
	};
#endif
#endif
	

}
#endif
