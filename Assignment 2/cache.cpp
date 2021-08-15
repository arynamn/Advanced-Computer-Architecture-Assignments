#include<bits/stdc++.h>
using namespace std;

typedef struct 
{
	char                type;
    int                 threadid;
	unsigned long long  addr;
}trace_entry;
typedef struct 
{
	unsigned long long int tag;
	unsigned long long int setindex;
}address;
typedef struct 
{
	int setsize  ;
	int sets ;
	int setindex_bits ;
	int tag_bits ;
}Cache_Params;

class LRU
{
    list <unsigned long long> tags;
    public:
    LRU()
    { }
    void updateLRU( unsigned long long tag)
    {
        tags.remove(tag);
        tags.push_front(tag);
    }
    unsigned long long evictLRU()
    {
        unsigned long long tag = tags.back();
        tags.pop_back();
        return tag;
    }
	void remove_tag(unsigned long long int tag)
	{
		tags.remove(tag);
	}
};

class cache_sim
{
    FILE *fout;
    Cache_Params CP;
    unsigned long long **Cache;
	LRU *Cache_LRU;

    int Cache_hit;
	int Cache_miss;
	
	address process_address(unsigned long long addr)
	{
		address a;
		a.setindex		= addr % CP.sets;
		a.tag			= addr / CP.sets;
		return a; 
	}
	int check_Cache(unsigned long long tag, unsigned long long setindex)
	{
		for(int j = 0 ; j < CP.setsize ; j++)
			if( tag == Cache[setindex][j])
				return j;
		return -1;
	}
	void Cache_update(address addr, int position)
	{
		Cache[addr.setindex][position] = addr.tag;
		Cache_LRU[addr.setindex].updateLRU(addr.tag);
	}

public:
	cache_sim(Cache_Params P,char *out_location)   
	{
        CP.setindex_bits = P.setindex_bits;
		CP.sets = P.sets;
        CP.setsize = P.setsize;
        CP.tag_bits = P.tag_bits;
    
		Cache = new unsigned long long *[CP.sets];
		Cache_LRU	= new LRU[CP.sets];
		for(unsigned long long int i = 0; i < CP.sets; i++)
		{
			Cache[i] = new unsigned long long[CP.setsize];
			for(unsigned long long j = 0; j < CP.setsize; j++)
				Cache[i][j] = -1;
		}
		Cache_hit = 0;
		Cache_miss = 0;
        
        string s = out_location;
        s.append("_missed.out");
        fout = fopen(s.c_str(),"w");
	}
	void process_entry(trace_entry trace)
	{
		address addr_Cache = process_address(trace.addr >> 6);		
		int block_Status = check_Cache(addr_Cache.tag, addr_Cache.setindex);
		if(block_Status == -1)
		{	
			Cache_miss++;

            fprintf(fout,"%c  %d  %llu\n", trace.type, trace.threadid, trace.addr);

			int empty_location = check_Cache( -1, addr_Cache.setindex);											
			if(empty_location == -1)	
			{   
				unsigned long long etag = Cache_LRU[addr_Cache.setindex].evictLRU();				
				empty_location = check_Cache(etag, addr_Cache.setindex);					
			}
			Cache_update(addr_Cache, empty_location);												
		}
		else
		{
			Cache_hit++;
			Cache_LRU[addr_Cache.setindex].updateLRU(addr_Cache.tag);
		}
	}

    void print_result()
    {
        cout<<endl;
        cout<<"Cache Hit    "<<Cache_hit<<endl;
        cout<<"Cache Miss   "<<Cache_miss<<endl;
    }
};

int main(int argc , char **argv)
{	
    FILE *fp;
	int count = 0;
	trace_entry trace;	
    
	fp = fopen(argv[1], "rb");
	assert(fp != NULL);

    Cache_Params CPar;
    CPar.tag_bits = 5;
    CPar.setindex_bits = 11;
    CPar.setsize = 16;
    CPar.sets = 2048;

    
    cache_sim mycache( CPar , argv[1]);

	while (!feof(fp)) 
	{
	    fscanf(fp,"%c %d %llu\n", &trace.type, &trace.threadid, &trace.addr);
        
        mycache.process_entry(trace);        
        count++;
		cout<<count<<"\r";
	}
    mycache.print_result();
    return 0;
}
