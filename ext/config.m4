PHP_ARG_ENABLE(performance-metrics,
  if performance metrics extension should be enabled,
  [  --enable-performance-metrics
                          Enable the Performance Metrics Extension]
)

AC_MSG_CHECKING([for supported PHP version])
PHP_PERFORMANCE_METRICS_FOUND_VERSION=`${PHP_CONFIG} --version`
PHP_PERFORMANCE_METRICS_VERSION_NUMBER=`echo \
  "${PHP_PERFORMANCE_METRICS_FOUND_VERSION}" | \
  ${AWK} 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 100 + [$]2) * 100 + [$]3;}'`
if test "${PHP_PERFORMANCE_METRICS_VERSION_NUMBER}" -lt "50500"; then
  AC_MSG_ERROR([not supported. PHP version 5.5.0+ required (found ${PHP_PERFORMANCE_METRICS_FOUND_VERSION})])
else
  AC_MSG_RESULT([supported (${PHP_PERFORMANCE_METRICS_FOUND_VERSION})])
fi

if test "${PHP_PERFORMANCE_METRICS}" != "no"; then
  dnl Add SPL as a dependency for exceptions
  ifdef(
    [PHP_ADD_EXTENSION_DEP],
    [PHP_ADD_EXTENSION_DEP(performance-metrics, spl)]
  )

  HDR_HISTOGRAM_C_CLASSES=" \
    vendor/HdrHistogram_c/src/hdr_encoding.c \
    vendor/HdrHistogram_c/src/hdr_histogram.c \
    vendor/HdrHistogram_c/src/hdr_histogram_log.c \
    vendor/HdrHistogram_c/src/hdr_interval_recorder.c \
    vendor/HdrHistogram_c/src/hdr_thread.c \
    vendor/HdrHistogram_c/src/hdr_time.c \
    vendor/HdrHistogram_c/src/hdr_writer_reader_phaser.c \
  ";

  AC_DEFINE(HAVE_PERFORMANCE_METRICS, 1, [Performance Metrics])
  PHP_NEW_EXTENSION(performance_metrics,
    performance_metrics.c \
    ${HDR_HISTOGRAM_C_CLASSES},
    ${ext_shared},
    ,
    []
  )
  PHP_SUBST(PERFORMANCE_METRICS_SHARED_LIBADD)
  PHP_ADD_BUILD_DIR(${ext_builddir}/vendor/HdrHistogram_c/src)
  PHP_ADD_INCLUDE(${ext_srcdir}/vendor/HdrHistogram_c/src)
fi
