#include<boost/bimap.hpp> 
#include<iostream>
#include<stdio.h>
#include<vector>
#include<string>
#include<string.h>
#include<fstream>
using namespace std;


int main(){
	bimap<int,int> a;
    a.left.insert(make_pair(3,4));
}

