#include "objrdf_time.h"
ostream& objrdf::operator<<(ostream& os,const std::chrono::system_clock::time_point& _t){
	std::time_t t=std::chrono::system_clock::to_time_t(_t);
	char buffer[80];
	tm* timeinfo=localtime(&t);
	strftime (buffer,80,"%FT%T",timeinfo);	
	char tz_buffer[20];
	strftime (tz_buffer,20,"%z",timeinfo);	
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(_t.time_since_epoch());
	char ms_buffer[10];	
	snprintf(ms_buffer,10,".%03lli",(ms.count()%1000));
	return os<<buffer<<ms_buffer<<tz_buffer;
}
istream& objrdf::operator>>(istream& is,const std::chrono::system_clock::time_point& t){return is;}//no op
