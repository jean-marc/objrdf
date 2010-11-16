#ifndef RDF_XML_PARSER_H
#define RDF_XML_PARSER_H
#include "xml_parser.h"
#include "objrdf.h"
#include <stack>
#include <sstream>
#include <algorithm>
//using namespace std;
/*
	the RDF/XML syntax is fairly complicated and provides many different ways to describe the same graph
	rdf:id
	rdf:about
	rdf:type
	rdf:resource
	rdf:Description
	literal properties can be expressed as attributes or child element
		<Description age='32'/>
		<Description>
			<age>32</age>
		</Description>



*/
#define LOG if(0) cerr
#define ERROR_PARSER cerr
namespace objrdf{
	struct rdf_xml_parser:xml_parser<rdf_xml_parser>{
		bool string_property;
		std::stack<shared_ptr<base_resource> > st;
		typedef multimap<string,base_resource::instance_iterator> MISSING_OBJECT;
		MISSING_OBJECT missing_object;//store all the properties waiting for object
		base_resource::type_iterator current_property;
		int depth;
		rdf::RDF& doc;
		void set_missing_object(shared_ptr<base_resource>); 
		rdf_xml_parser(rdf::RDF& _doc,std::istream& is);
		bool start_resource(string name,ATTRIBUTES att);
		bool end_resource(string name);
		bool start_property(string name,ATTRIBUTES att);
		bool end_property(string name);
		bool start_element(string name,ATTRIBUTES att);
		bool end_element(string name);
		bool characters(string s);
		bool go();
	};
}
#endif
