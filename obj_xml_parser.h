#ifndef OBJ_XML_PARSER_H
#define OBJ_XML_PARSER_H
/*
 *	parse a single resource that follows the RDF model, there is no RDF doc	
 */
#include "objrdf.h"
#include "xml_parser.h"
#include <stack>
#include <map>
#include <algorithm>
namespace objrdf{
	struct obj_xml_parser:xml_parser<obj_xml_parser>{
		stack<shared_ptr<base_resource> > st;
		map<string,rdfs::Class*> m;
		base_resource::type_iterator current_property;
		int depth;
		bool start;
		obj_xml_parser(base_resource* r,istream& is):xml_parser<obj_xml_parser>(is),current_property(r->end()){
			depth=0;
			start=true;
			st.push(r);
		}
		//template<typename T> void register_class(){m[T::c.second->id]=T::c.second;}
		bool start_resource(string name){
			cerr<<"start resource "<<name<<endl;
			/*if(start){
				start=false;
			//if(current_property==base_resource::nil.end()){
				//will only happens once, but tested everytime to be improved
			}else*/ if(current_property!=st.top()->end()){
				assert(current_property->get_Property()->get<rdfs::range>().get());
				if(current_property->get_Property()->get<rdfs::range>()->id==name){
					base_resource* r=current_property->get_Property()->get<rdfs::range>()->f();
					cerr<<"new resource:"<<r<<endl;
					r->id="??";
					current_property->add_property()->set_object(r);
					st.push(r);
				}else{//could be a sub-class
					map<string,rdfs::Class*>::iterator i=m.find(name);
					if(i!=m.end()){
						if(*current_property->get_Property()->get<rdfs::range>().get() < *i->second){
							base_resource* r=i->second->f();
							cerr<<"new resource:"<<r<<endl;
							r->id="??";
							current_property->add_property()->set_object(r);
							st.push(r);
						}else{
							cerr<<name<<" not a sub-class of "<<current_property->get_Property()->get<rdfs::range>()->id<<endl;
							st.push(base_resource::nil);
						}
					}else{
						cerr<<"Class "<<name<<" not found\n";
						st.push(base_resource::nil);
					}
					
				}
			}else if(depth>1){
				st.push(base_resource::nil);
			}
			return true;
		}
		bool end_resource(string name){
			cerr<<"end resource "<<name<<endl;
			st.pop();
			return true;
		}
		/*
			buggy if empty property
			<p/>
			<p></p>

		*/
		bool start_property(string name){
			cerr<<"start property "<<name<<"\t"<<is.peek()<<endl;
			current_property=std::find_if(st.top()->begin(),st.top()->end(),namep(name));
			if(current_property!=st.top()->end()){
				cerr<<"property "<<name<<" found"<<endl;
				if(current_property.literalp()){
					current_property->add_property()->in(is);
					is.clear();
				}
			}else{
				cerr<<"property "<<name<<" not found"<<endl;
			}
			return true;
		}
		bool end_property(string name){
			cerr<<"end property "<<name<<endl;
			return true;
		}
		bool start_element(string name,ATTRIBUTES att){
			++depth;
			//cout<<"start element "<<name<<endl;
			return (depth&1) ? start_resource(name) : start_property(name); 
		}
		bool end_element(string name){
			--depth;
			//cout<<"end element "<<name<<endl;
			return (depth&1) ? end_property(name) : end_resource(name); 
		}
		bool characters(string s){
			cerr<<s.size()<<" character(s) "<<s<<endl;
			return true;
		}
	};
}
#endif
