#include "../gbiso/gbiso.h"

/** To disable all the on-screen debug messages just undefine DEBUG **/

int main()
{
    gbIso9660 iso;
    int ret = iso.open_file("TEST.ISO");

    if(ret == OK)
    {
		printf("File main tree\n\n");
		iso.debug_print_main_tree();

		printf("\nFile FOLDER2/ASSEMB.ASM size:\n");
		ret = iso.get_file_size("FOLDER2/ASSEMB.ASM");
		printf("\tReturned %d\n", ret);

		printf("\nFile FOLDER1/FOLDER12/R.PNG offset:\n");
		ret = iso.get_file_location("FOLDER1/FOLDER12/R.PNG");
		printf("\tReturned %08X\n", ret);

		printf("\nExtracting CORN.PNG\n");
		ret = iso.extract_file("FOLDER1/FOLDER12/CORN.PNG", "CORN.PNG");
		printf("\tReturned %d\n", ret);

		printf("\nExtracting All\n");
		ret = iso.extract_all("Extracted_iso");
		printf("\tReturned %d\n", ret);
    }
	else
		printf("Cant open file");

    return 0;
};
