#include "rdf_xml_parser.h"
#include <sstream>
#include <algorithm>
using namespace objrdf;
rdf_xml_parser::rdf_xml_parser(rdf::RDF& doc,std::istream& is,generic_property::PROVENANCE p):xml_parser<rdf_xml_parser>(is),doc(doc),placeholder(new base_resource(uri("??"))),current_property(placeholder->end()),p(p){
	string_property=false;
	st.push(placeholder);
}
bool rdf_xml_parser::go(){
	bool r=xml_parser<rdf_xml_parser>::go();
	for(MISSING_OBJECT::iterator i=missing_object.begin();i!=missing_object.end();++i)
		cerr<<"missing objects: `"<<i->first<<"'"<<endl;
	return r;
};
bool rdf_xml_parser::start_resource(uri name,ATTRIBUTES att){//use ATTRIBUTES& to spare a map copy?
	//cerr<<"start resource "<<name<<endl;
	if(current_property!=st.top()->end()){
		if(current_property->get_Property()->get<rdfs::range>().get()->id==name){
			assert(current_property->get_Property()->get<rdfs::range>()->f);
			shared_ptr<base_resource> r=shared_ptr<base_resource>(current_property->get_Property()->get<rdfs::range>()->f(uri(att[rdf::ID])));
			//LOG<<"new resource:"<<r->id<<endl;
			current_property->add_property(p)->set_object(r);
			set_missing_object(r);
			st.push(r);
			doc.insert(r);
			//process attributes
			/*
				2 methods: 
				
			*/
			for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
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
				shared_ptr<base_resource> subject=doc.find(uri::hash_uri(i->second));
				if(subject){
					st.push(subject);
					for(base_resource::type_iterator i=subject->begin();i!=subject->end();++i){
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
					st.push(shared_ptr<base_resource>(new base_resource(uri::hash_uri(i->second))));
				}
				
			}else{
				ATTRIBUTES::iterator i=att.find(rdf::ID);
				if(i!=att.end()){
					st.push(shared_ptr<base_resource>(new base_resource(uri(i->second))));
				}else{
					ERROR_PARSER<<"anonymous resource"<<endl;
					st.push(placeholder);
				}
				ERROR_PARSER<<"un-typed resource"<<endl;
				cerr<<"placeholder ";st.top()->id.to_uri(cerr); cerr<<endl;
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
			shared_ptr<base_resource> r=doc.find(name);
			//alternatively only search rdfs::Class::get_instances() will work in most cases
			if(r&&r->get_Class()==rdfs::Class::get_class().get()){
				rdfs::Class& c=*static_cast<rdfs::Class*>(r.get());
				if(*current_property->get_Property()->get<rdfs::range>() < c){
					assert(c.f);
					shared_ptr<base_resource> r=shared_ptr<base_resource>(c.f(uri(att[rdf::ID])));
					//LOG<<"new resource:"<<r->id<<endl;
					//r->id=att[rdf::ID];
					current_property->add_property(p)->set_object(r);
					st.push(r);
					doc.insert(r);
					for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
						ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
						if(j!=att.end()){
							istringstream is(j->second);
							i->add_property(p)->in(is);
						}
					}
				}else{
					ERROR_PARSER<<name<<" not a sub-class of "<<current_property->get_Property()->get<rdfs::range>()->id<<endl;
					st.push(placeholder);
				}
			}else{
				ERROR_PARSER<<"Class `"<<name<<"' not found\n";
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
				shared_ptr<base_resource> subject=doc.find(uri::hash_uri(i->second));
				if(subject){
					st.push(subject);
					for(base_resource::type_iterator i=subject->begin();i!=subject->end();++i){
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
					st.push(shared_ptr<base_resource>(new base_resource(uri::hash_uri(i->second))));
				}
				
			}else{
				//placeholder->id=uri();//reset
				ATTRIBUTES::iterator i=att.find(rdf::ID);
				if(i!=att.end()){
					st.push(shared_ptr<base_resource>(new base_resource(uri(i->second))));
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
				cerr<<"placeholder ";st.top()->id.to_uri(cerr); cerr<<endl;
				//st.push(placeholder);
			}
		}else{
			/*
 			*	missing case where rdf:about present!
 			*/ 
			ATTRIBUTES::iterator i=att.find(rdf::about);
			if(i!=att.end()){
				shared_ptr<base_resource> subject=doc.find(uri::hash_uri(i->second));
				if(subject){
					st.push(subject);
					for(base_resource::type_iterator i=subject->begin();i!=subject->end();++i){
						ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
						if(j!=att.end()){
							istringstream is(j->second);
							i->add_property(p)->in(is);
						}
					}
				}else{
					shared_ptr<rdfs::Class> r=doc.query_t<rdfs::Class>(name);
					if(r){
						assert(r->f);
						shared_ptr<base_resource> subject(r->f(uri::hash_uri(i->second)));
						set_missing_object(subject);
						st.push(subject);
						doc.insert(subject);
						for(base_resource::type_iterator i=subject->begin();i!=subject->end();++i){
							ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
							if(j!=att.end()){
								istringstream is(j->second);
								i->add_property(p)->in(is);
							}
						}
					}else{
						ERROR_PARSER<<"Class `"<<name<<"' not found"<<endl;
						st.push(shared_ptr<base_resource>(new base_resource(uri::hash_uri(i->second))));
					}
				}
			}else{
				ATTRIBUTES::iterator i=att.find(rdf::ID);
				if(i!=att.end()){
					shared_ptr<rdfs::Class> r=doc.query_t<rdfs::Class>(name);
					if(r){
						assert(r->f);
						shared_ptr<base_resource> subject(r->f(uri::hash_uri(i->second)));
						set_missing_object(subject);
						st.push(subject);
						doc.insert(subject);
						for(base_resource::type_iterator i=subject->begin();i!=subject->end();++i){
							ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
							if(j!=att.end()){
								istringstream is(j->second);
								i->add_property(p)->in(is);
							}
						}
					}else{
						ERROR_PARSER<<"Class `"<<name<<"' not found"<<endl;
						st.push(shared_ptr<base_resource>(new base_resource(uri::hash_uri(i->second))));
					}

				}else{
					st.push(placeholder);
				}
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
void rdf_xml_parser::set_missing_object(shared_ptr<base_resource> object){
	//something fishy here
	//for(MISSING_OBJECT::iterator i=missing_object.begin();i!=missing_object.end();++i)
		//cerr<<"\tmissing objects: `"<<i->first<<"'"<<endl;
	LOG<<"missing object:"<<object.get()<<"\t`"<<object->id<<"'"<<endl;
	MISSING_OBJECT::iterator first=missing_object.find(object->id),last=missing_object.upper_bound(object->id);
	cerr<<(first==missing_object.end())<<"\t"<<(last==missing_object.end())<<"\t"<<(first==last)<<endl;
	if(first!=missing_object.end()){//why do we need that???
		for(MISSING_OBJECT::iterator i=first;i!=last;++i){
			LOG<<"setting object of resource `"<<i->first<<"'"<<endl;
			i->second->set_object(object);//???
		}
		//need to clean up
		missing_object.erase(first,last);
	}
} 
bool rdf_xml_parser::end_resource(uri name){
	//cerr<<"end resource "<<name<<endl;
	st.top()->end_resource();
	st.pop();
	current_property=placeholder->end();
	return true;
}
bool rdf_xml_parser::start_property(uri name,ATTRIBUTES att){
	//cerr<<"start property "<<name<<endl;
	current_property=std::find_if(st.top()->begin(),st.top()->end(),namep(name));
	if(current_property!=st.top()->end()){
		if(current_property.literalp()){
			if(current_property->get_Property()->get<rdfs::range>()!=xsd::String::get_class()){//if the RANGE is string it could consume the next `<'
				current_property->add_property(p)->in(is);
				if(!is.good()){
					ERROR_PARSER<<"wrong type"<<endl;
					is.clear();
				}
			}else{
				string_property=true;
			}
		}else{
			//there could be an `http://www.w3.org/1999/02/22-rdf-syntax-ns#resource' attribute
			ATTRIBUTES::iterator i=att.find(rdf::resource);
			if(i!=att.end()){
				shared_ptr<base_resource> object=doc.find(uri::hash_uri(i->second));
				if(object){
					current_property->add_property(p)->set_object(object);//NEED TO CHECK THE TYPE!!!!
				}else{
					//problem with current_property->add_property()
					missing_object.insert(MISSING_OBJECT::value_type(uri::hash_uri(i->second),current_property->add_property(p)));
					//ERROR_PARSER<<"resource "<<i->second<<" not found"<<endl;
				}
			}	

		}
	}else{
		if((name==rdf::type::get_property()->id)&&(st.top()->get_Class()==base_resource::get_class().get())){
			/*
 			*	we have a generic resource, we can maybe swap it for a typed resource
 			*	all XML attributes have been lost
 			*/ 
			ATTRIBUTES::iterator j=att.find(rdf::resource);
			if(j!=att.end()){
				shared_ptr<base_resource> r=doc.find(uri::hash_uri(j->second));
				if(r&&r->get_Class()==rdfs::Class::get_class().get()){
					rdfs::Class& c=*static_cast<rdfs::Class*>(r.get());
					assert(c.f);
					shared_ptr<base_resource> subject(c.f(st.top()->id));
					//LOG<<"new resource:"<<subject.get()<<endl;
					set_missing_object(subject);
					//subject->id=st.top()->id;
					st.pop();
					st.push(subject);
					doc.insert(subject);
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
	//cerr<<"end property "<<name<<endl;
	return true;
}
bool rdf_xml_parser::start_element(uri name,ATTRIBUTES att){
	//something fishy
	if(depth==1) current_property=placeholder->end();

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
	//cerr<<"characters "<<s<<endl;
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
