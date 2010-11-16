/*
 *	experiment with XML signature-like feature
 *		once a special attribute is detected the parser re-serializes the input onto 
 *		a buffer until the end of the element, no event is generated , it then performs checksum on it and
 *		if successful reparse the buffer this time with events.
 *		since the attributes are stored in a map, order does no matter 
 *		comments should not affect signature
 *		it is sensitive to white space 
 *		one must not nest signed elements
 *		in the RDF model an individual property can be signed by making it a child element
 *	the same code can be used to sign and check signature, there should be a way to make sure signature is present in the document
 *
 */
#include "xml_parser.h"
#include <sstream>
#include "Md5.h"
template<typename SAX_HANDLER> struct xml_signature:xml_parser<xml_signature<SAX_HANDLER> >{
	typedef xml_parser<xml_signature<SAX_HANDLER> > BASE;
	bool checking;
	std::stringstream ss;
	int depth;
	string signature;
	xml_signature(istream& is):BASE(is),checking(false),depth(0){}
	bool start_element(string name,ATTRIBUTES att){
		if(checking){
			ss<<"<"<<name;
			for(ATTRIBUTES::iterator i=att.begin();i!=att.end();++i)
				ss<<" "<<i->first<<"='"<<i->second<<"'";
			ss<<">";
			++depth;
			return true;
		}else{
			ATTRIBUTES::iterator i=att.find("signature");	
			if(i!=att.end()){
				signature=i->second;
				att.erase(i);
				checking=true;
				depth=0;
				start_element(name,att);
			}else{
				return static_cast<SAX_HANDLER*>(this)->start_element(name,att);
			}
		}	
	}
	bool end_element(string name){
		if(checking){
			ss<<"</"<<name<<">";
			--depth;
			if(depth==0){
				cerr<<"checking: "<<ss.str()<<endl;
				MD5_CTX mdContext;
				MD5Init(&mdContext);
				MD5Update(&mdContext,(unsigned char*) ss.str().c_str(),ss.str().size());
				MD5Final(&mdContext);
				//cout.width(2);
				ostringstream os;
				for (int i=0;i<16;i++){
					printf ("%02x", mdContext.digest[i]);
					os.width(2);
					os.fill('0');
					os<<hex<<(int)mdContext.digest[i];
				}
				if(os.str()==signature)
					cerr<<"\tmatched signature"<<endl;
				else
					cerr<<"\tmis-matched signature"<<endl;
				//MDPrint(&mdContext);
				//calculate checksum
				checking=false;
				basic_streambuf<char>* b=BASE::is.rdbuf(ss.rdbuf());
				BASE::go();
				BASE::is.rdbuf(b);
			}
			return true;
		}else{
			return static_cast<SAX_HANDLER*>(this)->end_element(name);
		}	
	}
	bool characters(string s){
		if(checking){
			ss<<s;
			return true;
		}else{
			return static_cast<SAX_HANDLER*>(this)->characters(s);
		}	
	}

};
