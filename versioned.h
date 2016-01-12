#ifndef VERSIONED_H
#define VERSIONED_H
#include <objrdf/objrdf.h>
/*
 *	test a special pointer that follows through to latest version
 *	of a resource
 */
namespace objrdf{
	template<
		typename NAMESPACE,
		typename NAME,
		typename RESOURCE,
		typename TRIGGER=std::tuple<>
	> struct versioned:resource<
		NAMESPACE,
		NAME,
		std::tuple<objrdf::next>,//do we want to type it?
		versioned<NAMESPACE,NAME,RESOURCE,TRIGGER>,
		RESOURCE,
		TRIGGER	//triggers? how can we express interest in all properties?
	>{
		typedef resource<NAMESPACE,NAME,std::tuple<objrdf::next>,versioned<NAMESPACE,NAME,RESOURCE,TRIGGER>,RESOURCE,TRIGGER> BASE;
		versioned(uri u):BASE(u){}	
		typename BASE::allocator_type::pointer update(){
			assert(BASE::template get<objrdf::next>()==nullptr);
			typename BASE::allocator_type a;
			auto p=a.allocate(1);
			a.construct(p,uri(string(BASE::id.local)+"@"));
			*p=*this;
			BASE::template get<objrdf::next>()=p;
			return p;
		}
		template<typename P> void set_p(P p,typename BASE::allocator_type::pointer){
			if(BASE::template get<objrdf::next>())
				return static_cast<typename BASE::allocator_type::pointer>(BASE::template get<objrdf::next>())->template set_p(p,nullptr);
			if(p!=BASE::template get<P>()) update()->template get<P>()=p;
		}
		template<typename P> void instead_of_delete(P&,typename BASE::allocator_type::pointer){

		}
		//maybe we should use a more explicit name like snapshot_pointer
		struct pointer:BASE::allocator_type::pointer{
			//could also modify destination 
			pointer(const typename BASE::allocator_type::pointer& p):BASE::allocator_type::pointer(
				p && p->get<objrdf::next>() ? static_cast<pointer>(p->get<objrdf::next>()) : p //recursive!
			){}
		};
		//use for snapshot so cannot be modified
		struct const_pointer:BASE::allocator_type::const_pointer{
			const_pointer(const typename BASE::allocator_type::const_pointer& p):BASE::allocator_type::const_pointer(
				//why do we need a `template' here but not above?
				p->template cget<objrdf::next>() ? static_cast<const_pointer>(p->get<objrdf::next>()) : p //recursive!
			){}
		};
		//moves history forward from subject
		static typename BASE::allocator_type::pointer _update(typename BASE::allocator_type::pointer head){
			assert(head->template get<objrdf::next>()==nullptr);//HEAD
			//there should at most be one call to update
			return static_for_each<typename RESOURCE::PROPERTIES>(visitor{head}).new_head;
			//we can use static_for_each and a well designed functor
			//the function table could be of some help or we could defined a static container with all the properties of interest
			//simpler to do in a generic way but we need to know the versioned<> type to access versioned<>::update
			//(it could also be implemented inside a special pointer)
			//iterate through members and find all versioned<>::allocator_type::pointer, would be nice to consolidate
			//all the changes
		}
		struct visitor{
			const typename BASE::allocator_type::pointer head;
			typename BASE::allocator_type::pointer new_head=nullptr;
		//we need a generic way to find versioned members (iterate), beware of cycles!
			template<typename PROPERTY,typename RDFS_RANGE> struct implementation{
				static void go(const typename BASE::allocator_type::pointer&,typename BASE::allocator_type::pointer&){}
			};//no op
			template<
				typename PROPERTY,
				typename OTHER_NAMESPACE,
				typename OTHER_NAME,
				typename OTHER_RESOURCE,
				typename OTHER_TRIGGER
			> struct implementation<PROPERTY,versioned<OTHER_NAMESPACE,OTHER_NAME,OTHER_RESOURCE,OTHER_TRIGGER>>{
				static void go(const typename BASE::allocator_type::pointer& head,typename BASE::allocator_type::pointer& new_head){
					typedef versioned<OTHER_NAMESPACE,OTHER_NAME,OTHER_RESOURCE,OTHER_TRIGGER> RDFS_RANGE;
					typename RDFS_RANGE::pointer tmp=head->template cget<PROPERTY>();//tmp now points to latest version or nullptr
					//tmp->_update();//recursive, should make sure no loop!!! maybe ahead now
					//_this->set_p<pb>(Bv::pointer(tmp->get<pb>()),tmp);
					//cout<<tmp->id<<endl;
				}
			};
			template<typename PROPERTY> void operator()(){implementation<PROPERTY,typename PROPERTY::RDFS_RANGE>::go(head,new_head);}
		};
	};
}
#endif
