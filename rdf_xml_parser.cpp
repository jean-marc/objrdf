#include "rdf_xml_parser.h"
#include <sstream>
#include <algorithm>
#include "reification.h"
using namespace objrdf;
/*
 *	how do we parse XML content?, it is not valid RDF, is it the responsibility of the property to properly parse?
 *
 *
 */
RESOURCE_PTR construct(uri u){
	base_resource::allocator_type a;
	auto p=a.allocate(1);
	a.construct(p,u);
	return p;
}
rdf_xml_parser::rdf_xml_parser(std::istream& is,PROVENANCE p):xml_parser<rdf_xml_parser>(is),placeholder(construct(uri("??"))),current_property(end(placeholder)),p(p){
	string_property=false;
	st.push(placeholder);
}
bool rdf_xml_parser::go(){
	/*
 	*	a simpler way might be to keep a list of subjects and property that are not satified yet,
 	*	once the object is created those subjects will be updated, no need to remove properties
 	*/ 
	bool r=xml_parser<rdf_xml_parser>::go();
	//we need to remove dangling pointers
	for(auto i=missing_object.begin();i!=missing_object.end();++i){
		LOG<<"removing statement `"<<i->second.subject->id<<"',`"<<i->second.get_Property()->id<<"',`"<<i->first<<"'"<<endl;
		//first let's build a base_resource::type_iterator from base_resource::instance_iterator
		base_resource::type_iterator j(i->second.subject,i->second.i);
		//then let's iterate through all instances
		//let's define a lambda to give us the first iterator 
		auto func=[](base_resource::instance_iterator first,base_resource::instance_iterator last){
			for (; first != last; ++first) {
				if (first->get_const_object() == nullptr) {
					return first;
				}
			}
			return last;
		};
		//erase entries one at a time
		for(auto k=func(j->begin(),j->end());k!=j->end();k=func(j->begin(),j->end())){
			//cerr<<(k->get_const_object()==nullptr)<<endl;
			erase(i->second.subject,k);
		}
	}
	return r;
};
bool rdf_xml_parser::start_resource(uri name,ATTRIBUTES att){//use ATTRIBUTES& to spare a map copy?
	LOG<<"start resource "<<name<<endl;
	if(current_property!=end(st.top())){
		#ifdef NATIVE
		if(current_property->get_Property()->cget<rdfs::range>().t->id==name && !current_property->constp()){
		#else
		if(current_property->get_Property()->cget<rdfs::range>()->id==name && !current_property->constp()){
		#endif
			//assert(current_property->get_Property()->cget<rdfs::range>()->constructor());
			#ifdef NATIVE
			auto cl=current_property->get_Property()->cget<rdfs::range>().t;
			#else
			auto cl=current_property->get_Property()->cget<rdfs::range>();
			#endif
			auto j=att.find(rdf::ID);
			//if resource stored inside property, do not allocate memory but constructor
			RESOURCE_PTR r;
			#ifdef NEW_FUNC_TABLE
			if((*current_property).t.set_object_generic){
			#else
			if((*current_property).t.set_object){
			#endif
				/*RESOURCE_PTR*/ r=(j!=att.end()) ? create_by_type(cl,uri(j->second)):create_by_type_blank(cl);
				auto k=att.find(rdf::nodeID);
				if(k!=att.end()){
					//there might be reference to that blank node but the ID is different now	
					blank_node[k->second]=r;	
				}
				current_property->add_property(p)->set_object(r);
				set_missing_object(r);
			}else{
				r=current_property->add_property(p)->get_object();	
				cl->t.ctor(r,(j!=att.end()) ? uri(j->second):get_uri(r));
			}
			st.push(r);
			for(base_resource::type_iterator i=begin(r);i!=end(r);++i){
				ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
				if(j!=att.end()){
					istringstream is(j->second);
					i->add_property(p)->in(is);
				}
			}
		}else if(name==rdf::Description){
			/*
 			*	not finished yet, more work needed
 			*/ 
			ATTRIBUTES::iterator i=att.find(rdf::about);
			if(i!=att.end()){
				//could be a local resource: starts with `#'
				RESOURCE_PTR subject=find(uri::hash_uri(i->second));
				if(subject){
					st.push(subject);
					for(base_resource::type_iterator i=begin(subject);i!=end(subject);++i){
						ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
						if(j!=att.end()){
							istringstream is(j->second);
							i->add_property(p)->in(is);
						}
					}
				}else{
					ERROR_PARSER<<"resource `"<<i->second<<"' not found"<<endl;
					ERROR_PARSER<<"un-typed resource"<<endl;
					//the id needs to be set at construction time
					/*
					placeholder->id=uri::hash_uri(i->second);
					st.push(placeholder);
					*/
					st.push(construct(uri::hash_uri(i->second)));
					
				}
				
			}else{
				ATTRIBUTES::iterator i=att.find(rdf::ID);
				if(i!=att.end()){
					st.push(construct(uri(i->second)));
				}else{
					ERROR_PARSER<<"anonymous resource"<<endl;
					st.push(placeholder);
				}
				ERROR_PARSER<<"un-typed resource"<<endl;
				//LOG<<"placeholder ";st.top()->id.to_uri(cerr); cerr<<endl;
				/*
				placeholder->id=uri();//reset
				ATTRIBUTES::iterator i=att.find(rdf::ID);
				if(i!=att.end()){
					//placeholder->id=uri::hash_uri(i->second);
					placeholder->id=uri(i->second);
				}else{
					//could also be rdf:about
					ATTRIBUTES::iterator i=att.find(rdf::about);
					if(i!=att.end())
						placeholder->id=uri::hash_uri(i->second);
					else
						ERROR_PARSER<<"anonymous resource"<<endl;
				}
				ERROR_PARSER<<"un-typed resource"<<endl;
				cerr<<"placeholder ";placeholder->id.to_uri(cerr); cerr<<endl;
				st.push(placeholder);
				*/
			}
		}else{//could be a sub-class
			RESOURCE_PTR r=find(name);
			if(r){
				if(get_class(r)==rdfs::Class::get_class()){
					#ifdef NATIVE
					CONST_CLASS_PTR c=static_cast<CONST_CLASS_PTR>(r);
					if(is_subclass(c,current_property->get_Property()->cget<rdfs::range>().t)){
					#else
					CONST_CLASS_PTR c(r);
					if(is_subclass(c,current_property->get_Property()->cget<rdfs::range>())){
					#endif
						RESOURCE_PTR r=create_by_type(c,uri(att[rdf::ID]));
						//LOG<<"new resource:"<<r->id<<endl;
						//r->id=att[rdf::ID];
						current_property->add_property(p)->set_object(r);
						st.push(r);
						for(base_resource::type_iterator i=begin(r);i!=end(r);++i){
							ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
							if(j!=att.end()){
								istringstream is(j->second);
								i->add_property(p)->in(is);
							}
						}
					}else{
						#ifdef NATIVE
						ERROR_PARSER<<name<<" not a sub-class of "<<current_property->get_Property()->cget<rdfs::range>().t->id<<endl;
						#else
						ERROR_PARSER<<name<<" not a sub-class of "<<current_property->get_Property()->cget<rdfs::range>()->id<<endl;
						#endif
						st.push(placeholder);
					}
				}else{
					ERROR_PARSER<<"resource `"<<name<<"' is not a rdfs:Class\n";
					st.push(placeholder);
				}
			}else{
				ERROR_PARSER<<"resource `"<<name<<"' not found\n";
				st.push(placeholder);
			}
		}
	}else{
		if(name==rdf::Description){
			/*
 			*	3 scenarios:
 			*		rdf:ID	must be local resource
 			*		rdf:about
 			*			local resource not created yet (should be able to tell by the URI used)
 			*			external resource
 			*/ 
			ATTRIBUTES::iterator i=att.find(rdf::about);
			if(i!=att.end()){
				//could be a local resource: starts with `#'
				RESOURCE_PTR subject=find(uri::hash_uri(i->second));
				if(subject){
					st.push(subject);
					for(base_resource::type_iterator i=begin(subject);i!=end(subject);++i){
						ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
						if(j!=att.end()){
							istringstream is(j->second);
							i->add_property(p)->in(is);
						}
					}
				}else{
					ERROR_PARSER<<"resource `"<<i->second<<"' not found"<<endl;
					ERROR_PARSER<<"un-typed resource"<<endl;
					//placeholder->id=uri::hash_uri(i->second);
					//st.push(placeholder);
					st.push(construct(uri::hash_uri(i->second)));
				}
				
			}else{
				//placeholder->id=uri();//reset
				ATTRIBUTES::iterator i=att.find(rdf::ID);
				if(i!=att.end()){
					st.push(construct(uri(i->second)));
					//placeholder->id=uri::hash_uri(i->second);
					//placeholder->id=uri(i->second);
				}else{
					//could also be rdf:about
					//ATTRIBUTES::iterator i=att.find(rdf::about);
					//if(i!=att.end())
					//	placeholder->id=uri::hash_uri(i->second);
					//else
					ERROR_PARSER<<"anonymous resource"<<endl;
					st.push(placeholder);
				}
				ERROR_PARSER<<"un-typed resource"<<endl;
				//cerr<<"placeholder ";st.top()->id.to_uri(cerr); cerr<<endl;
				//st.push(placeholder);
			}
		}else{
			/*
 			*	missing case where rdf:about present!
 			*/ 
			ATTRIBUTES::iterator i=att.find(rdf::about);
			if(i!=att.end()){
				RESOURCE_PTR subject=find(uri::hash_uri(i->second));
				if(subject){
					st.push(subject);
					for(base_resource::type_iterator i=begin(subject);i!=end(subject);++i){
						ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
						if(j!=att.end()){
							istringstream is(j->second);
							i->add_property(p)->in(is);
						}
					}
				}else{
					CONST_CLASS_PTR r=find_t<rdfs::Class>(name);
					if(r){
						RESOURCE_PTR subject=create_by_type(r,uri::hash_uri(i->second));
						set_missing_object(subject);
						st.push(subject);
						for(base_resource::type_iterator i=begin(subject);i!=end(subject);++i){
							ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
							if(j!=att.end()){
								istringstream is(j->second);
								i->add_property(p)->in(is);
							}
						}
					}else{
						ERROR_PARSER<<"Class `"<<name<<"' not found"<<endl;
						st.push(construct(uri::hash_uri(i->second)));
					}
				}
			}else{
				//ATTRIBUTES::iterator i=att.find(rdf::ID);
				//if(i!=att.end()){
					CONST_CLASS_PTR r=find_t<rdfs::Class>(name);
					if(r){
						ATTRIBUTES::iterator i=att.find(rdf::ID);
						RESOURCE_PTR subject=(i!=att.end()) ? create_by_type(r,uri::hash_uri(i->second)) : create_by_type_blank(r);
						auto k=att.find(rdf::nodeID);
						if(k!=att.end()){
							//there might be reference to that blank node but the ID is different now	
							blank_node[k->second]=subject;	
						}
						set_missing_object(subject);
						st.push(subject);
						for(base_resource::type_iterator i=begin(subject);i!=end(subject);++i){
							ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
							if(j!=att.end()){
								istringstream is(j->second);
								i->add_property(p)->in(is);
							}
						}
					}else{
						ERROR_PARSER<<"Class `"<<name<<"' not found"<<endl;
						st.push(placeholder);
						//st.push(RESOURCE_PTR::construct(uri::hash_uri(i->second)));
					}

				//}else{
					//st.push(placeholder);
				//}
			}
			/*
			shared_ptr<base_resource> r=doc.find(name);
			if(r&&r->get_Class()==rdfs::Class::get_class().get()){
				rdfs::Class& c=*static_cast<rdfs::Class*>(r.get());
				assert(c.f);
				shared_ptr<base_resource> subject(c.f(uri(att[rdf::ID])));
				//LOG<<"new resource:"<<subject->id<<endl;
				//subject->id=att[rdf::ID];
				set_missing_object(subject);
				st.push(subject);
				doc.insert(subject);
				///
				for(base_resource::type_iterator i=subject->begin();i!=subject->end();++i){
					ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
					if(j!=att.end()){
						istringstream is(j->second);
						i->add_property()->in(is);
					}
				}
			}else{
				ERROR_PARSER<<"Class `"<<name<<"' not found"<<endl;
				st.push(placeholder);
			}*/
		}
	}
	return true;
}
/*
 *	
 */
void rdf_xml_parser::set_missing_object(RESOURCE_PTR object){
	//need to distinguish blank nodes
	LOG<<"missing object:"<</*(void*)object.get()<<"\t`"<<*/object->id<<"'"<<endl;
	auto first=missing_object.find(object->id),last=missing_object.upper_bound(object->id);
	LOG<<(first==missing_object.end())<<"\t"<<(last==missing_object.end())<<"\t"<<(first==last)<<endl;
	if(first!=missing_object.end()){//why do we need that???
		for(auto i=first;i!=last;++i){
			LOG<<"setting property `"<<i->second->get_Property()->id<<"' of resource `"<<i->second.subject->id<<"' "<<endl;
			i->second->set_object(object);//???
		}
		//need to clean up
		missing_object.erase(first,last);
	}
} 
bool rdf_xml_parser::end_resource(uri name){
	LOG<<"end resource "<<name<<endl;
	st.top()->end_resource();
	st.pop();
	current_property=end(placeholder);
	return true;
}
bool rdf_xml_parser::start_property(uri name,ATTRIBUTES att){
	LOG<<"start property "<<name<<endl;
	current_property=std::find_if(begin(st.top()),end(st.top()),name_p(name));
	if(current_property!=end(st.top())){
		if(current_property.constp()){
			ERROR_PARSER<<"property const"<<endl;
		}else if(current_property.literalp()){
			#ifdef NATIVE
			if(current_property->get_Property()->cget<rdfs::range>().t==xsd::String::get_class()||
			   current_property->get_Property()->cget<rdfs::range>().t==xsd::anyURI::get_class()){//if the RANGE is string it could consume the next `<'
			#else
			if(current_property->get_Property()->cget<rdfs::range>()==xsd::String::get_class()||
			   current_property->get_Property()->cget<rdfs::range>()==xsd::anyURI::get_class()){//if the RANGE is string it could consume the next `<'
			#endif
				string_property=true;
			}else{
				current_property->add_property(p)->in(is);
				if(!is.good()){
					ERROR_PARSER<<"wrong type"<<endl;
					is.clear();
				}
			}
		}else{
			//there could be an `http://www.w3.org/1999/02/22-rdf-syntax-ns#resource' attribute
			ATTRIBUTES::iterator i=att.find(rdf::resource);
			if(i!=att.end()){
				RESOURCE_PTR object=find(uri::hash_uri(i->second));
				if(object){
					current_property->add_property(p)->set_object(object);//NEED TO CHECK THE TYPE!!!!
				}else{
					//problem with current_property->add_property()
					auto pr=current_property->add_property(p);
					LOG<<"stowing away property `"<<pr.get_Property()->id<<"' of resource `"<<pr.subject->id<<"' "<<endl;
					//missing_object.insert(MISSING_OBJECT::value_type(uri::hash_uri(i->second),current_property->add_property(p)));
					missing_object.insert(MISSING_OBJECT::value_type(uri::hash_uri(i->second),pr));
					//ERROR_PARSER<<"resource "<<i->second<<" not found"<<endl;
				}
			}else{	
				ATTRIBUTES::iterator i=att.find(rdf::nodeID);
				if(i!=att.end()){
					auto j=blank_node.find(i->second);
					if(j!=blank_node.end()){
						current_property->add_property(p)->set_object(j->second);//NEED TO CHECK THE TYPE!!!!
					}else{
						ERROR_PARSER<<"reference to unknown blank node `"<<i->second<<"'"<<endl;	
					}
				}else{
					ERROR_PARSER<<"no attribute `rdf:resource' or `rdf:nodeID' present"<<endl;
				}
			}
		}
	}else if(get_class(st.top())==rdf::Statement::get_class()){
		rdf::Statement::allocator_type::pointer t(st.top());
		LOG<<"looking for statement..."<<endl;
		//need to look through all subclasses of rdf::Statement
		for(auto s:rdf::Statement::get_class()->cget<rdfs::Class::array_superClassOf>()){
			LOG<<s->id<<endl;
			pool_allocator::pool::POOL_PTR p(s.index,0); //there is a mapping between Class and pools
			if(p->iterable){
				for(auto j=pool_allocator::pool::cbegin<base_resource::allocator_type::pointer::CELL>(p);j!=pool_allocator::pool::cend<base_resource::allocator_type::pointer::CELL>(p);++j){
					to_rdf_xml(j,cerr);
					rdf::Statement::allocator_type::generic_pointer r(j);
					//let's compare subject/predicate/object
					if((r->cget<rdf::subject>()==t->cget<rdf::subject>())&&
						(r->cget<rdf::predicate>()==t->cget<rdf::predicate>())&&
						(r->cget<rdf::object>()==t->cget<rdf::object>()))
					{
						//now we can delete current statement and replace with this one
						//but we have to remove from index first
						rdf::Statement::allocator_type a;
						rdf::Statement::get_index().erase(t->id);
						a.destroy(t);
						a.deallocate(t,1);
						st.top()=r;
						LOG<<"swapping statement"<<endl;
						return start_property(name,att);
					}
				}
				ERROR_PARSER<<"\nno matching statement found"<<endl;

			}else{
				ERROR_PARSER<<"pool not iterable"<<endl;
			}
		}
	}else{
		ERROR_PARSER<<"property `"<<name<<"' not found"<<endl;
		if((name==rdf::type::get_property()->id)&&(get_class(st.top())==base_resource::get_class())){
			/*
 			*	we have a generic resource, we can maybe swap it for a typed resource
 			*	all XML attributes have been lost
 			*/ 
			ATTRIBUTES::iterator j=att.find(rdf::resource);
			if(j!=att.end()){
				RESOURCE_PTR r=find(uri::hash_uri(j->second));
				if(r&&get_class(r)==rdfs::Class::get_class()){
					RESOURCE_PTR subject=create_by_type(CONST_CLASS_PTR(r),st.top()->id);
					//LOG<<"new resource:"<<subject.get()<<endl;
					set_missing_object(subject);
					st.pop();
					st.push(subject);
				}else{
					ERROR_PARSER<<"resource `"<<j->second<<"' not found"<<endl;
				}
			}else{
				ERROR_PARSER<<rdf::resource<<" must be present"<<endl;
			}
		}else{
			ERROR_PARSER<<"property `"<<name<<"' not found"<<endl;
		}
	}
	return true;
}
bool rdf_xml_parser::end_property(uri name){
	string_property=false;
	LOG<<"end property "<<name<<endl;
	return true;
}
bool rdf_xml_parser::start_element(uri name,ATTRIBUTES att){
	if(depth==1) current_property=end(placeholder);
	if(depth) //?? what for
		return (depth&1) ? start_resource(name,att) : start_property(name,att); 
	return true;
}
bool rdf_xml_parser::end_element(uri name){
	if(depth>=0)	
		return (depth&1) ? end_property(name) : end_resource(name); 
	return true;
}
//there should ne multiple calls if the buffer fills up
bool rdf_xml_parser::characters(string s){
	LOG<<"characters "<<s<<endl;
	if(string_property){
		current_property->add_property(p)->set_string(s);
		/*istringstream is(s);
		current_property->add_property()->in(is);
		if(!is.good()){
			ERROR<<"wrong type"<<endl;
		}*/
	}
	return true;
}
