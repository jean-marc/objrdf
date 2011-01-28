#include "rdf_xml_parser.h"
#include <sstream>
#include <algorithm>
using namespace objrdf;
rdf_xml_parser::rdf_xml_parser(rdf::RDF& _doc,std::istream& is):xml_parser<rdf_xml_parser>(is),doc(_doc),current_property(base_resource::nil->end()){
	string_property=false;
	depth=-1;
	st.push(base_resource::nil);
}
bool rdf_xml_parser::go(){
	bool r=xml_parser<rdf_xml_parser>::go();
	for(MISSING_OBJECT::iterator i=missing_object.begin();i!=missing_object.end();++i)
		cerr<<"missing objects: `"<<i->first<<"'"<<endl;
	return r;
};
bool rdf_xml_parser::start_resource(string name,ATTRIBUTES att){//use ATTRIBUTES& to spare a map copy?
	//cerr<<"start resource "<<name<<endl;
	if(current_property!=st.top()->end()){
		if(current_property->get_Property()->get<rdfs::range>().get()->id==name){
			assert(current_property->get_Property()->get<rdfs::range>()->f);
			shared_ptr<base_resource> r=current_property->get_Property()->get<rdfs::range>()->f();
			LOG<<"new resource:"<<r.get()<<endl;
			r->id=att["id"];//should use uri's instead
			current_property->add_property()->set_object(r.get());
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
					i->add_property()->in(is);
				}
			}
		}else{//could be a sub-class
			//map<string,rdfs::Class*>::iterator i=m.find(name);
			//if(i!=m.end()){
			base_resource* r=doc.find(name);
			//alternatively only search rdfs::Class::get_instances() will work in most cases
			if(r&&r->get_Class()==rdfs::Class::get_class()){
				rdfs::Class& c=*static_cast<rdfs::Class*>(r);
				if(*current_property->get_Property()->get<rdfs::range>() < c){
					assert(c.f);
					shared_ptr<base_resource> r=c.f();
					LOG<<"new resource:"<<r.get()<<endl;
					r->id=att["id"];//should use uri's instead
					current_property->add_property()->set_object(r.get());
					st.push(r);
					doc.insert(r);
					for(base_resource::type_iterator i=r->begin();i!=r->end();++i){
						ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
						if(j!=att.end()){
							istringstream is(j->second);
							i->add_property()->in(is);
						}
					}
				}else{
					ERROR_PARSER<<name<<" not a sub-class of "<<current_property->get_Property()->get<rdfs::range>()->id<<endl;
					st.push(base_resource::nil);
				}
			}else{
				ERROR_PARSER<<"Class "<<name<<" not found\n";
				st.push(base_resource::nil);
			}
		}
	}else{
		if(name=="Description"){
			ATTRIBUTES::iterator i=att.find("id");
			if(i!=att.end()){
				/*
 				*	! this is not correct per RDF format, rdf:type is NOT a literal property,
 				*	we should remove all this code
 				*/ 
				ATTRIBUTES::iterator j=att.find("type");
				if(j!=att.end()){
					base_resource* r=doc.find(j->second);
					if(r&&r->get_Class()==rdfs::Class::get_class()){
						rdfs::Class& c=*static_cast<rdfs::Class*>(r);
						assert(c.f);
						shared_ptr<base_resource> subject=c.f();
						LOG<<"new resource:"<<subject.get()<<endl;
						subject->id=i->second;//should use uri's instead
						set_missing_object(subject);
						st.push(subject);
						doc.insert(subject);
						for(base_resource::type_iterator i=subject->begin();i!=subject->end();++i){
							//we should not go through `type' property
							ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
							if(j!=att.end()){
								istringstream is(j->second);
								i->add_property()->in(is);
							}
						}
					}else{
						ERROR_PARSER<<"Class `"<<j->second<<"' unknown"<<endl;
						st.push(base_resource::nil);
					}
				}else{
					ERROR_PARSER<<"type not given"<<endl;
					st.push(base_resource::nil);
				}
			}else{
				ATTRIBUTES::iterator i=att.find("about");
				if(i!=att.end()){
					base_resource* subject=doc.find(i->second);
					if(subject){
						st.push(subject);
						for(base_resource::type_iterator i=subject->begin();i!=subject->end();++i){
							ATTRIBUTES::iterator j=att.find(i->get_Property()->id);
							if(j!=att.end()){
								istringstream is(j->second);
								i->add_property()->in(is);
							}
						}
					}else{
						ERROR_PARSER<<"resource `"<<i->second<<"' not found"<<endl;
						st.push(base_resource::nil);
					}
					
				}else{
					ERROR_PARSER<<"anonymous/un-typed resource"<<endl;
					st.push(base_resource::nil);
				}
			}	
		}else{
			base_resource* r=doc.find(name);
			if(r&&r->get_Class()==rdfs::Class::get_class()){
				rdfs::Class& c=*static_cast<rdfs::Class*>(r);
				assert(c.f);
				shared_ptr<base_resource> subject=c.f();
				LOG<<"new resource:"<<subject.get()<<endl;
				subject->id=att["id"];//should use uri's instead
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
				st.push(base_resource::nil);
			}
		}
	}
	return true;
}
void rdf_xml_parser::set_missing_object(shared_ptr<base_resource> object){
	if(!object->id.empty()){
		MISSING_OBJECT::iterator first=missing_object.find(object->id),last=missing_object.upper_bound(object->id);
		for(MISSING_OBJECT::iterator i=first;i!=last;++i)
			i->second->set_object(object.get());//???
		//need to clean up
		missing_object.erase(first,last);
	}
} 
bool rdf_xml_parser::end_resource(string name){
	//cerr<<"end resource "<<name<<endl;
	st.top()->end_resource();
	st.pop();
	current_property=base_resource::nil->end();
	return true;
}
bool rdf_xml_parser::start_property(string name,ATTRIBUTES att){
	//cerr<<"start property "<<name<<endl;
	current_property=std::find_if(st.top()->begin(),st.top()->end(),namep(name));
	if(current_property!=st.top()->end()){
		if(current_property.literalp()){
			if(current_property->get_Property()->get<rdfs::range>()!=String::get_class()){//if the RANGE is string it could consume the next `<'
				current_property->add_property()->in(is);
				if(!is.good()){
					ERROR_PARSER<<"wrong type"<<endl;
					is.clear();
				}
			}else{
				string_property=true;
			}
		}else{
			//there could be an `rdf:resource' attribute
			ATTRIBUTES::iterator i=att.find("resource");
			if(i!=att.end()){
				base_resource* object=doc.find(i->second);
				if(object){
					current_property->add_property()->set_object(object);//NEED TO CHECK THE TYPE!!!!
				}else{
					missing_object.insert(MISSING_OBJECT::value_type(i->second,current_property->add_property()));
					//ERROR_PARSER<<"resource "<<i->second<<" not found"<<endl;
				}
			}	

		}
	}else{
		ERROR_PARSER<<"property "<<name<<" not found"<<endl;
	}
	return true;
}
bool rdf_xml_parser::end_property(string name){
	string_property=false;
	//cerr<<"end property "<<name<<endl;
	return true;
}
bool rdf_xml_parser::start_element(string name,ATTRIBUTES att){
	++depth;
	if(depth)
		return (depth&1) ? start_resource(name,att) : start_property(name,att); 
	return true;
}
bool rdf_xml_parser::end_element(string name){
	--depth;
	if(depth>=0)	
		return (depth&1) ? end_property(name) : end_resource(name); 
	return true;
}
//there should ne multiple calls if the buffer fills up
bool rdf_xml_parser::characters(string s){
	//cerr<<"characters "<<s<<endl;
	if(string_property){
		current_property->add_property()->set_string(s);
		/*istringstream is(s);
		current_property->add_property()->in(is);
		if(!is.good()){
			ERROR<<"wrong type"<<endl;
		}*/
	}
	return true;
}
