#include "gbiso.h"

gbIso9660 :: gbIso9660()
{
	stream = NULL;
	tree = NULL;
};

gbIso9660 :: ~gbIso9660()
{
	close_file();
	delete_main_tree();
};

int gbIso9660 :: load_primary_volume_descriptor()
{
	primary_volume_descriptor pvd;

	//Go to primary volume descriptor sector and read
	if(read_sector(VOLUME_DESCRIPTOR_START_SECTOR, &pvd) != OK)
		return ERROR_INVALID;

	//Check it is a primary volume descriptor
	if(pvd.type != PRIMARY_VOLUME_DESCRIPTOR)
		return ERROR_INVALID;

#ifdef DEBUG
	printf("VD INFO\n\ttype: %d\n\tmagic: %s\n\tversion: %d\n", pvd.type, pvd.magic, pvd.version);
	printf("\n");
#endif

	//Store in the object
	this->pvd = pvd;

	return OK;
};

int gbIso9660 :: load_directory_record(u32 path_entry_location, vector<tree_entry> * te)
{
	directory_record dr;
	u8 * name = NULL;
	u32 restore = ftell(stream);
	int flag = 0;
	unsigned bytes_read = 0;

#ifdef DEBUG
	printf("\tDir record loc %08X\n", path_entry_location);
#endif

	//Go to directory record offset
	fseek(stream, path_entry_location * SECTOR_SIZE, SEEK_SET);
	fread(&dr, sizeof(directory_record), 1, stream);

	//Enter directory record load loop
	while(dr.length)
	{
		//Get entry name
		name = new u8[dr.length_file_id + 1];
		fread(name, dr.length_file_id, 1, stream);
		*(name + dr.length_file_id) = '\0';

		//Check odd and system use padding
		fseek(stream, dr.length - (sizeof(directory_record) + dr.length_file_id), SEEK_CUR);

		//Create the tree entry for this directory
		tree_entry new_te;
		new_te.childs = NULL;
		new_te.name = name;
		new_te.location = dr.location.little;
		new_te.size = dr.data_length.little;

#ifdef DEBUG
		printf("\t\tnew entry %s\n\t\tloc: %08X\n\t\tsize: %08X\n\t\tflags %d\n", new_te.name, new_te.location, new_te.size, dr.flags);
		printf("\t\tfile_id_len %08X\textra_len %d gap %d dr len %08X\n\n", dr.length_file_id, dr.extended_length, dr.gap_size, dr.length);
#endif

		if(/*dr.length_file_id != 1*/ flag >= 2 /* name != '\0'*/)
		{
			if((dr.flags & FLAG_DIRECTORY))
			{
				//Do recursive loops if its a directory
				new_te.childs = new vector<tree_entry>();
				load_directory_record(new_te.location, new_te.childs);
			}
			else
			{
				//Some ISO software does not write the ";1" string
				if(!memcmp(name + dr.length_file_id - 2, ";1", 2))
					*(name + dr.length_file_id - 2) = '\0';
			};

			//Add to the tree
			te->push_back(new_te);
		};

		//Directory entries cant exceed sectors, check padding
		bytes_read += dr.length;
		if(bytes_read + sizeof(directory_record) + 8 > SECTOR_SIZE)
		{
			fseek(stream, SECTOR_SIZE - bytes_read, SEEK_CUR);
			bytes_read = 0;
		};

		flag++;
		fread(&dr, sizeof(directory_record), 1, stream);
	};

	fseek(stream, restore, SEEK_SET);

	return OK;
};

int gbIso9660 :: create_path_table()
{
	//Adjust endianness
	u32 path_table_location;
	//u32 path_table_size;

	path_table_location = pvd.path_table_location_LSB;
	//path_table_size = pvd.path_table_size.little;

#ifdef DEBUG
	printf("path table loc: %08X\n", path_table_location);
#endif

	//Go to path table offset
	if(fseek(stream, path_table_location * SECTOR_SIZE, SEEK_SET))
		return ERROR_INVALID;

	//Read root dir info from path table
	path_entry pe;
	fread(&pe, sizeof(path_entry), 1, stream);

	//Make the main tree
	tree = new vector<tree_entry>();
	load_directory_record(pe.location, tree);

	return OK;
};

