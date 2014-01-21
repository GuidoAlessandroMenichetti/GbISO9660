#include <iostream>
#include "gbiso/gbiso.h"

using namespace std;

int main()
{
    gbIso9660 iso;
    int ret = iso.open_file("tes3.ISO");
    iso.debug_print_main_tree();
	//iso.extract_file("Launche.dat", "asd.dat");
	//ret = iso.extract_all("elderscrolss");
	printf("ret %d", ret);

	//iso.open_file("w.iso");
    return 0;
}
