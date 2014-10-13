#ifndef GEO_H
#define GEO_H
#include "objrdf.h"
namespace geo{
	//in degrees
	RDFS_NAMESPACE("http://www.w3.org/2003/01/geo/wgs84_pos#","geo")
	PROPERTY(lat,float);
	//problem:conflict with type long
	//PROPERTY(long,float);
	struct _long{static char* name(){return "long";}};
	typedef objrdf::property<rdfs_namespace,_long,float> _long_;
	CLASS(Point,std::tuple<lat,_long_>);
	//we could define functions that operate on geo:Point eg. distance (see http://www.w3.org/TR/rdf-sparql-query/#extensionFunctions)
}
#endif