int gbIso9660 :: open_file(const char * file)
{
	stream = fopen(file, "rb");

	if(!stream)
		return ERROR_NOT_OPENED;

	//Check the file is at least 0x8000 long
	if(fseek(stream, VOLUME_DESCRIPTOR_START_SECTOR * SECTOR_SIZE, SEEK_SET))
	{
		fclose(stream);
		return ERROR_INVALID;
	};

	//Check the volume descriptor signature
	volume_descriptor vd;
	fread(&vd, sizeof(volume_descriptor), 1, stream);
	if(memcmp(&vd.magic, VOLUME_DESCRIPTOR_SIGNATURE, sizeof(vd.magic)))
	{
		fclose(stream);
		return ERROR_NOT_ISO;
	};

	//Load primary volume descriptor
	if(load_primary_volume_descriptor() != OK)
		return ERROR_PRIMARY_VOLUME_DESCRIPTOR;

	//Create ISO filesystem tree
	delete_main_tree();
	if(create_path_table() != OK)
		return ERROR_PATH_TABLE;

	return OK;
};

void gbIso9660 :: close_file()
{
	if(stream)
		fclose(stream);
	stream = NULL;
};

unsigned gbIso9660 :: get_file_location(const char * path)
{
	tree_entry * file = get_file_info(path);
	if(file)
		return file->location * SECTOR_SIZE;

	return 0;
};

unsigned gbIso9660 :: get_file_size(const char * path)
{
	tree_entry * file = get_file_info(path);
	if(file)
		return file->size;

	return 0;
};

tree_entry * gbIso9660 :: get_file_info(const char * path)
{
	tree_entry * ret = NULL;

	if(tree)
	{
		//Make a copy of the path
		char * tmp = new char[strlen(path) + 1];
		strcpy(tmp, path);

		//Check for format errors
		fix_string(tmp);

		//Start search
		ret = get_file_info_recursive(tmp, tree);

		delete [] tmp;
	};

	return ret;
};

tree_entry * gbIso9660 :: get_file_info_recursive(const char * path, vector<tree_entry> * te)
{
	int type = ENTRY_DIRECTORY;

	//Get the directory name length
	char * file_name_end = strchr(path, '/');

	//If not, get the filename length
	if(!file_name_end)
	{
		file_name_end = file_name_end + strlen(path);
		type = ENTRY_FILE;
	};

	u32 name_len = file_name_end - path;
	for(unsigned i = 0; i < te->size(); i++)
	{
		//If filename is the same
		if(!strncmp((const char *)((*te)[i].name), path, name_len))
		{
			//Check if its a directory or file
			if(type == ENTRY_FILE)
				return &(*te)[i];
			else
				return get_file_info_recursive(path + name_len + 1, (*te)[i].childs);
		};
	};

	return NULL;
};

int gbIso9660 :: extract_file(const char * file_name, const char * extract_path)
{
	tree_entry * te = get_file_info(file_name);

	//Check for the file data on the tree
	if(!te)
		return ERROR_NOT_FOUND;

	//Create the output file
	return copy_bytes(te->location * SECTOR_SIZE, te->size, extract_path);
};

int gbIso9660 :: extract_all(const char * extract_path)
{
	if(tree)
	{
		//Create the main path
		if(mkdir(extract_path) <= 0)
			return extract_recursive(extract_path, tree);
		else
			return ERROR_CANT_CREATE;
	};

	return ERROR_NOT_OPENED;
};

