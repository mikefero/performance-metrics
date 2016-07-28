#ifndef __PHP_PERFORMANCE_METRICS_H__
#define __PHP_PERFORMANCE_METRICS_H__

#ifdef PHP_WIN32
#	define PHP_PERFORMANCE_METRICS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_PERFORMANCE_METRICS_API __attribute__ ((visibility("default")))
#else
#	define PHP_PERFORMANCE_METRICS_API
#endif

#ifdef ZTS
# include <TSRM.h>
#endif

#if PHP_VERSION_ID < 50500
#  error PHP 5.5.0 or later is required
#endif

#if HAVE_SPL
#  include <ext/spl/spl_iterators.h>
#  include <ext/spl/spl_exceptions.h>
#else
#  error SPL must be enabled in order to build the driver
#endif

#define PHP_PERFORMANCE_METRICS_NAME "Performance Metrics"
#define PHP_PERFORMANCE_METRICS_VERSION "0.0.1"

extern zend_module_entry performance_metrics_module_entry;
#define phpext_performance_metrics_ptr &performance_metrics_module_entry

#include "php_5to7_macros.h"
#include "hdr_histogram.h"
#include "ewma.h"

/* Macro definitions to create an object for the module */
#if PHP_VERSION_ID >= 70000
# define PHP_PERFORMANCE_METRICS_BEGIN_OBJECT(type_name) typedef struct _php_performance_metrics_##type_name##_ {
# define PHP_PERFORMANCE_METRICS_END_OBJECT(type_name) zend_object zval; \
} type_name; \
\
static inline type_name* php_##type_name##_object_fetch(zend_object *obj) { \
    return (type_name*) ((char*) obj - XtOffsetOf(type_name, zval)); \
}
#else
# define PHP_PERFORMANCE_METRICS_BEGIN_OBJECT(type_name) typedef struct _php_performance_metrics_##type_name##_ { \
  zend_object zval;
# define PHP_PERFORMANCE_METRICS_END_OBJECT(type_name) } type_name;
#endif

/* Marco definition to simplify getting the metrics data */
#define PHP_PERFORMANCE_METRICS_GET_METRICS(self, metrics) \
  self = PHP_PERFORMANCE_METRICS_GET_PERFORMANCE_METRICS(getThis()); \
  if (!self->metrics) { \
    zend_throw_exception_ex( \
      spl_ce_RuntimeException, \
      0 TSRMLS_CC, \
      "Metrics data is not valid (NULL)" \
    ); \
  } \
  metrics = self->metrics;
#define PHP_PERFORMANCE_METRICS_GET_METRICS_AND_HDR(self, metrics, hdr) \
  PHP_PERFORMANCE_METRICS_GET_METRICS(self, metrics) \
  if (!metrics->hdr) { \
    zend_throw_exception_ex( \
      spl_ce_RuntimeException, \
      0 TSRMLS_CC, \
      "HDR histogram data is not valid (NULL)" \
    ); \
  } \
  hdr = metrics->hdr;

/* PHP base extension/module functions */
PHP_MINIT_FUNCTION(performance_metrics);
PHP_MINFO_FUNCTION(performance_metrics);

/* Global extension functions */
PHP_FUNCTION(persistent_performance_metrics_count);

/* Globals */
ZEND_BEGIN_MODULE_GLOBALS(performance_metrics)
  php_size_t persistent_performance_metrics;
ZEND_END_MODULE_GLOBALS(performance_metrics)

#if PHP_VERSION_ID >= 70000
# define PERFORMANCE_METRICS_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(performance_metrics, v)
#else
# ifdef ZTS
#   define PERFORMANCE_METRICS_G(v) TSRMG(performance_metrics_globals_id, zend_performance_metrics_globals*, v)
# else
#   define PERFORMANCE_METRICS_G(v) (performance_metrics_globals.v)
# endif
#endif

#if defined(ZTS) && defined(COMPILE_DL_PERFORMANCE_METRICS)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

/**
 * Internal structure for gathering latencies and metered rate metrics
 */
typedef struct _performance_metrics_Metrics {
  /**
   * HDR histogram instance
   */
  struct hdr_histogram* hdr;
  /**
   * Time the metrics were last "ticked"
   */
  uint64_t last_tick;
  /**
   * Time the metrics were started
   */
  uint64_t start_time;
  /**
   * 1 minute rate (per second)
   */
  Meter m1_rate;
  /**
   * 5 minute rate (per second)
   */
  Meter m5_rate;
  /**
   * 15 minute rate (per second)
   */
  Meter m15_rate;
} Metrics;

/**
 * Persistent metrics resource
 */
static int le_performance_metrics_persistent_res;

/* PHP PerformanceMetrics internal structure */
PHP_PERFORMANCE_METRICS_BEGIN_OBJECT(PerformanceMetrics)
  char* name;
  php_size_t name_length;
  Metrics* metrics;
PHP_PERFORMANCE_METRICS_END_OBJECT(PerformanceMetrics)
#if PHP_VERSION_ID >= 70000
# define PHP_PERFORMANCE_METRICS_GET_PERFORMANCE_METRICS(obj) php_PerformanceMetrics_object_fetch(Z_OBJ_P(obj))
#else
# define PHP_PERFORMANCE_METRICS_GET_PERFORMANCE_METRICS(obj) (PerformanceMetrics*) zend_object_store_get_object((obj) TSRMLS_CC)
#endif

/**
 * PHP PerformanceMetrics class definition
 */
static zend_class_entry *php_PerformanceMetrics_ce;
/**
 * PHP PerformanceMetrics class handlers
 */
static zend_object_handlers php_PerformanceMetrics_handlers;

/* PHP PerformanceMetrics helper functions */
static void define_PerformanceMetrics(TSRMLS_D);
static void php_PerformanceMetrics_dtor(php_zend_resource resource TSRMLS_DC);
static void php_PerformanceMetrics_free(php_zend_object_free *object TSRMLS_DC);
static php_zend_object php_PerformanceMetrics_new(zend_class_entry *ce TSRMLS_DC);

/* PerformanceMetrics class methods */
PHP_METHOD(PerformanceMetrics, __construct);
PHP_METHOD(PerformanceMetrics, elapsed_time);
PHP_METHOD(PerformanceMetrics, metrics);
PHP_METHOD(PerformanceMetrics, name);
PHP_METHOD(PerformanceMetrics, observe);
PHP_METHOD(PerformanceMetrics, start_timer);
PHP_METHOD(PerformanceMetrics, stop_timer);
PHP_METHOD(PerformanceMetrics, tick_rates);

#endif /* PHP_PERFORMANCE_METRICS_H */
