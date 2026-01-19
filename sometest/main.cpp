#include <cstdio>  
#include <string>  
#include <iostream>  

int main()  
{  
	std::string sometest = u8"sometest干一杯";  

	std::cout << sometest.length() << "  "<<sometest.size() << std::endl;

    printf("%s 向你问好!\n", "sometest");  
    return 0;  
}