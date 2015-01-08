/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_FAST_EXCEL_H
#define PHP_FAST_EXCEL_H

extern zend_module_entry fast_excel_module_entry;
#define phpext_fast_excel_ptr &fast_excel_module_entry

#define PHP_FAST_EXCEL_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_FAST_EXCEL_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_FAST_EXCEL_API __attribute__ ((visibility("default")))
#else
#	define PHP_FAST_EXCEL_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(fast_excel);
PHP_MSHUTDOWN_FUNCTION(fast_excel);
PHP_RINIT_FUNCTION(fast_excel);
PHP_RSHUTDOWN_FUNCTION(fast_excel);
PHP_MINFO_FUNCTION(fast_excel);

PHP_FUNCTION(confirm_fast_excel_compiled);	/* For testing, remove later. */
PHP_FUNCTION(excel_get_array);	/* For testing, remove later. */
PHP_FUNCTION(excel_write_array);	/* For testing, remove later. */

typedef struct _mcb_header{
	unsigned char mcb_mark[8];
	unsigned char mcb_identify[16];
	short revision;
	short version;
	short endianness;
	short sector_size;
	short short_sector_size;
	unsigned char reserved[10];
	int sat_sector_num;
	int first_dir_stream_sid;
	unsigned char reserved_2[4];
	int standard_stream_size;
	int first_ssat_sector_sid;
	int ssat_sector_num;
	int first_msat_sector_sid;
	int msat_sector_num;
	int first_msat_id_block[109];
}mcb_header;

typedef struct _mcb_dir{
	unsigned char name[64];
	short name_len;
	char type;
	int l_did;
	int r_did;
	int root_did;
	unsigned char identify[16];
	int user;
	unsigned char create_time[8];
	unsigned char last_modify_time[8];
	int first_ssat_sector_sid;
	int stream_size;
	unsigned char reserved[4];
}mcb_dir;

typedef struct _biff_record{
	short number;
	short length;
	unsigned char *data;
}biff_record;

typedef struct _index_record{
	int reserved;
	unsigned int first_row;
	unsigned int last_row;
	int reserved_2;
	unsigned int *cell_array;
}index_record;

typedef struct _dbcell{
	int offset_to_row;
	unsigned short *offset_array;
}dbcell;

typedef struct _row_record{
	unsigned short number;
	unsigned short first_row;
	unsigned short last_row;
	unsigned short row_height;
}row_record;

typedef struct _labelsst_record{
	unsigned short row;
	unsigned short column;
	unsigned short xf_index;
	unsigned int sst_index;
}labelsst_record;

typedef struct _unicode_string{
	short char_num;
	char flag;
	int ext_length;
	unsigned char *string;
}unicode_string;



#define FREE_SID -1
#define END_OF_CHAIN_SID -2
#define SAT_SID -3
#define MSAT_SID -4

#define ROW_RECORD 0x208
#define LABELSST_RECORD 0xFD
#define INDEX_RECORD 0x20B
#define SST_RECORD 0xFC
/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(fast_excel)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(fast_excel)
*/

/* In every utility function you add that needs to use variables 
   in php_fast_excel_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as FAST_EXCEL_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define FAST_EXCEL_G(v) TSRMG(fast_excel_globals_id, zend_fast_excel_globals *, v)
#else
#define FAST_EXCEL_G(v) (fast_excel_globals.v)
#endif

#endif	/* PHP_FAST_EXCEL_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
