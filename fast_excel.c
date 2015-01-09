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

#define DEBUG 0

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_fast_excel.h"

#include <math.h>
#include <iconv.h>
/* If you declare any globals in php_fast_excel.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(fast_excel)
*/

/* True global resources - no need for thread safety here */
static int le_fast_excel;

PHP_FUNCTION(excel_get_array){
	char *filename;
	int filename_len;
	int i,j,k;
	array_init(return_value);
	zval *row_array;


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
	unsigned char *data;
	index_record index;
	int dbcell_num = 0;
	i = 0;
	int fdcells = 0;
	int sst_read = 0;
	int index_read = 0;
	unsigned int total_string_num = 0;
	unsigned int unique_string_num = 0;
	unsigned char *unicode_string_array;
	unsigned int unicode_string_array_length;
	while(i < workbook.stream_size){
		biff_record record;
		memcpy(&record, &record_stream[i], 4);
		data = malloc(record.length);
		memcpy(data, &record_stream[i + 4], record.length);
#if DEBUG
		php_printf("\nRECORD_NUM=0x%4x, RECORD_LENGTH=%4d, RECORD_DATA=%s", record.number, record.length, data);
#endif
#if DEBUG
		if(record.number == 0xfd)
			fdcells++;
#endif
		if(record.number == INDEX_RECORD){
			memcpy(&index, data, 16);
			unsigned int *cell_array = malloc(record.length - 16);
			memcpy(cell_array, data + 16, record.length - 16);
			index.cell_array = cell_array;
			dbcell_num = (record.length - 16) / 4;

#if DEBUG
			php_printf("\nfirst row=%d, last row=%d, cell_array={", index.first_row, index.last_row);
			for(i = 0; i < (record.length - 16 ) / 4; i++){
				php_printf("%d,", *(index.cell_array + i));
			}
			php_printf("}");
			index_read = 1;
#endif
		}
		if(record.number == SST_RECORD){
			biff_record sst_record;
			memcpy(&sst_record, &record_stream[i], 4);
			memcpy(&total_string_num, &record_stream[i + 4], 4);
			memcpy(&unique_string_num, &record_stream[i + 8], 4);
			unicode_string_array = malloc(sst_record.length - 8);
			unicode_string_array_length = sst_record.length - 12;
			memcpy(unicode_string_array, &record_stream[i + 12], sst_record.length - 12);
			sst_read = 1;
		}

		if(sst_read && index_read){
			break;
		}
		free(data);
		i = i + record.length + 4;
	}

	i = 0;
	unsigned char **string_table = malloc(sizeof(unsigned char*) * unique_string_num);
	k = 0;
	while(i < unicode_string_array_length){
		unsigned char *string;
		unsigned int string_length;
		unicode_string unicode;
		memcpy(&unicode, &unicode_string_array[i], 3);
		memcpy(&unicode.ext_length, &unicode_string_array[i + 3], 4);

		if(unicode.flag == 5){
			string_length = 3 * unicode.char_num + 1;
			string = (unsigned char *)malloc(string_length);
			string[string_length] = '\0'; 
			for(j = 0; j < unicode.char_num; j++){
				int unic = 0;
				unsigned char *word = (unsigned char*)malloc(3);
				unic = unicode_string_array[i + 7 + j] ;
				memcpy(&unic, &unicode_string_array[i + 7 + j * 2 ], 2);
				unicode2utf8(unic, word, 3);
				memcpy(&string[j * 3], word, 3);
			}
		}else{
			string_length = 1 * unicode.char_num;
			string = (unsigned char *)malloc(string_length + 1);
			string[string_length] = '\0';
			memcpy(string, &unicode_string_array[i + 7], string_length);
		}
/* #if DEBUGa */
		/* php_printf("\n%d", sizeof(*string)); */
/* #endif */
		*(string_table + k) = string;
		k++;
		i += 2 + 1 + 4 + string_length +  unicode.ext_length;
	}

#if DEBUG
	php_printf("\ntotal=%d, unique=%d", total_string_num, unique_string_num);
#endif

	int dbcell_offset = 0;
	int row_offset = 0;
	int fdreads = 0;
	//each db
	for(i = 0; i < dbcell_num; i++){
		biff_record dbcell_record;
		dbcell cell;
		unsigned short* offset_array;
		dbcell_offset = *(index.cell_array + i);
		memcpy(&dbcell_record, &record_stream[dbcell_offset], 4);
		memcpy(&cell, &record_stream[dbcell_offset + 4], 4);
		offset_array = malloc(dbcell_record.length - 4);
		memcpy(offset_array, &record_stream[dbcell_offset + 8], dbcell_record.length - 4);
		row_offset = dbcell_offset - cell.offset_to_row; 

		//first row
		biff_record first_row_record;
		memcpy(&first_row_record, &record_stream[row_offset], 4);
		row_record row;
		memcpy(&row, &record_stream[row_offset + 4], 8);
		row_offset += first_row_record.length + 4;


		//each row
		for(j = 0; j < (dbcell_record.length - 4)/2; j++){
			MAKE_STD_ZVAL(row_array);
			array_init(row_array);
			row_offset += *(offset_array + j);
			biff_record row_first_cell_record;
			memcpy(&row_first_cell_record, &record_stream[row_offset], 4);
			int cell_offset = row_offset;
			//each cell
			for(k = 0; k < row.last_row - row.first_row; k++){
				fdreads++;
				biff_record cell_record;
				memcpy(&cell_record, &record_stream[cell_offset], 4);
#if DEBUG
				php_printf("\nRECORD_NUM=0x%4x, RECORD_LENGTH=%4d", cell_record.number, cell_record.length);
#endif
				labelsst_record labelsst;
				memcpy(&labelsst.sst_index, &record_stream[cell_offset + 10], 4);

				if(cell_record.number == LABELSST_RECORD){
					add_next_index_string(row_array, *(string_table + labelsst.sst_index), 1); //write to php row 
#if DEBUG
					php_printf("\n%d", labelsst.sst_index);
					php_printf("\n%s", *(string_table + labelsst.sst_index));
#endif
				}
				cell_offset += 4 + cell_record.length;
			}
			zend_hash_index_update(Z_ARRVAL_P(return_value), i * 32 + j, (void *)&row_array, sizeof(zval *), NULL);
		}
	}

#if DEBUG
	php_printf("\nhas %d, read %d", fdcells, fdreads);
#endif
}
ZEND_BEGIN_ARG_INFO(arginfo_excel_get_array, 0)
ZEND_END_ARG_INFO()

