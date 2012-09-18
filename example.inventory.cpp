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
 *	need to have generic schema defined in a library
 */
#include "objrdf.h"
#include "sparql_engine.h"
#include "httpd.h"
#include "geo.h"
#include "rdf_xml_parser.h"
using namespace objrdf;

RDFS_NAMESPACE("http://www.example.org/inventory#","inv")

typedef persistent_store STORE;

CLASS(Organization,std::tuple<>);
PROPERTY(organization,pseudo_ptr<Organization,STORE>);
//CLASS(Site,std::tuple<organization>);
DERIVED_CLASS(Site,geo::Point,std::tuple<organization>);
PROPERTY(located,pseudo_ptr<Site,STORE>);
//maybe should use friend of a friend ontology for people
//people
PROPERTY(phone,long);
//typedef located site;//alias does not work!
PROPERTY(site,pseudo_ptr<Site,STORE>);
//not quite ready yet
//CLASS(Person,std::tuple<array<phone,STORE>,site>);// template array<> is broken with literal properties
CLASS(Person,std::tuple<phone,site>);
PROPERTY(status,int);//needs to be developed: working/not working,...history
PROPERTY(_mac_,long);//MAC address 48 bytes
struct mac:_mac_{
	void in(istream& is){is>>hex>>t>>dec;}
	void out(ostream& os) const{os<<hex<<t<<dec;}
};
/*
 *	a server can send an update based on its own mac address and the one returned by arp
 *	
 *	insert data {?x :status 1 .} where {?x :mac 00:11:22:33:44:55 .}
 */
//would be nice to know where a piece of equipment has been, also to know if it has been serviced
//history is a linked-list (how does git work?)
CLASS(Manufacturer,std::tuple<>);
PROPERTY(manufacturer,pseudo_ptr<Manufacturer,STORE,false>);
CLASS(Model,std::tuple<manufacturer>);
PROPERTY(model,pseudo_ptr<Model,STORE,false>);
CLASS(Set,std::tuple<located/*,array<part>*/>);
PROPERTY(partOf,pseudo_ptr<Set,STORE,true>);
/*
 *	we could have manufacturer & model but it could become inconsistent, so only model is used
 */
CLASS(Equipment,std::tuple<partOf,model,status>);
//PROPERTY(part,pseudo_ptr<Equipment,STORE>);
//a digital doorway or a drum could be considered a Set
//could also see a set as a Bag of equipment, using RDF semantic
//would be nice to have URI for those
DERIVED_CLASS(DDoorway,Set,std::tuple<>);
DERIVED_CLASS(DDrum,Set,std::tuple<>);
DERIVED_CLASS(Uniport,Set,std::tuple<>);
DERIVED_CLASS(Kiosk,Set,std::tuple<>);

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
CLASS(Version,std::tuple<>);
//PROPERTY(version,pseudo_ptr<base_resource>);//most general
PROPERTY(version,pseudo_ptr<Version>);//most general
DERIVED_CLASS(Drive,Equipment,std::tuple<version>);//information about OS/software version
//could define versions with some explanations and link to them
//eg ubuntu-desktop-11.10-amd64
DERIVED_CLASS(Laptop,Equipment,std::tuple<mac>);
DERIVED_CLASS(Server,Equipment,std::tuple<mac>);
DERIVED_CLASS(UPS,Equipment,std::tuple<>);
PROPERTY(sim,char[20]);
DERIVED_CLASS(Modem,Equipment,std::tuple<sim>);
PROPERTY(power,int);
PROPERTY(voltage,int);
DERIVED_CLASS(PV_panel,Equipment,std::tuple<power,voltage>);
//need to keep track of batteries, also used to give unique ID because they don't come with s/n
DERIVED_CLASS(Battery,Equipment,std::tuple<>);
DERIVED_CLASS(Charge_Controller,Equipment,std::tuple<>);
DERIVED_CLASS(Relay_Driver,Equipment,std::tuple<>);
int main(int argc,char* argv[]){
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
	PV_panel::get_class();
	Battery::get_class();
	Charge_Controller::get_class();
	Relay_Driver::get_class();
	Organization::get_class();
	Model::get_class();	
	for(int i=1;i<argc;++i){
		ifstream in(argv[i]);
		rdf_xml_parser r(in);
		cerr<<"parsing file `"<<argv[1]<<"'"<<endl;
		r.go();
	}
	to_rdf_xml(cout);
	//start web server
	objrdf::httpd h;
	h.run();
	/*
	//h.start();
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





