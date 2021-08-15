#include<iostream>
#include<cmath>
#include<assert.h>
#include<list> 
#include<iterator> 
#include<algorithm>

using namespace std;

typedef struct 
{
	int blocksize ;
	int setsize  ;
	int size ;
	int sets ;
	int setindex_bits ;
	int boffset_bits ;
	int tag_bits ;
} cache_par; 
typedef struct 
{
	unsigned long long int tags;
	unsigned long long int setindex;
	unsigned long long int blocksoffset;
}split_addr;
typedef struct 
{
	char iord;
	char type;
	unsigned long long addr;
	unsigned pc;
}trace_entry;
cache_par get_cache_par( int bs , int ss , int size )
{
	cache_par	c;
	c.blocksize		    = bs;
	c.setsize			= ss;
	c.size				= size;
	c.sets				= c.size / ( c.setsize * c.blocksize );
	c.setindex_bits 	= log2( c.sets );
	c.boffset_bits  	= log2( c.blocksize );
	c.tag_bits	  		= 64 - ( c.setindex_bits + c.boffset_bits ); 
	return c;
}

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
	cache_par L2_par;
	cache_par L3_par;
	unsigned long long **L2;
	unsigned long long **L3;
	bool **L2_valid;
	bool **L3_valid;
	LRU *L2_lru;
	LRU *L3_lru;

	int L2_hit;
	int L2_miss;
	int L3_hit;
	int L3_miss;

    int coldmiss;
    int capacitymiss;
    int conflictmiss;
	
    list <unsigned long long int> L3_tags;
    

	split_addr process_address(int cache_type, unsigned long long addr) // cache_type 0 for L2, 1 for L3
	{
		split_addr a;
		if(cache_type == 0)
		{
			a.blocksoffset 	= addr % L2_par.blocksize;
			a.setindex		= (addr / L2_par.blocksize) % L2_par.sets;
			a.tags			= addr / (L2_par.blocksize*L2_par.sets);
		}
		else
		{
			a.blocksoffset 	= addr % L3_par.blocksize;
			a.setindex		= (addr / L3_par.blocksize) % L3_par.sets;
			a.tags			= addr / (L3_par.blocksize*L3_par.sets);
		}
		return a; 
	}
	
	int check_l2_cache(unsigned long long int tag , unsigned long long int setindex)
	{
		for(int j = 0 ; j < L2_par.setsize ; j++)
			if( tag == L2[ setindex ][j])
				return j;
		return -1;
	}
	int check_l3_cache(unsigned long long int tag , unsigned long long int setindex)
	{
        for(int j = 0 ; j < L3_par.setsize ; j++)
        {
			if( tag == L3[ setindex ][j])
				return j;
		}
		return -1;
	}
	int check_l2_empty(split_addr addr)
	{
		for(int j = 0 ; j < L2_par.setsize ; j++)
			if( L2[addr.setindex][j] == -1 )
				return j;
		for(int j = 0 ; j < L2_par.setsize ; j++)
			if( L2_valid[addr.setindex][j] == 0)
				return j;

		return -1;
	}
	int check_l3_empty(split_addr addr)
	{
        for(int j = 0 ; j < L3_par.setsize ; j++)
			if( L3[addr.setindex][j] == -1 ) 
				return j;
		for(int j = 0 ; j < L3_par.setsize ; j++)
			if( L3_valid[addr.setindex][j] == 0) 
				return j;
		return -1;
	}
	void L2_invalidate(unsigned long long int tag, unsigned long long int setindex)
	{
		unsigned long long int index = check_l2_cache(tag, setindex);
		L2_valid[setindex][index] = 0;
		L2_lru[setindex].remove_tag(tag);
	}	
	void L3_invalidate(unsigned long long int tag, unsigned long long int setindex)
	{
		unsigned long long int index = check_l3_cache(tag, setindex);
		L3_valid[setindex][index] = 0;
		L3_lru[setindex].remove_tag(tag);
	}
	void L2_cache_update(split_addr addr, int position)
	{
		L2[addr.setindex][position] = addr.tags;
		L2_valid[addr.setindex][position] = 1;
		L2_lru[addr.setindex].updateLRU(addr.tags);
	}
	void L3_cache_update(split_addr addr, int position)
	{
		L3[addr.setindex][position] = addr.tags;
		L3_valid[addr.setindex][position] = 1;
		L3_lru[addr.setindex].updateLRU(addr.tags);
	}
	split_addr addr_translate_l2_l3(unsigned long long int tag, unsigned long long int setindex)
	{
		int val = tag%2;
		split_addr addr;
		addr.tags = tag/2;
		addr.setindex = val*pow(2,10) + setindex;
		addr.blocksoffset = 0;
		return addr;
	}
	split_addr addr_translate_l3_l2(unsigned long long int tag, unsigned long long int setindex)
	{
		unsigned long long int temp_val = pow(2,10);
		int val =  ( setindex / temp_val )  % 2;
		split_addr addr;
		addr.tags = tag*2 + val;
		addr.setindex =  setindex % temp_val ;
		addr.blocksoffset = 0;
		return addr;
	}

    void classify_l3_miss(split_addr addr)
    {
	unsigned long long search_tag = addr.tags * pow(2,11) + addr.setindex;
        bool found = find(L3_tags.begin(), L3_tags.end(), search_tag) != L3_tags.end();
        if(found)
        {
            int flag = 0;
            for(int i = 0; i < L3_par.sets ; i++)
			{
				for(int j = 0; j < L3_par.setsize ; j++)
				{
					if(L3_valid[i][j] == 0)
					{
							flag++;
							break;
					}
				}
				if(flag!= 0)
					break;
			}
			if(flag != 0)
				conflictmiss++;
			else
			{
				capacitymiss++;
			}               
        }
        else
        {
            coldmiss++;
            L3_tags.push_back(search_tag);
        }
               
    }

	void print_cache(int type , unsigned long long setindex)
	{
		cout<<"Start"<<endl;
		if(type == 0)
		{
			for(int i = 0; i < L2_par.setsize;i++)
			{
				cout<<L2[setindex][i]<<"   "<<L2_valid[setindex][i]<<endl;
			}
		}
		else
		{
			for(int i = 0; i < L3_par.setsize;i++)
			{
				cout<<L3[setindex][i]<<"   "<<L3_valid[setindex][i]<<endl;
			}
		}
		cout<<"End"<<endl;
		
	}

