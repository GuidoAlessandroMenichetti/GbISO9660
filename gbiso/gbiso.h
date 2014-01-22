/**

	GBISO9660 v1.0 (22/01/14)
	Iso 9660 Library 32-Bit
	by GUIDOBOT
	http://github.com/GUIDOBOT/
	
**/

#ifndef GBISO_GBISO
#define GBISO_GBISO

#include "types.h"
#include <stdio.h>
#include <string.h>
#include <vector>

#define _POSIX_SOURCE
#include <sys/stat.h>
#include <unistd.h>
#undef _POSIX_SOURCE

#define DEBUG

using namespace std;

enum e_gbiso_returns
{
	ERROR_NOT_OPENED = 0x0,
	ERROR_INVALID = 0x1,
	ERROR_NOT_ISO = 0x2,
	ERROR_PRIMARY_VOLUME_DESCRIPTOR = 0x3,
	ERROR_PATH_TABLE = 0x4,
	ERROR_NOT_FOUND = 0x5,
	ERROR_CANT_CREATE = 0x6,
	OK = 0xFF
};

typedef struct s_tree_entry
{
	vector<struct s_tree_entry> * childs;
	u8 * name;
	u32 location;
	u32 size;
} tree_entry;

class gbIso9660
{
	public:
	gbIso9660();
	~gbIso9660();

	int open_file(const char * file);

	unsigned get_file_location(const char * path);
	unsigned get_file_size(const char * path);

	int extract_file(const char * file_name, const char * extract_path);
	int extract_all(const char * extract_path);

	void debug_print_main_tree();

	static void fix_string(char * str);

	private:

	void close_file();
	int read_sector(u32 sector, void * store);

	int load_primary_volume_descriptor();

	int create_path_table();
	int load_directory_record(u32 path_entry_location, vector<tree_entry> * te);
	void delete_main_tree();

	void debug_print_tree_recursive(vector<tree_entry> * te);
	void delete_tree_recursive(vector<tree_entry> * te);
	int extract_recursive(const char * extract_path, vector<tree_entry> * te);

	tree_entry * get_file_info(const char * path);
	tree_entry * get_file_info_recursive(const char * path, vector<tree_entry> * te);

	int copy_bytes(unsigned offset, unsigned size, const char * out);

	primary_volume_descriptor pvd;
	FILE * stream;
	vector<tree_entry> * tree;
};

#endif