int unicode2utf8(unsigned long unic, unsigned char *pOutput,  int outSize){  
  
    if ( unic <= 0x0000007F )  
    {  
        // * U-00000000 - U-0000007F:  0xxxxxxx  
        *pOutput     = (unic & 0x7F);  
        return 1;  
    }  
    else if ( unic >= 0x00000080 && unic <= 0x000007FF )  
    {  
        // * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx  
        *(pOutput+1) = (unic & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 6) & 0x1F) | 0xC0;  
        return 2;  
    }  
    else if ( unic >= 0x00000800 && unic <= 0x0000FFFF )  
    {  
        // * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx  
        *(pOutput+2) = (unic & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >>  6) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 12) & 0x0F) | 0xE0;  
        return 3;  
    }  
    else if ( unic >= 0x00010000 && unic <= 0x001FFFFF )  
    {  
        // * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
        *(pOutput+3) = (unic & 0x3F) | 0x80;  
        *(pOutput+2) = ((unic >>  6) & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >> 12) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 18) & 0x07) | 0xF0;  
        return 4;  
    }  
    else if ( unic >= 0x00200000 && unic <= 0x03FFFFFF )  
    {  
        // * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
        *(pOutput+4) = (unic & 0x3F) | 0x80;  
        *(pOutput+3) = ((unic >>  6) & 0x3F) | 0x80;  
        *(pOutput+2) = ((unic >> 12) & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >> 18) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 24) & 0x03) | 0xF8;  
        return 5;  
    }  
    else if ( unic >= 0x04000000 && unic <= 0x7FFFFFFF )  
    {  
        // * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
        *(pOutput+5) = (unic & 0x3F) | 0x80;  
        *(pOutput+4) = ((unic >>  6) & 0x3F) | 0x80;  
        *(pOutput+3) = ((unic >> 12) & 0x3F) | 0x80;  
        *(pOutput+2) = ((unic >> 18) & 0x3F) | 0x80;  
        *(pOutput+1) = ((unic >> 24) & 0x3F) | 0x80;  
        *pOutput     = ((unic >> 30) & 0x01) | 0xFC;  
        return 6;  
    }  
  
    return 0;  
}  


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
