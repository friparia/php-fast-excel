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

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_fast_excel.h"

/* If you declare any globals in php_fast_excel.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(fast_excel)
*/

/* True global resources - no need for thread safety here */
static int le_fast_excel;

PHP_FUNCTION(excel_get_array){
	char *filename;
	int filename_len;


	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE){
		return;
	}

	FILE *inputstream = fopen(filename, "rb");
	if(inputstream == NULL){
		php_error(E_WARNING, "File %s not found", filename);
		RETURN_FALSE;
	}

	mcb_directory_stream *excel_stream = malloc(10);
	excel_stream = mcd_read("excel");

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
