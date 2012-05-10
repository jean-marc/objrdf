/*
 *	schema to keep track of all items
 *	should be also defined in SQL for comparison
 *	there should be forms to help entering data: sparql+js or sparql+xslt (maybe better)
 *	we could also creates special handler in the server for each request:
 *		.new piece of inventory
 *		.update piece of inventory
 *		....
 *	it could also be combined with monitoring system but then needs to be made secure because will
 *	be on-line, we could tunnel through ssh
 *	need to be able to dump to RDF anytime for back-up and schema upgrade
 */
#include "objrdf.h"
#include "sparql_engine.h"
#include "httpd.h"
#include "rdf_xml_parser.h"
//#include "rdf_xml_parser.h"
using namespace objrdf;

RDFS_NAMESPACE("http://www.example.org/inventory#","inv")

typedef persistent_store STORE;
CLASS(Site,std::tuple<>);
PROPERTY(located,pseudo_ptr<Site,STORE>);
CLASS(Organization,std::tuple<>);

//maybe should use friend of a friend ontology for people
//people
PROPERTY(phone,long);
//typedef located site;//alias does not work!
PROPERTY(site,pseudo_ptr<Site,STORE>);
CLASS(Person,std::tuple<phone,site>);
PROPERTY(status,int);//needs to be developed: working/not working,...history
PROPERTY(mac,long);//MAC address 48 bytes
/*
 *	a server can send an update based on its own mac address and the one returned by arp
 *	
 *	insert data {?x :status 1 .} where {?x :mac 00:11:22:33:44:55 .}
 *	insert data {?x :status 1 .} where {?x :mac 00:11:22:33:44:55 .}
 *	insert data {?x :status 1 .} where {?x :mac 00:11:22:33:44:55 .}
 *	insert data {?x :status 1 .} where {?x :mac 00:11:22:33:44:55 .}
 */
//would be nice to know where a piece of equipment has been, also to know if it has been serviced
//history is a linked-list (how does git work?)
//what if it's in storage, we define a special set?
CLASS(Set,std::tuple<located/*,array<part>*/>);
PROPERTY(partOf,pseudo_ptr<Set,STORE,true>);
CLASS(Equipment,std::tuple<partOf,status>);
//PROPERTY(part,pseudo_ptr<Equipment,STORE>);
//a digital doorway or a drum could be considered a Set
//could also see a set as a Bag of equipment, using RDF semantic
//would be nice to have URI for those
DERIVED_CLASS(DDoorway,Set,std::tuple<>);
DERIVED_CLASS(DDrum,Set,std::tuple<>);
DERIVED_CLASS(Uniport,Set,std::tuple<>);
DERIVED_CLASS(Kiosk,Set,std::tuple<>);
//mac address property 11:22:33:44:55:66
//48bit so we can use long

//equipment
/*what kind of ID shall we use? 
*
* MAC address? nice because easily accessible 
* 	for instance a server can go through all the clients attached and post a sparql query, without knowing the actual ID
* 	we can get the MAC address from the BIOS menu, which will have to be modified for each new machine anyway
* 	INSERT {?x :status :on .} WHERE {?x :mac '00:11:22:33:44:55' .}
* 	we can also have some kind of `auto-discovery'
* 	INSERT DATA {<00:11:22:33:44:55> :a :Laptop . <SET_ID> :part <00:11:22:33:44:55> .}
* 	needs to be attached to the SET because equipment can be replaced
* 	MAC could be stored as an int in memory for fast look-up
*
* pseudo_ptr? nice because very fast access, we can attach that new ID to the piece of equipment (bar code/QR code,..)
*
* serial number? useful when dealing with manufacturer
*/
DERIVED_CLASS(Drive,Equipment,std::tuple<>);//information about OS/software version
//could define versions with some explanations and link to them
//eg ubuntu-desktop-11.10-amd64
DERIVED_CLASS(Laptop,Equipment,std::tuple<>);
DERIVED_CLASS(Server,Equipment,std::tuple<>);
DERIVED_CLASS(UPS,Equipment,std::tuple<>);
PROPERTY(sim,char[20]);
DERIVED_CLASS(Modem,Equipment,std::tuple<sim>);
//need to keep track of batteries

int main(){
	Site::get_class();
	Person::get_class();
	Set::get_class();
	DDoorway::get_class();
	DDrum::get_class();
	Kiosk::get_class();
	Uniport::get_class();
	Equipment::get_class();
	Drive::get_class();
	Laptop::get_class();
	Server::get_class();
	UPS::get_class();
	Modem::get_class();	
	cerr<<"parsing!"<<endl;
	rdf_xml_parser r(cin);
	r.go();
	/*
	//start web server
	to_rdf_xml(cout);
	objrdf::httpd h;
	h.start();
	
	cin.exceptions(iostream::eofbit);
	sparql_parser sp(std::cin);
	bool r=sp.go();
	if(r){
		cerr<<"parsing success"<<endl;
		sp.out(cout);
	}else{
		cerr<<"parsing failure"<<endl;
		exit(1);
	}
	*/

};





