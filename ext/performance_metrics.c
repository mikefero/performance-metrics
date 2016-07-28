#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <php.h>
#include <php_ini.h>
#include <ext/standard/info.h>

#if HAVE_SPL
# include <ext/spl/spl_exceptions.h>
# include <Zend/zend_exceptions.h>
#else
# error SPL must be enabled
#endif

#include "php_performance_metrics.h"
#include "utils.h"

/* Enable and initialize the globals */
ZEND_DECLARE_MODULE_GLOBALS(performance_metrics)
static PHP_GINIT_FUNCTION(performance_metrics);

#if ZEND_MODULE_API_NO >= 20050617
/**
 * Dependencies for the extension
 */
static zend_module_dep php_performance_metrics_deps[] = {
  ZEND_MOD_REQUIRED("spl")
  ZEND_MOD_END
};
#endif

/**
 * Global extension functions
 */
const zend_function_entry performance_metrics_functions[] = {
  PHP_FE(persistent_performance_metrics_count,	NULL)
  PHP_FE_END /* Must be the last line in performance_metrics_functions[] */
};

/* {{{ performance_metrics_module_entry
 * Performance metrics initialization entry point
 */
zend_module_entry performance_metrics_module_entry = {
#if ZEND_MODULE_API_NO >= 20050617
  STANDARD_MODULE_HEADER_EX, NULL, php_performance_metrics_deps,
#elif ZEND_MODULE_API_NO >= 20010901
  STANDARD_MODULE_HEADER,
#endif
  PHP_PERFORMANCE_METRICS_NAME,
  performance_metrics_functions,
  PHP_MINIT(performance_metrics),
  NULL,
  NULL,
  NULL,
  PHP_MINFO(performance_metrics),
#if ZEND_MODULE_API_NO >= 20010901
  PHP_PERFORMANCE_METRICS_VERSION,
#endif
  PHP_MODULE_GLOBALS(performance_metrics),
  PHP_GINIT(performance_metrics),
  NULL,
  NULL,
  STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 * Initialize the extension
 */
PHP_MINIT_FUNCTION(performance_metrics) {
  /* Initialize the PerformanceMetrics resources */
  le_performance_metrics_persistent_res = zend_register_list_destructors_ex(
    NULL,
    php_PerformanceMetrics_dtor,
    PHP_PERFORMANCE_METRICS_NAME,
    module_number
   );

  /* Define the PHP PerformanceMetrics class */
  define_PerformanceMetrics(TSRMLS_C);

  return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 * Display information and statistics about the extension
 */
PHP_MINFO_FUNCTION(performance_metrics) {
  /* Get the number of persistent metrics in use */
  char count_str[64];
  php_size_t count = PERFORMANCE_METRICS_G(persistent_performance_metrics);
  snprintf(count_str, sizeof(count_str), "%d", count);

  /* Display the information */
  php_info_print_table_start();
  php_info_print_table_header(2, "Performance Metrics support", "enabled");
  php_info_print_table_header(2, "Persistent performance metrics", count_str);
  php_info_print_table_end();
}
/* }}} */

/* {{{ PHP_GINIT_FUNCTION
 * Initialize the globals
 */
static PHP_GINIT_FUNCTION(performance_metrics) {
  performance_metrics_globals->persistent_performance_metrics = 0;
}
/* }}} */

/* Enable and initialize the extension */
#ifdef COMPILE_DL_PERFORMANCE_METRICS
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(performance_metrics)
#endif

/* {{{ persistent_performance_metrics_count()
 * Return the number of persistent performance metrics
 */
PHP_FUNCTION(persistent_performance_metrics_count) {
  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }
  RETURN_LONG(PERFORMANCE_METRICS_G(persistent_performance_metrics));
}
/* }}} */

/* Argument definitions for the PHP PerformanceMetrics class */
ZEND_BEGIN_ARG_INFO_EX(arginfo_none, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_name, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_latency, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, latency)
ZEND_END_ARG_INFO()

/**
 * PHP PerformanceMetrics class methods
 */
static zend_function_entry php_PerformanceMetrics_methods[] = {
  PHP_ME(PerformanceMetrics, __construct, arginfo_name, ZEND_ACC_PUBLIC)
  PHP_ME(PerformanceMetrics, elapsed_time, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(PerformanceMetrics, metrics, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(PerformanceMetrics, name, arginfo_name, ZEND_ACC_PUBLIC)
  PHP_ME(PerformanceMetrics, observe, arginfo_latency, ZEND_ACC_PUBLIC)
  PHP_ME(PerformanceMetrics, start_timer, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(PerformanceMetrics, stop_timer, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(PerformanceMetrics, tick_rates, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_FE_END
};

/* {{{ define_PerformanceMetrics
 * Define the PHP PerformanceMetrics class
 */
static void define_PerformanceMetrics(TSRMLS_D) {
  zend_class_entry ce;

  /* Define the PHP PerformanceMetrics class and its attributes */
  INIT_CLASS_ENTRY(ce, "PerformanceMetrics", php_PerformanceMetrics_methods);
  php_PerformanceMetrics_ce = zend_register_internal_class(&ce TSRMLS_CC);
  php_PerformanceMetrics_ce->create_object = php_PerformanceMetrics_new;
  memcpy(
    &php_PerformanceMetrics_handlers,
    zend_get_std_object_handlers(),
    sizeof(zend_object_handlers)
  );
}
/* }}} */

/* {{{ php_PerformanceMetrics_dtor(resource)
 * Destroy the persistent memory (Metrics) from the PHP PerformanceMetrics
 * internal structure
 */
static void php_PerformanceMetrics_dtor(php_zend_resource resource TSRMLS_DC) {
  /* Get the PHP PerformanceMetrics internal structure */
  Metrics* metrics = (Metrics*) resource->ptr;

  /* Determine if the Metrics memory can be cleaned up */
  if (metrics) {
    if (metrics->hdr) {
      free(metrics->hdr);
      metrics->hdr = NULL;
    }
    METER_CLEANUP(metrics->m1_rate)
    METER_CLEANUP(metrics->m5_rate)
    METER_CLEANUP(metrics->m15_rate)
    pefree(metrics, 1);
    metrics = NULL;
  }

  /* Decrement the number of persistent metrics and clear the resource */
  PERFORMANCE_METRICS_G(persistent_performance_metrics)--;
  resource->ptr = NULL;
}
/* }}} */

/* {{{ php_PerformanceMetrics_free(object)
 * Free the memory from the PHP PerformanceMetrics internal structure (NOT
 * PERSISTENT MEMORY)
 */
static void php_PerformanceMetrics_free(php_zend_object_free *object TSRMLS_DC) {
  /* Get the PHP PerformanceMetrics internal structure */
  PerformanceMetrics* self = PHP_OBJECT_GET(PerformanceMetrics, object);

  /* Determine if the name memory can be cleaned up */
  if (self->name) {
    efree(self->name);
    self->name = NULL;
    self->name_length = 0;
  }

  /* Determine if the PHP PerformanceMetrics instance should be destroyed */
  zend_object_std_dtor(&self->zval TSRMLS_CC);
#if PHP_VERSION_ID < 70000
  efree(self);
//  self = NULL;
#endif
}
/* }}} */

/* {{{ php_PerformanceMetrics_new(ce)
 * Create the PHP PerformanceMetrics internal structure and initialize the
 * defaults
 */
static php_zend_object php_PerformanceMetrics_new(zend_class_entry *ce TSRMLS_DC) {
  PerformanceMetrics* self = PHP_ECALLOC(PerformanceMetrics, ce);

  self->name = NULL;
  self->name_length = 0;
  self->metrics = NULL;

  PHP_OBJECT_INIT(PerformanceMetrics, self, ce);
}
/* }}} */

/* {{{ __construct(name)
 * Create the PHP PerformanceMetrics instance. The internal Metrics structure
 * will be retrieved from persistent memory (if available) or a new instance
 * will be created and added to persistent memory
 */
PHP_METHOD(PerformanceMetrics, __construct) {
  zval* zname = NULL;
  PerformanceMetrics* self = NULL;
  char* name = NULL;
  php_size_t name_length = 0;
  char* hash_key = NULL;
  php_size_t hash_key_length = 0;
  php_zend_resource_list_entry* le = NULL;
  php_zend_resource_list_entry resource;

  /* Make sure the name of the PHP PerformanceMetrics is valid */
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zname) == FAILURE) {
    RETURN_NULL();
  }
  if (Z_TYPE_P(zname) != IS_STRING || Z_STRLEN_P(zname) == 0) {
    zend_throw_exception_ex(
      spl_ce_InvalidArgumentException,
      0 TSRMLS_CC,
      "Name must be a non-empty string, %Z given", zname
    );
  }

  /* Get the PHP PerformanceMetrics instance */
  self = PHP_PERFORMANCE_METRICS_GET_PERFORMANCE_METRICS(getThis());

  /* Determine if the Metrics are available in persistent memory */
  name = Z_STRVAL_P(zname);
  name_length = Z_STRLEN_P(zname);
  hash_key_length = spprintf(&hash_key, 0, "performance-metrics:%s", name);
  if (PHP_ZEND_HASH_FIND(&EG(persistent_list), hash_key, hash_key_length + 1, le) &&
      Z_RES_P(le)->type == le_performance_metrics_persistent_res) {
    self->metrics = (Metrics*) Z_RES_P(le)->ptr;
  }

  // Add/Update the name of the PHP PerformanceMetrics instance
  self->name = estrndup(name, name_length);
  self->name_length = name_length;

  /* Determine if the Metrics were retrieved from persistent memory */
  if (!self->metrics) {
    /* Initialize the new Metrics internal structure for persistent memory */
    self->metrics = (Metrics*) pecalloc(1, sizeof(Metrics), 1);
    hdr_init(LOWEST_TRACKABLE_VALUE, HIGHEST_TRACKABLE_VALUE, 3, &self->metrics->hdr);
    self->metrics->last_tick = 0;
    self->metrics->start_time = 0;
    METER_STANDARD_ONE_MINUTE_INIT(self->metrics->m1_rate)
    METER_STANDARD_FIVE_MINUTE_INIT(self->metrics->m5_rate)
    METER_STANDARD_FIFTEEN_MINUTE_INIT(self->metrics->m15_rate)


    /* Add the Metrics to persistent memory */
  #if PHP_MAJOR_VERSION >= 7
    ZVAL_NEW_PERSISTENT_RES(&resource, 0, self->metrics, le_performance_metrics_persistent_res);
    PHP_ZEND_HASH_UPDATE(&EG(persistent_list), hash_key, hash_key_length + 1, &resource, sizeof(php_zend_resource_list_entry));
  #else
    resource.type = le_performance_metrics_persistent_res;
    resource.ptr = self->metrics;
    PHP_ZEND_HASH_UPDATE(&EG(persistent_list), hash_key, hash_key_length + 1, resource, sizeof(php_zend_resource_list_entry));
  #endif

    /* Increment the number persistent metrics in memory */
    PERFORMANCE_METRICS_G(persistent_performance_metrics)++;
  }

  /* Clean up the memory allocated to the hash key */
  efree(hash_key);
}
/* }}} */

/* {{{ elapsed_time()
 * Get the elapsed time
 */
PHP_METHOD(PerformanceMetrics, elapsed_time) {
  PerformanceMetrics* self = NULL;
  Metrics* metrics = NULL;
  uint64_t elapsed = 0;

  /* Ensure that no arguments are passed in */
  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  /* Get the metrics from the PHP PerformanceMetrics instance */
  PHP_PERFORMANCE_METRICS_GET_METRICS(self, metrics)

  /* Calculate the elapsed time */
  if (metrics->start_time > 0) {
    elapsed = get_fasttime() - metrics->start_time;
  }

  RETURN_LONG(elapsed);
}
/* }}} */

/* {{{ metrics()
 * Returns an array representing the performance metrics snapshot
 *
 * Keys:
 * +------------+-------------------------------------------------------------+
 * | t          | Timestamp (in seconds)                                      |
 * | count      | Total count observed                                        |
 * | min        | Minimum latency observed (microseconds)                     |
 * | max        | Maximum latency observed (microseconds)                     |
 * | mean       | Mean of the latency observed (microseconds)                 |
 * | stddev     | Standard deviation of the latencies observed (microseconds) |
 * | median/p50 | Median of the latencies observed (microseconds)             |
 * | p75        | Latency at the 75th percentile (microseconds)               |
 * | p95        | Latency at the 95th percentile (microseconds)               |
 * | p98        | Latency at the 98th percentile (microseconds)               |
 * | p99        | Latency at the 99th percentile (microseconds)               |
 * | p999       | Latency at the 999th percentile (microseconds)              |
 * | mean_rate  | Mean rate (per second)                                      |
 * | m1_rate    | One minute rate (per second)                                |
 * | m5_rate    | Five minute rate (per second)                               |
 * | m15_rate   | Fifteen minute rate (per second)                            |
 * +------------+-------------------------------------------------------------+
 */
PHP_METHOD(PerformanceMetrics, metrics) {
  PerformanceMetrics* self = NULL;
  Metrics* metrics = NULL;
  struct hdr_histogram* hdr = NULL;
  int64_t median = 0;

  /* Ensure that no arguments are passed in */
  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  /* Get the metrics and histogram from the PHP PerformanceMetrics instance */
  PHP_PERFORMANCE_METRICS_GET_METRICS_AND_HDR(self, metrics, hdr)

  /* Create the key/value pair array and assign the values */
  median = hdr_value_at_percentile(hdr, 50.0);
  array_init(return_value);
  add_assoc_long(return_value, "t", get_timestamp());
  add_assoc_long(return_value, "count", hdr->total_count);
  add_assoc_long(return_value, "min", hdr_min(hdr));
  add_assoc_long(return_value, "max", hdr_max(hdr));
  add_assoc_long(return_value, "mean", (int64_t) hdr_mean(hdr));
  add_assoc_long(return_value, "stddev", (int64_t) hdr_stddev(hdr));
  add_assoc_long(return_value, "median", median);
  add_assoc_long(return_value, "p50", median);
  add_assoc_long(return_value, "p75", hdr_value_at_percentile(hdr, 75.0));
  add_assoc_long(return_value, "p95", hdr_value_at_percentile(hdr, 95.0));
  add_assoc_long(return_value, "p98", hdr_value_at_percentile(hdr, 98.0));
  add_assoc_long(return_value, "p99", hdr_value_at_percentile(hdr, 99.0));
  add_assoc_long(return_value, "p999", hdr_value_at_percentile(hdr, 99.9));
  add_assoc_double(return_value, "mean_rate", mean_rate(&metrics->m1_rate)); /* NOTE: All metered rates will have the same mean_rate */
  add_assoc_double(return_value, "m1_rate", metrics->m1_rate.rate);
  add_assoc_double(return_value, "m5_rate", metrics->m5_rate.rate);
  add_assoc_double(return_value, "m15_rate", metrics->m15_rate.rate);
}
/* }}} */

/* {{{ name()
 * Get the name of the PHP PerformanceMetrics instance
 */
PHP_METHOD(PerformanceMetrics, name) {
  PerformanceMetrics* self = NULL;

  /* Ensure that no arguments are passed in */
  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  /* Return the name of the PHP PerformanceMetricS instance */
  self = PHP_PERFORMANCE_METRICS_GET_PERFORMANCE_METRICS(getThis());
  PHP_RETURN_STRINGL(self->name, self->name_length);
 }

/* {{{ observe(latency [optional])
 * Observe and record the latency in microseconds (optional: latency)
 */
PHP_METHOD(PerformanceMetrics, observe) {
  zval* zlatency = NULL;
  PerformanceMetrics* self = NULL;
  Metrics* metrics = NULL;
  struct hdr_histogram* hdr = NULL;
  uint64_t latency = 0;

  /* Ensure that no arguments or the latency is passed in */
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &zlatency) == FAILURE) {
    return;
  }
  if (ZEND_NUM_ARGS() == 1) {
    if (Z_TYPE_P(zlatency) != IS_LONG || Z_LVAL_P(zlatency) == 0) {
      zend_throw_exception_ex(
        spl_ce_InvalidArgumentException,
        0 TSRMLS_CC,
        "Latency must be numberic and > 0, %Z given", zlatency
      );
    }
    latency = Z_LVAL_P(zlatency);
  }

  /* Get the metrics and histogram from the PHP PerformanceMetrics instance */
  PHP_PERFORMANCE_METRICS_GET_METRICS_AND_HDR(self, metrics, hdr)

  /* Determine if the latency should be calculated */
  if (ZEND_NUM_ARGS() == 0) {
    if (metrics->start_time == 0) {
      zend_throw_exception_ex(
        spl_ce_InvalidArgumentException,
        0 TSRMLS_CC,
        "Time was not started and latency was not pass in as an argument"
      );
    }

    /* Calculate the latency and reset the timer */
    metrics->last_tick = get_fasttime();
    latency = (metrics->last_tick - metrics->start_time) / NANOSECONDS_TO_MICROSECONDS;
    metrics->start_time = 0;
  }

  /* Observe/Record the latency */
  if (!hdr_record_value(hdr, latency)) {
    zend_throw_exception_ex(
      spl_ce_RuntimeException,
      0 TSRMLS_CC,
      "Elapsed time [%d] is larger than the highest trackable value", latency
    );
  };

  /* Tick the metered rates */
  mark_meter(&metrics->m1_rate);
  mark_meter(&metrics->m5_rate);
  mark_meter(&metrics->m15_rate);

  RETURN_TRUE;
}
/* }}} */

/* {{{ start_timer()
 * Start the timer (also used latency observation when latency not passed in)
 */
PHP_METHOD(PerformanceMetrics, start_timer) {
  PerformanceMetrics* self = NULL;
  Metrics* metrics = NULL;

  /* Ensure that no arguments are passed in */
  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  /* Get the metrics from the PHP PerformanceMetrics instance */
  PHP_PERFORMANCE_METRICS_GET_METRICS(self, metrics)

  /* Ensure the timer has not already been started */
  if (metrics->start_time > 0) {
    zend_throw_exception_ex(
      spl_ce_RuntimeException,
      0 TSRMLS_CC,
      "Timer has already been started"
    );
  }

  /* Assign the starting time for the upcoming observation */
  metrics->start_time = get_fasttime();

  RETURN_TRUE;
}
/* }}} */

/* {{{ stop_timer()
 * Stop and reset the timer; return the elapsed time
 */
PHP_METHOD(PerformanceMetrics, stop_timer) {
  PerformanceMetrics* self = NULL;
  Metrics* metrics = NULL;
  uint64_t elapsed = 0;

  /* Ensure that no arguments are passed in */
  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  /* Get the metrics from the PHP PerformanceMetrics instance */
  PHP_PERFORMANCE_METRICS_GET_METRICS(self, metrics)

  /* Calculate the elapsed time and reset the start timer */
  if (metrics->start_time > 0) {
    elapsed = get_fasttime() - metrics->start_time;
    metrics->start_time = 0;
  }

  RETURN_LONG(elapsed);
}
/* }}} */

/* {{{ tick_rates()
 * Tick the metered rates (if needed)
 */
PHP_METHOD(PerformanceMetrics, tick_rates) {
  PerformanceMetrics* self = NULL;
  Metrics* metrics = NULL;

  /* Ensure that no arguments are passed in */
  if (zend_parse_parameters_none() == FAILURE) {
    return;
  }

  /* Get the metrics from the PHP PerformanceMetrics instance */
  PHP_PERFORMANCE_METRICS_GET_METRICS(self, metrics)

  /* Tick the metered rates */
  tick_meter(&metrics->m1_rate);
  tick_meter(&metrics->m5_rate);
  tick_meter(&metrics->m15_rate);

  RETURN_TRUE;
}
/* }}} */