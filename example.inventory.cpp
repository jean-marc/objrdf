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
 *
 *	prefix :<http://inventory.unicefuganda.org/#>
 *	select ?type ?model ?equipment ?set ?site ?manufacturer 
 *	where {	?equipment a :Equipment;:partOf ?set;a ?type;:model ?model .
 *		?set :located ?site . 
 *		?model :manufacturer ?manufacturer .
 *	} order by ?type
 */
#include "objrdf.h"
#include "sparql_engine.h"
#include "httpd.h"
#include "geo.h"
#include "rdf_xml_parser.h"
using namespace objrdf;

RDFS_NAMESPACE("http://inventory.unicefuganda.org/#","inv")

typedef persistent_store STORE;

CLASS(Organization,std::tuple<>);
PROPERTY(organization,pseudo_ptr<Organization>);
DERIVED_CLASS(Site,geo::Point,std::tuple<organization>);
PROPERTY(located,pseudo_ptr<Site>);
//maybe should use friend of a friend ontology for people
//people
PROPERTY(phone,long);
PROPERTY(site,pseudo_ptr<Site>);
CLASS(Person,std::tuple<array<phone,STORE>,array<site>>);//same person can be responsible for more than one site
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
PROPERTY(manufacturer,pseudo_ptr<Manufacturer>);
CLASS(Model,std::tuple<manufacturer>);
PROPERTY(model,pseudo_ptr<Model>);
/*
 *	add properties and functions to track usage: how many hours it has been up, the VPN server
 *	will run a query when a client connects or disconnects, it will trigger a counter on the object.
 *	shall we store all the information in the db or use files? let's use the db, it will help create nice plots
 *
 */
/*
 *	logger: to keep track of the important data:
 *		on,
 *		uptime, will have to be adjusted if it is more than time since last entry
 *		number of clients on
 *		voltage
 *	should use blank nodes so the monitoring module does not have to come up with new id
 *	prefix :<http://inventory.unicefuganda.org/#> insert data {
 *		<test> :logger [
 *			:voltage 14.1;
 *			:n_client 3;
 *			:uptime 10
 *			] .}
 * 	
 * 	there are 2 ways to store the information:
 * 	1: Set -->-- Logger
 * 	<Set id='a'>
 * 	 <logger><Logger/></logger>
 * 	 <logger><Logger/></logger>
 * 	 ...
 * 	</Set>
 * 	instances have a list of predicate logger stored in a vector, the vector plays 2 roles: indicate which Logger belongs
 * 	to the Set and the sequence order, this can be accomplished in the second method by using indexes on `time' and `prop'
 * 	the problem with this method is that the object grows over time and becomes cumbersome to serialize
 * 	For instance if we want to track version it get expensive to stow away the object
 * 	2: Logger -->-- Set
 * 	<Set id='a'/>
 * 	<Logger time='0'><prop resource='#a'/></Logger>
 * 	<Logger time='1'><prop resource='#a'/></Logger>
 * 	...
 * 	we could use a special pool with no de-allocate so order matches time of instantiation
 *
 */
PROPERTY(time_stamp,time_t);
PROPERTY(time_stamp_v,objrdf::NIL);
PROPERTY(uptime,time_t);//how long has the kiosk been up
PROPERTY(uptime_v,objrdf::NIL);
PROPERTY(n_client,uint16_t);
PROPERTY(volt,float);
//CLASS(Logger,std::tuple<time_stamp,uptime,n_client,volt>);
char _Logger[]="Logger";
class Logger:public resource<rdfs_namespace,_Logger,std::tuple<time_stamp,uptime,n_client,volt>,Logger>{
public:
	typedef std::tuple<time_stamp_v,uptime_v> PSEUDO_PROPERTIES;
	Logger(uri id):SELF(id){
		get<time_stamp>().t=time(0);	
	}
	void out_p(time_stamp_v,ostream& os) const{
		os<<ctime(&get_const<time_stamp>().t);
	}
	void out_p(uptime_v,ostream& os) const{
		time_t t=get_const<uptime>().t;
		int m=(t/60)%60;	
		int h=(t/3600)%24;	
		int d=t/(3600*24);	
		os<<d<<"d "<<h<<"h "<<m<<"m";
	}
};

