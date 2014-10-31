#ifndef RDF_XML_PARSER_H
#define RDF_XML_PARSER_H

#include "objrdf.h"
#include "xml_parser.h"
#include <stack>
#include <sstream>
#include <algorithm>
/*
	the RDF/XML syntax is fairly complicated and provides many different ways to describe the same graph
	rdf:ID
	rdf:about
	rdfs:type
	rdf:resource
	rdf:Description
	literal properties can be expressed as attributes or child element
		<rdf:Description age='32'/>
		<rdf:Description>
			<age>32</age>
		</rdf:Description>



*/
//#define LOG if(0) cerr
#define LOG cerr
#define ERROR_PARSER cerr
namespace objrdf{
	struct rdf_xml_parser:xml_parser<rdf_xml_parser>{
		PROVENANCE p;
		bool string_property;
		std::stack<RESOURCE_PTR> st;
		typedef multimap<uri,base_resource::instance_iterator> MISSING_OBJECT;
		MISSING_OBJECT missing_object;//store all the properties waiting for object
		typedef map<string,RESOURCE_PTR> BLANK_NODE;
		BLANK_NODE blank_node;
		/*
 		*	placeholder when a resource can not be created for any reason (eg: the rdfs:type
 		*	is not known yet) so that the parsing can go on.
 		*	could it be allocated on the stack?, no because we need a pointer to it
 		*/ 
		RESOURCE_PTR placeholder;
		base_resource::type_iterator current_property;
		void set_missing_object(RESOURCE_PTR); 
		rdf_xml_parser(std::istream& is,PROVENANCE p=1);
		bool start_resource(uri name,ATTRIBUTES att);
		bool end_resource(uri name);
		bool start_property(uri name,ATTRIBUTES att);
		bool end_property(uri name);
		bool start_element(uri name,ATTRIBUTES att);
		bool end_element(uri name);
		bool characters(string s);
		bool go();
	};
}
#endif
