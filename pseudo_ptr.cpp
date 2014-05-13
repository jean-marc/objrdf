#include "pseudo_ptr.h"
uint32_t jenkins_one_at_a_time_hash(const char *key, size_t len)
{
    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

param::param(void* v,size_t n,const size_t cell_size,const size_t max_size,const bool writable):v(v),n(n),cell_size(cell_size),max_size(max_size),writable(writable){}
void param::resize(size_t n){
	cerr<<"resize:"<<n<<" max_size:"<<max_size<<endl;
	if(n<=max_size)
		resize_impl(n);
	else
		throw std::runtime_error("maximum pool size reached");
}
pool::info::info():next(0),n_cells(0){}
pool::match_type_id::match_type_id(const size_t type_id):type_id(type_id){}
pool::pool(param& p,DESTRUCTOR destructor,size_t type_id):p(p),destructor(destructor),type_id(type_id){
	info& first=*static_cast<info*>(p.v);
	info& second=*static_cast<info*>(p.v+p.cell_size);
	if(first.next==0){
		first.next=1;
		first.n_cells=0;
	}
	cerr<<"new pool "<<this<<" "<<(size_t)first.n_cells<<" out of "<<p.n<<" cells used type_id:"<<type_id<<" max_size:"<<p.max_size<<" hash:"<<hash()<<endl;
}
uint32_t pool::hash() const{return jenkins_one_at_a_time_hash((char*)p.v,p.n);}
pool::~pool(){
	cerr<<"~hash: "<<hash()<<" "<<this<<endl;
}
/*
 *	used by RESOURCE_PTR objrdf::create_by_type(CLASS_PTR c,uri id)
 */
size_t pool::allocate(){
	info* first=static_cast<info*>(p.v);
	if(first->n_cells==p.n-1){
		cerr<<"increasing pool size from "<<p.n<<" to "<<2*p.n<<" cells "<<this<<endl;
		size_t original_size=p.n;
		cerr<<"before: "<<p.v<<endl;
		p.resize(2*p.n);
		cerr<<"after: "<<p.v<<endl;
		cerr<<"new pool size: "<<p.n<<endl;
		first=static_cast<info*>(p.v);
	}	
	info* current=static_cast<info*>(p.v+first->next*p.cell_size);
	size_t i=first->next;
	first->next=(current->next==0) ? first->next+1 : current->next;
	first->n_cells++;
	cerr<<"allocate cell at index "<<i<<"{"<<(p.v+i*p.cell_size)<<"} in pool "<<this<<endl;
	return i;	
}
size_t pool::allocate_at(size_t i){
	info& first=*static_cast<info*>(p.v);
	cerr<<"allocate cell at set index "<<i<<"{"<<p.v+i*p.cell_size<<"} in pool "<<this<<endl;
	assert(i<p.n);
	//that is not correct: keeps increasing counter even if previously occupied
	//should only allowed on non persistent store
	first.n_cells++;
	return i;	
}
void pool::deallocate(size_t i){
	//assume only 1 cell allocated
	cerr<<"deallocate cell at index "<<i<<" in pool "<<this<<endl;
	info& first=*static_cast<info*>(p.v);
	info& current=*static_cast<info*>(p.v+i*p.cell_size);
	//keep linked list of empty cells ordered for iterator
	//it does not work, not really in order, will cause problems in iterator!
	//need to clear the memory 
	if(i<first.next){
		current.next=first.next;
		first.next=i;
	}else{
		info& second=*static_cast<info*>(p.v+first.next*p.cell_size);
		current.next=second.next;
		second.next=i;
	}
	first.n_cells--;
}

void* pool::get(size_t i){
	return p.v+i*p.cell_size;
}
//could use function pointer:allocate, deallocate,...
size_t pool::get_size()const{return static_cast<info*>(p.v)->n_cells;}
bool pool::TEST(const pool& p){
	//tricky because it might be all zeroes, we can not use p.p because it is a reference
	//problem classes without instances (literal) won't show up here
	return p.type_id;//good because the type_id can not be 0
}
pool_array::pool_array(param& p,DESTRUCTOR destructor,size_t type_id):p(p),destructor(destructor),type_id(type_id){
	/*
 	*	we might have to modify p.cell_size
 	*/ 
	auto payload_size=p.cell_size;
	cerr<<"payload size:"<<payload_size<<endl;
	p.cell_size=max(payload_size,sizeof(pool_array::info));
	cerr<<"cell size:"<<p.cell_size<<endl;
	info& first=*static_cast<info*>(p.v);
	info& second=*static_cast<info*>(p.v+p.cell_size);
	cerr<<"initial pool size:"<<first.n_cells<<endl;
	if(first.n_cells==0){
		cerr<<"setting up pool..."<<endl;
		first.next=1;
		second.next=0;
		//second.cell_size=p.n-1;//this is wrong when the payload is smaller than pool_array::info
		//second.range_size=((payload_size<sizeof(pool_array::info)) ? p.n/sizeof(pool_array::info) : p.n)-1;//this is wrong when the payload is smaller than pool_array::info
		second.range_size=p.n-1;//this is wrong when the payload is smaller than pool_array::info
	}
	cerr<<"new pool_array "<<this<<" "<<(size_t)first.n_cells<<" out of "<<p.n<<" cells used type_id:"<<type_id<<" max_size:"<<p.max_size<<" hash:"<<hash()<<endl;
}
bool pool_array::TEST(const pool_array& p){
	//tricky because it might be all zeroes, we can not use p.p because it is a reference
	//problem classes without instances (literal) won't show up here
	return p.type_id;//good because the type_id can not be 0
}
uint32_t pool_array::hash() const{return jenkins_one_at_a_time_hash((char*)p.v,p.n);}
//never called
pool_array::~pool_array(){
	cerr<<"~hash: "<<hash()<<" "<<this<<endl;
}
void* pool_array::get(size_t i){
	return p.v+i*p.cell_size;
}
//could use function pointer:allocate, deallocate,...
size_t pool_array::get_size()const{return static_cast<info*>(p.v)->n_cells;}

empty_store::empty_store():param(new pool::info(),1,0,1){}
void empty_store::resize_impl(size_t){assert(0);}


free_store::free_store(void* v,size_t n,const size_t cell_size,const size_t max_size):param(v,n,cell_size,max_size){}

//_n new number of cells
void free_store::resize_impl(size_t _n){
	//let's just grow for now	
	if(_n>n){
		char* _v=new char[_n*cell_size];
		memcpy(_v,v,n*cell_size);
		memset(_v+n*cell_size,0,(_n-n)*cell_size);
		delete[] v;
		v=_v;
		n=_n;
	}
}

persistent_store::persistent_store(void* v,size_t n,const size_t cell_size,const size_t max_size,const bool writable,int fd,size_t file_size):param(v,n,cell_size,max_size,writable),fd(fd),file_size(file_size){}
persistent_store::~persistent_store(){
	if(munmap(v,file_size)){
		cerr<<"munmap failed"<<endl;
		exit(EXIT_FAILURE);
	}
}
void persistent_store::resize_impl(size_t _n){
	cerr<<"resizing persistent store "<<_n<<endl;
	if(_n>n){
		size_t n_page=max<size_t>(ceil((double)(cell_size*_n)/PAGE_SIZE),1);
		size_t _file_size=n_page*PAGE_SIZE;
		if(_file_size>file_size){
			//grow the file
			/*if(munmap(v,file_size)){
				cerr<<"munmap failed"<<endl;
				exit(EXIT_FAILURE);
			}
			*/
			//system call
			//system("cp db/sat_C db/sat_C.old");		
			int result = lseek(fd,_file_size-1, SEEK_SET);
			if (result == -1) {
				close(fd);
				cerr<<"Error calling lseek() to 'stretch' the file"<<endl;
				exit(EXIT_FAILURE);
			}
			result = write(fd, "", 1);
			if (result != 1) {
				close(fd);
				cerr<<"Error writing last byte of the file"<<endl;
				exit(EXIT_FAILURE);
			}
			/*
			result = msync(v,file_size,MS_SYNC);
			if (result != 0){
				cerr<<"Error synchronizing"<<endl;
				exit(EXIT_FAILURE);
			}
			*/
			/*
			v = (char*)mmap((void*)NULL,_file_size,PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if (v == MAP_FAILED) {
				close(fd);
				cerr<<"Error mmapping the file"<<endl;
				exit(EXIT_FAILURE);
			}
			cerr<<"new mapping at "<<v<<" size:"<<_file_size<<endl;
			*/
			void* _v=(char*)mremap(v,file_size,_file_size,MAP_SHARED,MREMAP_MAYMOVE);
			if (_v == MAP_FAILED) {
				close(fd);
				cerr<<"Error mremapping the file"<<endl;
				exit(EXIT_FAILURE);
			}
			v=_v;
			cerr<<"new mapping at "<<v<<" size:"<<_file_size<<endl;
			file_size=_file_size;
			n=file_size/cell_size;
		}
	}
}
