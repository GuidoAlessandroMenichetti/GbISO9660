#ifndef GBISO_TYPES
#define GBISO_TYPES

#define SECTOR_SIZE 0x800
#define VOLUME_DESCRIPTOR_START_SECTOR 0x10
#define VOLUME_DESCRIPTOR_SIGNATURE "CD001"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

typedef struct s_both_endian_32
{
	u32 little;
	u32 big;
} both_endian_32;

typedef struct s_both_endian_16
{
	u16 little;
	u16 big;
} both_endian_16;

typedef struct s_date_time
{
	u8 year[4];
	u8 month[2];
	u8 day[2];
	u8 hour[2];
	u8 minute[2];
	u8 second[2];
	u8 mil[2];
	u8 gmt;
} date_time;

typedef struct s_volume_descriptor
{
	u8 type;
	u8 magic[5];
	u8 version;
	u8 data[2041];
} volume_descriptor;

typedef struct s_boot_record
{
	u8 type;
	u8 magic[5];
	u8 version;
	u8 boot_sys_id[32];
	u8 boot_id[32];
	u8 boot_sys_use[1977];
} boot_record;

typedef struct s_primary_volume_descriptor
{
	u8 type;
	u8 magic[5];
	u8 version;
	u8 unused;
	u8 sys_id[32];
	u8 vol_id[32];
	u8 unused2[8];
	both_endian_32 vol_size;
	u8 unused3[32];

	both_endian_16 vol_count;
	both_endian_16 vol_index;
	both_endian_16 logical_block_size;
	both_endian_32 path_table_size;

	u32 path_table_location_LSB;
	u32 path_table_optional_location_LSB;
	u32 path_table_location_MSB;
	u32 path_table_optional_location_MSB;

	u8 root_entry[34];

	u8 vol_set_id[128];
	u8 publisher_id[128];
	u8 data_preparer_id[128];
	u8 app_id[128];
	u8 copyright_file[38];
	u8 abstract_file[36];
	u8 biblio_file[37];

	date_time vol_creation;
	date_time vol_modif;
	date_time vol_expiration;
	date_time vol_effective;

	u8 file_structure_version;
	u8 unused4;

	u8 extra_data[512];
	u8 reserved[653];
} __attribute__((__packed__)) primary_volume_descriptor;

typedef struct s_path_entry
{
	u8 name_length;
	u8 extended_length;
	u32 location;
	u16 parent;
} __attribute__((__packed__)) path_entry ;

typedef struct s_directory_record
{
	u8 length;
	u8 extended_length;
	both_endian_32 location;
	both_endian_32 data_length;
	u8 date[7]; //arreglar
	u8 flags;
	u8 unit_size;
	u8 gap_size;
	both_endian_16 sequence_number;
	u8 length_file_id; //files end with ;1
	//file id
	//padding
	//system use
} __attribute__((__packed__)) directory_record;

enum e_directory_record_flags
{
	FLAG_HIDDEN = 1,
	FLAG_DIRECTORY = 2,
	FLAG_ASSOCIATED = 4,
	FLAG_EXTENDED = 8,
	FLAG_OWNER = 16,
	FLAG_RESERVED1 = 32,
	FLAG_RESERVED2 = 64,
	FLAG_NOT_FINAL = 128
};

enum e_volume_descriptor_type
{
	BOOT_RECORD = 0x0,
	PRIMARY_VOLUME_DESCRIPTOR = 0x1,
	SUPPLEMENTARY_VOLUME_DESCRIPTOR = 0x2,
	VOLUME_PARTITION_DESCRIPTOR = 0x3,
	VOLUME_SET_TERMINATOR = 0xFF
};

enum e_endianness
{
	LITTLE_ENDIAN = 0,
	BIG_ENDIAN = 1
};

enum e_file_type
{
	ENTRY_FILE,
	ENTRY_DIRECTORY
};

#endif
