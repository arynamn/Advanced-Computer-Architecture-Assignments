#include<bits/stdc++.h>
using namespace std;
typedef struct 
{
	char                type;
    int                 threadid;
	unsigned long long  addr;
}trace_entry;

map< unsigned long long int , set<unsigned long long int>> threads_used;
map< unsigned long long int , unsigned long long int> sharing_profile;   

void increment(unsigned long long int d)
{
    if( sharing_profile.find(d) == sharing_profile.end() )
    {
        sharing_profile[d] = 1;
    }
    else
    {
        sharing_profile[d]++;
    }
}
int compute_threads()
{
    int N = 0;
    for (auto const &kv : threads_used) 
    {
        int t = kv.second.size();
        increment(t);
    }
    return N;
}
void store_values(char *out)
{
    unsigned long long int prev_count = 0;
    FILE *fp;

    string s = out;
    s.append("_sharing_profile.out");
    fp = fopen(s.c_str(),"w");
    for (auto &kv : sharing_profile) 
    {
       fprintf(fp,"%llu  %llu\n",kv.first, kv.second);    
    }
}

int main(int argc , char **argv)
{	
    set<unsigned long long int> distincts;
    FILE *fp;
	int count = 0;
	trace_entry trace;	
	fp = fopen(argv[1], "rb");
	assert(fp != NULL);
	while (!feof(fp)) 
	{
	    	fscanf(fp,"%c  %d  %llu\n", &trace.type, &trace.threadid, &trace.addr);
        	trace.addr  = trace.addr>>6;
        	distincts.insert(trace.addr);
        	threads_used[trace.addr].insert(trace.threadid);
        	count++;
		cout<<count<<"\r";
	}
    unsigned long long int N = compute_threads();
    store_values(argv[1]);
    cout<<" Distinct Blocks : "<<distincts.size()<<endl;
    return 0;
}