//CLASS(Set,std::tuple<located>);
char _Set[]="Set";
PROPERTY(on,bool);
PROPERTY(tot_uptime,time_t);//how long has the kiosk been up
PROPERTY(logger,pseudo_ptr<Logger>);
PROPERTY(plot,objrdf::NIL);
/*
 *	how do we track changes: eg new location
 *
 */
class Set:public resource<rdfs_namespace,_Set,std::tuple<located,on,/*uptime*/array<logger>/*,objrdf::prev*/>,Set/*very important!*/>{
	time_t start;
public:
	typedef std::tuple<plot> PSEUDO_PROPERTIES;
	//typedef on TRIGGER;
	//typedef located TRIGGER;
	//typedef located VERSION;
	Set(uri id):SELF(id),start(time(0)){cerr<<"new Set()"<<endl;}
	static bool comp(const logger& a,time_t b){return a->get_const<time_stamp>().t<b;}
	void out_p(plot,ostream& os) const{
	/*
 	*	display of uptime, power, bandwidth,....
 	*	get the time at midnight or display the last 24 hours, or the last 24 hours recorded
 	*	it might make sense to cache it, but the function is declared const
 	*/ 
		if(get_const<array<logger>>().size()==0) return;
		int width=200,height=100;	
		//should be made a parameter
		time_t duration=10*24*3600;//1 day
		float scale_x=static_cast<float>(width)/duration;
		//need to distinguish 12V/24V
		//we could query db or guess from voltage
		float min_v=0,max_v=0,dscn_v=0;
		if(get_const<array<logger>>().back()->get_const<volt>().t>20){
			min_v=20;
			max_v=30;
			dscn_v=24.5;
		}else{
			min_v=10;
			max_v=20;
			dscn_v=12.25;
		}
		float range_v=max_v-min_v;
		float scale_y=static_cast<float>(height)/range_v;
		//careful : could be empty!
		//time_t stop=get_const<array<logger>>().back()->get_const<time_stamp>().t;	
		time_t stop=time(0);//now
		time_t start=stop-duration; 
		//find the index in get<array>
		auto j=lower_bound(get_const<array<logger>>().cbegin(),get_const<array<logger>>().cend(),start,Set::comp);
		os<<"<svg width='"<<width<<"' height='"<<height<<"' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' style='fill:none;stroke:black;stroke-width:1;'>";
		//how to draw a line at midnight?
		time_t rawtime;
		time ( &rawtime );
		struct tm * timeinfo;
		timeinfo = localtime ( &rawtime );
		timeinfo->tm_sec=0;
		timeinfo->tm_min=0;
		timeinfo->tm_hour=0;
		time_t mn=mktime(timeinfo);
		{
			auto i=j;
			os<<"<g transform='scale(1,-1) translate(0,"<<-height<<")'>"<<endl;;
			for(;mn>start;mn-=24*3600){
				int wday=localtime(&mn)->tm_wday;
				if(wday==0||wday==6) 
					os<<"<rect class='we' x='"<<((mn-start)*scale_x)<<"' y='0' width='"<<(24*3600*scale_x)<<"' height='"<<height<<"'/>"<<endl;
				os<<"<path class='vgrid' d='M"<<((mn-start)*scale_x)<<" 0v"<<height<<"'/>"<<endl;
			}
			for(float v=min_v+1.0;v<max_v;v+=1.0)
				os<<"<path class='hgrid' d='M0 "<<((v-min_v)*scale_y)<<"h"<<width<<"'/>"<<endl;
			os<<"<path class='dscn' d='M0 "<<((dscn_v-min_v)*scale_y)<<"h"<<width<<"'/>"<<endl;
			int x=((*i)->get_const<time_stamp>().t-start)*scale_x;
			int y=((*i)->get_const<volt>().t-min_v)*scale_y;
			os<<"<path class='trace' d='M"<<x<<" "<<y<<"L";
			++i;
			for(;i<get_const<array<logger>>().cend();++i){
				int x=((*i)->get_const<time_stamp>().t-start)*scale_x;
				int y=((*i)->get_const<volt>().t-min_v)*scale_y;
				os<<x<<" "<<y<<" ";
			}	
			os<<"'/></g>"<<endl;
		}
		//is the machine on? how many clients connected?, let's draw rectangles
		{
			os<<"<g transform='scale(1,-1) translate(0,"<<-height<<")'>"<<endl;;
			time_t prev=start;
			for(auto i=j;i<get_const<array<logger>>().cend();++i){
				time_t current=(*i)->get_const<time_stamp>().t;
				float x=(current-start)*scale_x;
				float w=min(current-prev,(*i)->get_const<uptime>().t)*scale_x;
				int h=0.1*height*(1+(*i)->get_const<n_client>().t);
				//room for optimization: 1 big rectangle instead of many (assuming same number of clients)
				os<<"<rect class='uptime' x='"<<x-w<<"' y='0' width='"<<w<<"' height='"<<h<<"'/>"<<endl;		
				prev=current;
			}	
			os<<"</g>";
		}
		os<<"<rect class='frame' x='0' y='0' width='"<<width<<"' height='"<<height<<"'/>";
		os<<"</svg>";
	}
	void set_p(on& p){
		/*if(p.t)
			start=time(0);
		else
			get<uptime>().t+=time(0)-start;
		*/
	//	get<on>()=p;
	}
	void set_p(const located& p){
		//get<located>()=p;
	}
};


