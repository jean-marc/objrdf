#ifndef GEO_H
#define GEO_H
#include "objrdf.h"
namespace geo{
	//in degrees
	RDFS_NAMESPACE("http://www.w3.org/2003/01/geo/wgs84_pos#","geo")
	typedef objrdf::property<rdfs_namespace,objrdf::str<'l','a','t'>,float> lat;
	//problem:conflict with type long
	typedef objrdf::property<rdfs_namespace,objrdf::str<'l','o','n','g'>,float> _long_;
	typedef objrdf::property<rdfs_namespace,objrdf::str<'a','l','t'>,float> alt;
	CLASS(Point,std::tuple<lat,_long_,alt>);
	//we could define functions that operate on geo:Point eg. distance (see http://www.w3.org/TR/rdf-sparql-query/#extensionFunctions)
}
#endif