public:

	cache_sim( cache_par l2_par ,cache_par l3_par)   //constructor
	{
		int i , j;
		L2_par = l2_par;
		L3_par = l3_par;

		L2 = new unsigned long long *[L2_par.sets];
		L2_valid = new bool *[L2_par.sets];
		L2_lru	= new LRU[L2_par.sets];

		//cout<<L2_par.setsize<<"   "<<L2_par.sets<<endl;
		for(i=0; i<L2_par.sets; i++)
		{
			L2[i] = new unsigned long long[L2_par.setsize];
			L2_valid[i] = new bool[L2_par.setsize];
			for(j=0; j<L2_par.setsize; j++)
			{
				L2[i][j] = -1;
				L2_valid[i][j] = false;
			}
		}

		//cout<<L3_par.setsize<<"   "<<L3_par.sets<<endl;
		L3 = new unsigned long long  *[L3_par.sets];
		L3_valid = new bool *[L3_par.sets];
		L3_lru	= new LRU[L3_par.sets];

		for(i=0; i<L3_par.sets; i++)
		{
			L3[i] = new unsigned long long[L3_par.setsize];
			L3_valid[i] = new bool[L3_par.setsize];
			for(j=0; j<L3_par.setsize; j++)
			{
				L3[i][j] = -1;
				L3_valid[i][j] = false;
			}
		}
		L2_hit = 0;
		L2_miss = 0;
		L3_hit = 0;
		L3_miss = 0;
        coldmiss = 0;
        capacitymiss = 0;
        conflictmiss = 0;
		//cout<<"L2  "<<L2_par.tag_bits<<"  "<<L2_par.setindex_bits<<"  "<<L2_par.boffset_bits<<endl;
		//cout<<"L3  "<<L3_par.tag_bits<<"  "<<L3_par.setindex_bits<<"  "<<L3_par.boffset_bits<<endl;
	}

	void process_entry(trace_entry t)
	{
		split_addr addr_l2 = process_address(0,t.addr);

		// Searching in L2
		int l2_status = check_l2_cache(addr_l2.tags, addr_l2.setindex);
		if(l2_status == -1)
		{	
            
			L2_miss++;

			split_addr addr_l3 = process_address(1,t.addr);		

			// Searching in L3
			int l3_status = check_l3_cache(addr_l3.tags, addr_l3.setindex);
			if( l3_status == -1 )
			{
                classify_l3_miss(addr_l3);

				L3_miss++;

				// Checking for empty space in L3
				int l3_empty_status = check_l3_empty(addr_l3);
				if( l3_empty_status == -1 )
				{
					unsigned long long evicted_l3_tag = L3_lru[addr_l3.setindex].evictLRU(); 	// Get LRU 
					l3_empty_status   = check_l3_cache(evicted_l3_tag, addr_l3.setindex);  		// l3_empty_status = emptied position
					
					split_addr evicted_l2 = addr_translate_l3_l2(evicted_l3_tag, addr_l3.setindex);

					L2_invalidate( evicted_l2.tags , addr_l2.setindex);							// Invalidate in L2
				}
				L3_cache_update(addr_l3, l3_empty_status);										// Update L3 cache with new tag
				l3_status = l3_empty_status;													// L3 cache position to copy to L2
			}	
			else
			{
				L3_hit++;
			}
			
			// Checking for empty space in L2
			int l2_empty_status = check_l2_empty(addr_l2);
			if(l2_empty_status == -1)
			{
				unsigned long long evicted_l2_tag = L2_lru[addr_l2.setindex].evictLRU();		// Evict LRU block
				l2_empty_status = check_l2_cache(evicted_l2_tag, addr_l2.setindex);				// l2_empty_block = emptied location
			}
			L2_cache_update(addr_l2, l2_empty_status);											// Copy Tag to L2_empty_status
		}
		else
		{
			L2_hit++;
			L2_lru[addr_l2.setindex].updateLRU(addr_l2.tags);
		}
		cout<<"Done  :"<<addr_l2.tags<<endl;
	}

	void print_report()
	{
		cout<<"\nReport for misses and hits"<<endl;
		cout<<"L2 cache hit  :"<<L2_hit<<endl;
		cout<<"L2 cache mis  :"<<L2_miss<<endl;
		cout<<"L3 cache hit  :"<<L3_hit<<endl;
		cout<<"L3 cache mis  :"<<L3_miss<<endl;
        cout<<"Cold Miss  :"<<coldmiss<<endl;
        cout<<"Capacity Miss  :"<<capacitymiss<<endl;
        cout<<"Conflict Miss  :"<<conflictmiss<<endl;
	}

	
};

int main(int argc , char **argv)
{	
	int k;
	int numtraces = atoi(argv[2]);
	trace_entry trace;	
	cache_par l2 = get_cache_par(64, 8 , 512*1024 );
	cache_par l3 = get_cache_par(64, 16, 2*1024*1024 );
	cache_sim mycache(l2,l3);
	for (k=0; k<numtraces; k++) 
	{
		char input_name[30];
		FILE *fp;
	    sprintf(input_name, "%s_%d", argv[1], k);
	    fp = fopen(input_name, "rb");
	    assert(fp != NULL);

	    while (!feof(fp)) 
	    {
	      	fread(&trace.iord, sizeof(char), 1, fp);
			fread(&trace.type, sizeof(char), 1, fp);
			fread(&trace.addr, sizeof(unsigned long long), 1, fp);
			fread(&trace.pc, sizeof(unsigned), 1, fp);
			if(int(trace.type) == 1 ) 
				mycache.process_entry(trace);
		}
		
	    printf("Done reading file %d!\n", k);
	}
	mycache.print_report();
	return 0;
}
