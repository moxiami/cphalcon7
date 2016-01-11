
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2014 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  +------------------------------------------------------------------------+
*/

#include "mvc/url.h"
#include "mvc/urlinterface.h"
#include "mvc/url/exception.h"
#include "mvc/routerinterface.h"
#include "diinterface.h"
#include "di/injectable.h"

#include <Zend/zend_closures.h>

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/exception.h"
#include "kernel/object.h"
#include "kernel/array.h"
#include "kernel/fcall.h"
#include "kernel/framework/url.h"
#include "kernel/concat.h"
#include "kernel/string.h"
#include "kernel/framework/router.h"

#include "interned-strings.h"

/**
 * Phalcon\Mvc\Url
 *
 * This components aids in the generation of: URIs, URLs and Paths
 *
 *<code>
 *
 * //Generate a URL appending the URI to the base URI
 * echo $url->get('products/edit/1');
 *
 * //Generate a URL for a predefined route
 * echo $url->get(array('for' => 'blog-post', 'title' => 'some-cool-stuff', 'year' => '2012'));
 *
 *</code>
 */
zend_class_entry *phalcon_mvc_url_ce;

PHP_METHOD(Phalcon_Mvc_Url, setBaseUri);
PHP_METHOD(Phalcon_Mvc_Url, setStaticBaseUri);
PHP_METHOD(Phalcon_Mvc_Url, getBaseUri);
PHP_METHOD(Phalcon_Mvc_Url, getStaticBaseUri);
PHP_METHOD(Phalcon_Mvc_Url, setBasePath);
PHP_METHOD(Phalcon_Mvc_Url, getBasePath);
PHP_METHOD(Phalcon_Mvc_Url, get);
PHP_METHOD(Phalcon_Mvc_Url, getStatic);
PHP_METHOD(Phalcon_Mvc_Url, path);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_url_setstaticbaseuri, 0, 0, 1)
	ZEND_ARG_INFO(0, staticBaseUri)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_mvc_url_getstatic, 0, 0, 0)
	ZEND_ARG_INFO(0, uri)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_mvc_url_method_entry[] = {
	PHP_ME(Phalcon_Mvc_Url, setBaseUri, arginfo_phalcon_mvc_urlinterface_setbaseuri, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Url, setStaticBaseUri, arginfo_phalcon_mvc_url_setstaticbaseuri, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Url, getBaseUri, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Url, getStaticBaseUri, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Url, setBasePath, arginfo_phalcon_mvc_urlinterface_setbasepath, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Url, getBasePath, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Url, get, arginfo_phalcon_mvc_urlinterface_get, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Url, getStatic, arginfo_phalcon_mvc_url_getstatic, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Mvc_Url, path, arginfo_phalcon_mvc_urlinterface_path, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Mvc\Url initializer
 */
PHALCON_INIT_CLASS(Phalcon_Mvc_Url){

	PHALCON_REGISTER_CLASS_EX(Phalcon\\Mvc, Url, mvc_url, phalcon_di_injectable_ce, phalcon_mvc_url_method_entry, 0);

	zend_declare_property_null(phalcon_mvc_url_ce, SL("_baseUri"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_url_ce, SL("_staticBaseUri"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_url_ce, SL("_basePath"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_mvc_url_ce, SL("_router"), ZEND_ACC_PROTECTED);

	zend_class_implements(phalcon_mvc_url_ce, 1, phalcon_mvc_urlinterface_ce);

	return SUCCESS;
}

/**
 * Sets a prefix for all the URIs to be generated
 *
 *<code>
 *	$url->setBaseUri('/invo/');
 *	$url->setBaseUri('/invo/index.php/');
 *</code>
 *
 * @param string $baseUri
 * @return Phalcon\Mvc\Url
 */
PHP_METHOD(Phalcon_Mvc_Url, setBaseUri){

	zval *base_uri, *static_base_uri;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &base_uri);

	phalcon_update_property_this(getThis(), SL("_baseUri"), base_uri);

	static_base_uri = phalcon_read_property(getThis(), SL("_staticBaseUri"), PH_NOISY);
	if (Z_TYPE_P(static_base_uri) == IS_NULL) {
		phalcon_update_property_this(getThis(), SL("_staticBaseUri"), base_uri);
	}

	RETURN_THIS();
}

/**
 * Sets a prefix for all static URLs generated
 *
 *<code>
 *	$url->setStaticBaseUri('/invo/');
 *</code>
 *
 * @param string $staticBaseUri
 * @return Phalcon\Mvc\Url
 */
PHP_METHOD(Phalcon_Mvc_Url, setStaticBaseUri){

	zval *static_base_uri;

	phalcon_fetch_params(0, 1, 0, &static_base_uri);

	phalcon_update_property_this(getThis(), SL("_staticBaseUri"), static_base_uri);
	RETURN_THISW();
}

/**
 * Returns the prefix for all the generated urls. By default /
 *
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Url, getBaseUri){

	zval *base_uri, slash, *_SERVER, php_self, uri;

	PHALCON_MM_GROW();

	base_uri = phalcon_read_property(getThis(), SL("_baseUri"), PH_NOISY);
	if (Z_TYPE_P(base_uri) == IS_NULL) {
		ZVAL_STRING(&slash, "/");
		_SERVER = phalcon_get_global_str(SL("_SERVER"));
		if (phalcon_array_isset_fetch_str(&php_self, _SERVER, SL("PHP_SELF"))) {
			phalcon_get_uri(&uri, &php_self);
		}

		if (!zend_is_true(&uri)) {
			ZVAL_COPY_VALUE(base_uri, &slash);
		} else {
			PHALCON_CONCAT_VVV(base_uri, &slash, &uri, &slash);
		}

		phalcon_update_property_this(getThis(), SL("_baseUri"), base_uri);
	}

	RETURN_CCTOR(base_uri);
}

/**
 * Returns the prefix for all the generated static urls. By default /
 *
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Url, getStaticBaseUri){

	zval *static_base_uri;

	PHALCON_MM_GROW();

	static_base_uri = phalcon_read_property(getThis(), SL("_staticBaseUri"), PH_NOISY);
	if (Z_TYPE_P(static_base_uri) != IS_NULL) {
		RETURN_CCTOR(static_base_uri);
	}

	PHALCON_RETURN_CALL_METHOD(getThis(), "getbaseuri");
	RETURN_MM();
}

/**
 * Sets a base path for all the generated paths
 *
 *<code>
 *	$url->setBasePath('/var/www/htdocs/');
 *</code>
 *
 * @param string $basePath
 * @return Phalcon\Mvc\Url
 */
PHP_METHOD(Phalcon_Mvc_Url, setBasePath){

	zval *base_path;

	phalcon_fetch_params(0, 1, 0, &base_path);

	phalcon_update_property_this(getThis(), SL("_basePath"), base_path);
	RETURN_THISW();
}

/**
 * Returns the base path
 *
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Url, getBasePath){


	RETURN_MEMBER(getThis(), "_basePath");
}

/**
 * Generates a URL
 *
 *<code>
 *
 * //Generate a URL appending the URI to the base URI
 * echo $url->get('products/edit/1');
 *
 * //Generate a URL for a predefined route
 * echo $url->get(array('for' => 'blog-post', 'title' => 'some-cool-stuff', 'year' => '2012'));
 * echo $url->get(array('for' => 'blog-post', 'hostname' => true, 'title' => 'some-cool-stuff', 'year' => '2012'));
 *
 *</code>
 *
 * @param string|array $uri
 * @param array|object args Optional arguments to be appended to the query string
 * @param bool|null $local
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Url, get){

	zval *uri = NULL, *args = NULL, *local = NULL, *base_uri = NULL, *router = NULL, *dependency_injector;
	zval service, route_name, hostname, *route = NULL, exception_message;
	zval *pattern = NULL, *paths = NULL, processed_uri, query_string;
	zval matched, regexp;
	zval *generator = NULL, arguments;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 3, &uri, &args, &local);

	if (!uri) {
		uri = &PHALCON_GLOBAL(z_null);
	}

	if (!args) {
		args = &PHALCON_GLOBAL(z_null);
	}

	if (!local) {
		local = &PHALCON_GLOBAL(z_null);
	} else {
		PHALCON_SEPARATE_PARAM(local);
	}

	PHALCON_CALL_METHOD(&base_uri, getThis(), "getbaseuri");

	if (Z_TYPE_P(uri) == IS_STRING) {
		if (strstr(Z_STRVAL_P(uri), ":")) {
			ZVAL_STRING(&regexp, "/^[^:\\/?#]++:/");
			RETURN_MM_ON_FAILURE(phalcon_preg_match(&matched, &regexp, uri, NULL));
			if (zend_is_true(&matched)) {
				PHALCON_INIT_NVAR(local);
				ZVAL_FALSE(local);
			}
		}

		if (Z_TYPE_P(local) == IS_NULL || zend_is_true(local)) {
			PHALCON_CONCAT_VV(return_value, base_uri, uri);
		} else {
			ZVAL_ZVAL(return_value, uri, 1, 0);
		}
	} else if (Z_TYPE_P(uri) == IS_ARRAY) {
		if (!phalcon_array_isset_fetch_str(&route_name, uri, SL("for"))) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_url_exception_ce, "It's necessary to define the route name with the parameter \"for\"");
			return;
		}

		router = phalcon_read_property(getThis(), SL("_router"), PH_NOISY);

		/**
		 * Check if the router has not previously set
		 */
		if (Z_TYPE_P(router) != IS_OBJECT) {
			dependency_injector = phalcon_read_property(getThis(), SL("_dependencyInjector"), PH_NOISY);
			if (!zend_is_true(dependency_injector)) {
				PHALCON_THROW_EXCEPTION_STR(phalcon_mvc_url_exception_ce, "A dependency injector container is required to obtain the \"url\" service");
				return;
			}

			ZVAL_STRING(&service, ISV(router));

			router = NULL;
			PHALCON_CALL_METHOD(&router, dependency_injector, "getshared", &service);
			PHALCON_VERIFY_INTERFACE(router, phalcon_mvc_routerinterface_ce);
			phalcon_update_property_this(getThis(), SL("_router"), router);
		}

		/**
		 * Every route is uniquely identified by a name
		 */
		PHALCON_CALL_METHOD(&route, router, "getroutebyname", &route_name);
		if (Z_TYPE_P(route) != IS_OBJECT) {
			PHALCON_CONCAT_SVS(&exception_message, "Cannot obtain a route using the name \"", &route_name, "\"");
			PHALCON_THROW_EXCEPTION_ZVAL(phalcon_mvc_url_exception_ce, &exception_message);
			return;
		}

		PHALCON_CALL_METHOD(&pattern, route, "getpattern");

		/**
		 * Return the reversed paths
		 */
		PHALCON_CALL_METHOD(&paths, route, "getreversedpaths");

		/**
		 * Return the Url Generator
		 */
		PHALCON_CALL_METHOD(&generator, route, "geturlgenerator");

		if (phalcon_is_callable(generator) ||
			(Z_TYPE_P(generator) == IS_OBJECT && instanceof_function(Z_OBJCE_P(generator), zend_ce_closure))) {
			array_init_size(&arguments, 3);
			phalcon_array_append(&arguments, base_uri, PH_COPY);
			phalcon_array_append(&arguments, paths, PH_COPY);
			phalcon_array_append(&arguments, uri, PH_COPY);
			PHALCON_CALL_USER_FUNC_ARRAY(&return_value, generator, &arguments);
		} else {
			/**
			 * Replace the patterns by its variables
			 */
			phalcon_replace_paths(&processed_uri, pattern, paths, uri);

			if (phalcon_array_isset_fetch_str(&hostname, uri, SL("hostname")) && zend_is_true(hostname)) {
				PHALCON_CALL_METHOD(&hostname, route, "gethostname");
				PHALCON_CONCAT_VVV(return_value, &hostname, base_uri, &processed_uri);
			} else {
				PHALCON_CONCAT_VV(return_value, base_uri, &processed_uri);
			}
		}
	}

	if (zend_is_true(args)) {
		phalcon_http_build_query(&query_string, args, "&");
		if (Z_TYPE_P(&query_string) == IS_STRING && Z_STRLEN_P(&query_string)) {
			if (phalcon_memnstr_str(return_value, "?", 1)) {
				PHALCON_SCONCAT_SV(return_value, "&", &query_string);
			}
			else {
				PHALCON_SCONCAT_SV(return_value, "?", &query_string);
			}
		}
	}

	RETURN_MM();
}

/**
 * Generates a URL for a static resource
 *
 * @param string|array $uri
 * @param array $args
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Url, getStatic){

	zval *uri = NULL, *args = NULL, *static_base_uri, *base_uri = NULL;
	zval *matched, *pattern, *query_string;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 0, 2, &uri, &args);

	if (!uri) {
		uri = &PHALCON_GLOBAL(z_null);
	} else {
		PHALCON_ENSURE_IS_STRING(uri);

		if (strstr(Z_STRVAL_P(uri), "://")) {
			PHALCON_INIT_VAR(matched);
			PHALCON_INIT_VAR(pattern);
			ZVAL_STRING(pattern, "/^[^:\\/?#]++:/");
			RETURN_MM_ON_FAILURE(phalcon_preg_match(matched, pattern, uri, NULL));
			if (zend_is_true(matched)) {
				RETURN_CTOR(uri);
			}
		}
	}

	static_base_uri = phalcon_read_property(getThis(), SL("_staticBaseUri"), PH_NOISY);
	if (Z_TYPE_P(static_base_uri) != IS_NULL) {
		PHALCON_CONCAT_VV(return_value, static_base_uri, uri);
	} else {	
		PHALCON_CALL_METHOD(&base_uri, getThis(), "getbaseuri");
		PHALCON_CONCAT_VV(return_value, base_uri, uri);
	}

	if (args) {
		PHALCON_INIT_VAR(query_string);
		phalcon_http_build_query(query_string, args, "&");
		if (Z_TYPE_P(query_string) == IS_STRING && Z_STRLEN_P(query_string)) {
			if (phalcon_memnstr_str(return_value, "?", 1)) {
				PHALCON_SCONCAT_SV(return_value, "&", query_string);
			}
			else {
				PHALCON_SCONCAT_SV(return_value, "?", query_string);
			}
		}
	}

	RETURN_MM();
}

/**
 * Generates a local path
 *
 * @param string $path
 * @return string
 */
PHP_METHOD(Phalcon_Mvc_Url, path){

	zval *path = NULL, *base_path;

	phalcon_fetch_params(0, 0, 1, &path);

	if (!path) {
		path = &PHALCON_GLOBAL(z_null);
	}

	base_path = phalcon_read_property(getThis(), SL("_basePath"), PH_NOISY);
	PHALCON_CONCAT_VV(return_value, base_path, path);
}