int gbIso9660 :: extract_recursive(const char * extract_path, vector<tree_entry> * te)
{
	int ret;

	for(unsigned i=0; i< te->size(); i++)
	{
		//Build the complete path
		char * create = new char[strlen(extract_path) + strlen((const char *)(*te)[i].name) + 2];
		strcpy(create, extract_path);
		strcat(create, "/");
		strcat(create, (const char *)(*te)[i].name);

#ifdef DEBUG
		printf("Extracting %s..\n", create);
#endif

		if((*te)[i].childs)
		{
			//Its a folder
			mkdir(create);
			ret = extract_recursive(create, (*te)[i].childs);
		}
		else //Its a file
			ret = copy_bytes((*te)[i].location * SECTOR_SIZE, (*te)[i].size, create);

		delete [] create;

		if(ret != OK)
			return ERROR_CANT_CREATE;
	};

	return OK;
};

int gbIso9660 :: copy_bytes(unsigned offset, unsigned size, const char * out)
{
	FILE * output = fopen(out, "wb");
	if(!output)
		return ERROR_CANT_CREATE;

	unsigned read_size;
	char buffer[SECTOR_SIZE];

	fseek(stream, offset, SEEK_SET);

	//Read and write each bytes block
	while(size)
	{
		read_size = size > SECTOR_SIZE? SECTOR_SIZE: size;

		fread(buffer, read_size, 1, stream);
		fwrite(buffer, read_size, 1, output);

		size -= read_size;
	};

	fclose(output);

	return OK;
};

void gbIso9660 :: delete_main_tree()
{
	if(tree)
	{
		delete_tree_recursive(tree);
		tree = NULL;
	};
};

void gbIso9660 :: delete_tree_recursive(vector<tree_entry> * te)
{
	//Do a recursive delete
	for(unsigned i=0; i< te->size(); i++)
	{
		if((*te)[i].childs)
			delete_tree_recursive((*te)[i].childs);

		delete [] (*te)[i].name;
		delete (*te)[i].childs;
	};

	te->clear();
};

void gbIso9660 :: debug_print_main_tree()
{
	if(tree)
		debug_print_tree_recursive(tree);
};

void gbIso9660 :: debug_print_tree_recursive(vector<tree_entry> * te)
{
	for(unsigned i=0; i< te->size(); i++)
	{
		printf("%s\n", (*te)[i].name);
		if((*te)[i].childs)
			debug_print_tree_recursive((*te)[i].childs);
	};
};

int gbIso9660 :: read_sector(u32 sector, void * store)
{
	int ret = OK;
	if(!stream)
		return ERROR_NOT_OPENED;

	//Get the current offset to restore
	u32 restore = ftell(stream);

	//Read sector
	if(!fseek(stream, sector * SECTOR_SIZE, SEEK_SET))
		fread(store, SECTOR_SIZE, 1, stream);
	else
		ret = ERROR_INVALID;

	//Restore file pointer
	fseek(stream, restore, SEEK_SET);

	return ret;
};

void gbIso9660 :: fix_string(char * str)
{
	//Make a path ISO9660 compatible
	char * store = str;
	char * aux;

	int len = strlen(str);

	for(int i = 0; i < len; i++)
	{
		//Only uppercase allowed
		if(str[i] >= 'a' && str[i] <= 'z')
			str[i] = str[i] - 0x20;

		if(str[i] == '\\')
			str[i] = '/';
	};

	while(* str)
	{
		aux = strchr(str, '/');
		if(aux)
		{
			//Its a directory name (limit 8)
			len = (int)(aux - str);
			if(len > 8)
			{
				strncpy(store, str, 6);
				strcpy(store + 6, "~0/");
				store += 9;
			}
			else
			{
				strncpy(store, str, len + 1);
				store += len + 1;
			};
			str += len + 1;
		}
		else
		{
			//Its a file name
			len = strlen(str);
			strncpy(store, str, len);
			store += len;

			break;
		};
	};

	* store = '\0';
};