PROPERTY(partOf,pseudo_ptr<Set,Set::STORE,true>);
/*
 *	we could have manufacturer & model but it could become inconsistent, so only model is used
 */
CLASS(Equipment,std::tuple<partOf,model,status>);
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
//PROPERTY(version,pseudo_ptr<base_resource>);//most general but problem with persistence
PROPERTY(version,pseudo_ptr<Version>);
DERIVED_CLASS(Drive,Equipment,std::tuple<version>);//information about OS/software version
//could define versions with some explanations and link to them
//eg ubuntu-desktop-11.10-amd64
DERIVED_CLASS(Laptop,Equipment,std::tuple<mac>);//problem: won't show up in schema, we can work-around in stylesheet
DERIVED_CLASS(Server,Equipment,std::tuple<mac>);
DERIVED_CLASS(UPS,Equipment,std::tuple<>);
//PROPERTY(sim,char[20]);
CLASS(Sim,std::tuple<>);
PROPERTY(sim,pseudo_ptr<Sim>);//because we will start moving sim cards around
/*
 *	is the modem attached to a service provider, if not it means it is unlocked
 */
CLASS(Telco,std::tuple<>); //Orange, MTN, Airtel, Warid
PROPERTY(telco,pseudo_ptr<Telco>);
DERIVED_CLASS(Modem,Equipment,std::tuple<sim/*,telco*/>);
PROPERTY(power,int);
PROPERTY(voltage,int);
DERIVED_CLASS(PV_panel,Equipment,std::tuple<power,voltage>);
DERIVED_CLASS(Battery,Equipment,std::tuple<>);
DERIVED_CLASS(Charge_Controller,Equipment,std::tuple<>);
DERIVED_CLASS(Relay_Driver,Equipment,std::tuple<>);
DERIVED_CLASS(Inverter,Equipment,std::tuple<>);
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
	Inverter::get_class();
	Organization::get_class();
	Model::get_class();	
	/*
	//how do I get access to the pool?
	//auto i=POOL_PTR::help<Set>();
	//pseudo_ptr<Set> p(i->allocate());
	//p.construct(Set(uri("jm")));	
	auto p=pseudo_ptr<Set>::construct(uri("jm"));
	cerr<<"start test..."<<endl;
	cerr<<sizeof(Logger)<<" bytes"<<endl;
	for(;;){
		//auto i=POOL_PTR::help<Logger>();
		pseudo_ptr<Logger> l(pseudo_ptr<Logger>::get_pool()->allocate());//broken!!!!
		//auto l=pseudo_ptr<Logger>::allocate();
		ostringstream os;
		l._print(os);
		uri u(os.str());
		u.index=1;
		new(l) Logger(u);
		//l.construct(u);	
		p->get<array<logger>>().push_back(logger(l));
	}
	//RESOURCE_PTR p=create_by_type(Set::get_class(),uri("test"));
	*/
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
