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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define DEBUG 1

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_fast_excel.h"

#include <math.h>
/* If you declare any globals in php_fast_excel.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(fast_excel)
*/

/* True global resources - no need for thread safety here */
static int le_fast_excel;

PHP_FUNCTION(excel_get_array){
	char *filename;
	int filename_len;
	int i,j;


	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE){
		return;
	}

	FILE *inputstream = fopen(filename, "rb");
	if(inputstream == NULL){
		php_error(E_WARNING, "File %s not found", filename);
		RETURN_FALSE;
	}

	mcb_header header; 
	if(fread(&header, sizeof(unsigned char), 512, inputstream) == 0){
		php_error(E_WARNING, "File %s's header not found", filename);
		RETURN_FALSE;
	}

#if DEBUG
	php_printf("\nmcb_mark is");
	for(i = 0;i < 8;i++){
		php_printf("%3x", header.mcb_mark[i]);
	}
	php_printf("\nmcb_indetify is");
	for(i = 0;i < 16;i++){
		php_printf("%3x", header.mcb_identify[i]);
	}
	php_printf("\nrevision=%d", header.revision);
	php_printf("\nversion=%d", header.version);
	php_printf("\nendianess=%d", header.endianness);
	php_printf("\nsector size=%d", header.sector_size);
	php_printf("\nshort sector size=%d", header.short_sector_size);
	php_printf("\nsat sector num=%d", header.sat_sector_num);
	php_printf("\nfirst dir stream sid=%d", header.first_dir_stream_sid);
	php_printf("\nstandard stream size=%d", header.standard_stream_size);
	php_printf("\nfirst ssat sector sid=%d", header.first_ssat_sector_sid);
	php_printf("\nssat sector num=%d", header.ssat_sector_num);
	php_printf("\nfirst msat sector sid=%d", header.first_msat_sector_sid);
	php_printf("\nmsat sector num=%d", header.msat_sector_num);
	php_printf("\nmsat array={");
	for(i = 0;i < 109; i++){
		php_printf("%d,",header.first_msat_id_block[i]);
	}
	php_printf("}\n");
#endif

	int sector_size = pow(2, header.sector_size);
	int short_sector_size  = pow(2, header.short_sector_size);

	if(header.endianness != -2){
		php_error(E_WARNING, "Big-Endian not supported!");
		RETURN_FALSE;
	}

	//READ THE MSAT
	int first_msat_num = 0;
	for(i = 0; i < 109; i++){
		if(header.first_msat_id_block[i] != -1){
			first_msat_num++;
		}
	}

	int msat_id_num = first_msat_num + header.msat_sector_num * (sector_size/sizeof(int));
#if DEBUG
	php_printf("\nmsat_num=%d", msat_id_num);
#endif

	int msat_sid[msat_id_num];
	for(i = 0; i < msat_id_num; i++){
		msat_sid[i] = -1;
	}

	memcpy(msat_sid, header.first_msat_id_block, sizeof(int) * first_msat_num);

	int mast_int_sector[128];
	int p_msat_sector = header.first_msat_sector_sid;
	for(i = 0; i < header.msat_sector_num; i++){
		fseek(inputstream, 512 + sector_size * p_msat_sector, SEEK_SET);
		fread(mast_int_sector, sizeof(int), sector_size / sizeof(int), inputstream);
		for(j = 0; j < 127; j++){
			if(mast_int_sector[j] >= 0){
				msat_sid[109 + i * 128 + j - i] = mast_int_sector[j];
			}else{
				msat_sid[109 + i * 128 + j - i] = -1;
			}
		}
		if(mast_int_sector[127] != -2){
			p_msat_sector = mast_int_sector[127];
		}else{
			break;
		}
	}

	int msat_num = 0;

	for(i = 0; i < msat_id_num; i++){
		if(msat_sid[i] >= 0){
			msat_num++;
		}
	}

#if DEBUG
	php_printf("\nmsat array={");
	for(i = 0;i < msat_num; i++){
		php_printf("%d,",msat_sid[i]);
	}
	php_printf("}\n");
