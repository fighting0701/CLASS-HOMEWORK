#include<iostream>
using namespace std;

int main()
{
    cout << "Hello,World!" << endl;
    cout  << "Please input :";
    string s;
    getline(cin,s);
    cout << "Your input is: " << s;
    return 0; 
}