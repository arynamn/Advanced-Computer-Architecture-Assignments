#include<iostream>
#include<fstream>
#include<cstring>
using namespace std;

int main()
{
    fstream newfile;
    newfile.open("files.txt",ios::in); 
    if (newfile.is_open())
    {   
        string tp;
        while(getline(newfile, tp))
        { 
            string inc =  "./inclusive  " + tp;
            string exl =  "./exclusive  " + tp;
            string nine = "./nine  " + tp;
            
            cout<<"Report for : "<<tp<<endl;
            cout<<"\nInclusive"<<endl;
            system(inc.c_str());
            cout<<"\nExclusive"<<endl;
            system(exl.c_str());
            cout<<"\nNine"<<endl;
            system(nine.c_str());
        }
        newfile.close(); 
    }
}