#endif

	int msat_size = sector_size * msat_num;
	int sat_sid[msat_size/sizeof(int)];
	for(i = 0; i < msat_size/4; i++){
		sat_sid[i] = -1;
	}

	for(i = 0; i < msat_num; i++){
		fseek(inputstream, 512 + msat_sid[i] * sector_size, SEEK_SET);
		if(fread(&sat_sid[i * sector_size / sizeof(int)], 1, sector_size, inputstream) == 0){
			php_error(E_WARNING, "MSAT cannot loaded");
			RETURN_FALSE;
		}
	}

#if DEBUG
	php_printf("\nsat array={");
	for(i = 0; i < msat_size/4; i++){
		/* php_printf("%d,", sat_sid[i]); */
	}
	php_printf("}");
#endif
	
	//READ THE 2nd DIR which is the workbook
	mcb_dir workbook;
	fseek(inputstream, 512 + 512 * header.first_dir_stream_sid + sizeof(mcb_dir) , SEEK_SET);
	fread(&workbook, 1, sizeof(mcb_dir), inputstream);

	unsigned char record_sector[sector_size];
	unsigned char *record_stream = malloc(workbook.stream_size);
	int sid = workbook.first_ssat_sector_sid;
	i = 0;
	fseek(inputstream, 512 + sid * sector_size, SEEK_SET);
	while(sid != END_OF_CHAIN_SID){
		fread(record_sector, sizeof(unsigned char), sector_size, inputstream);
		memcpy(&record_stream[i * sector_size], record_sector, sector_size);
		sid = sat_sid[sid];
		i++;
	}

	//record_stream is what we want
	biff_record record;
	unsigned char *data;
	index_record index;
	int rows = 0;
	i = 0;
	while(i < workbook.stream_size){
		memcpy(&record, &record_stream[i], 4);
		data = malloc(record.length);
		memcpy(data, &record_stream[i + 4], record.length);
#if DEBUG
		/* php_printf("\nRECORD_NUM=0x%4x, RECORD_LENGTH=%4d, RECORD_DATA=%s", record.number, record.length, data); */
#endif
		if(record.number == INDEX_RECORD){
			memcpy(&index, data, 16);
			unsigned int *cell_array = malloc(record.length - 16);
			memcpy(cell_array, data + 16, record.length - 16);
			index.cell_array = cell_array;
			rows = (record.length - 16) / 4;

#if DEBUG
			php_printf("\nfirst row=%d, last row=%d, cell_array={", index.first_row, index.last_row);
			for(i = 0; i < (record.length - 16 ) / 4; i++){
				php_printf("0x%x,", *index.cell_array + i * 4);
			}
			php_printf("}");
#endif
			break;
		}
		free(data);
		i = i + record.length + 4;
	}





	


}
ZEND_BEGIN_ARG_INFO(arginfo_excel_get_array, 0)
ZEND_END_ARG_INFO()


PHP_FUNCTION(excel_write_array){
}
ZEND_BEGIN_ARG_INFO(arginfo_excel_write_array, 0)
ZEND_END_ARG_INFO()

/* {{{ fast_excel_functions[]
 *
 * Every user visible function must have an entry in fast_excel_functions[].
 */
const zend_function_entry fast_excel_functions[] = {
	PHP_FE(confirm_fast_excel_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(excel_get_array,	arginfo_excel_get_array)		/* For testing, remove later. */
	PHP_FE(excel_write_array,	arginfo_excel_write_array)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in fast_excel_functions[] */
};
/* }}} */

/* {{{ fast_excel_module_entry
 */
zend_module_entry fast_excel_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"fast_excel",
	fast_excel_functions,
	PHP_MINIT(fast_excel),
	PHP_MSHUTDOWN(fast_excel),
	PHP_RINIT(fast_excel),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(fast_excel),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(fast_excel),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_FAST_EXCEL_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FAST_EXCEL
ZEND_GET_MODULE(fast_excel)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("fast_excel.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_fast_excel_globals, fast_excel_globals)
    STD_PHP_INI_ENTRY("fast_excel.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_fast_excel_globals, fast_excel_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_fast_excel_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_fast_excel_init_globals(zend_fast_excel_globals *fast_excel_globals)
{
	fast_excel_globals->global_value = 0;
	fast_excel_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(fast_excel)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(fast_excel)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(fast_excel)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(fast_excel)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(fast_excel)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "fast_excel support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_fast_excel_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_fast_excel_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "fast_excel", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */


/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
