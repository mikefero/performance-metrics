#ifndef __PHP_5TO7_MACROS_H__
#define __PHP_5TO7_MACROS_H__

/* Macro definitions for PHP v5.5, 5.6, and 7.0 support */
#if PHP_VERSION_ID >= 70000
  typedef zval php_zend_resource_list_entry;
  typedef zend_resource* php_zend_resource;
  typedef zend_object *php_zend_object;
  typedef zend_object php_zend_object_free;
  typedef size_t php_size_t;

# define PHP_OBJECT_GET(type_name, object) php_##type_name##_object_fetch(object);
# define PHP_ECALLOC(type_name, ce) (type_name*) ecalloc(1, sizeof(type_name) + zend_object_properties_size(ce))
# define PHP_OBJECT_INIT(type_name, self, ce) PHP_OBJECT_INIT_EX(type_name, self, ce)
#   define PHP_OBJECT_INIT_EX(type_name, self, ce) do { \
      zend_object_std_init(&self->zval, ce TSRMLS_CC); \
      ((zend_object_handlers*) &php_##type_name##_handlers)->offset = XtOffsetOf(type_name, zval); \
      ((zend_object_handlers*) &php_##type_name##_handlers)->free_obj = php_##type_name##_free; \
      self->zval.handlers = (zend_object_handlers*) &php_##type_name##_handlers; return &self->zval; \
    } while(0)
# define PHP_ZEND_HASH_FIND(ht, key, len, res) ((res = zend_hash_str_find(ht, key, (size_t) (len - 1))) != NULL)
# define PHP_ZEND_HASH_UPDATE(ht, key, len, val, val_size) ((zend_hash_str_update(ht, key, (size_t) (len - 1), val)) != NULL)
# define PHP_ZEND_HASH_ADD(ht, key, len, val, val_size) (zend_hash_str_add(ht, key, (size_t) (len - 1), val) != NULL)
# define PHP_RETURN_STRINGL(s, len) RETURN_STRINGL(s, len)
#else
  typedef zend_rsrc_list_entry php_zend_resource_list_entry;
  typedef zend_rsrc_list_entry* php_zend_resource;
  typedef zend_object_value php_zend_object;
  typedef void php_zend_object_free;
  typedef int php_size_t;

# define PHP_OBJECT_GET(type_name, object) (type_name*) object
# define Z_RES_P(zv) (zv)
# define PHP_ECALLOC(type_name, ce) (type_name*) ecalloc(1, sizeof(type_name))
# define PHP_OBJECT_INIT(type_name, self, ce) PHP_OBJECT_INIT_EX(type_name, self, ce)
#   define PHP_OBJECT_INIT_EX(type_name, self, ce) do { \
      zend_object_value retval; \
      zend_object_std_init(&self->zval, ce TSRMLS_CC); \
      object_properties_init(&self->zval, ce); \
      retval.handle = zend_objects_store_put(self, \
      (zend_objects_store_dtor_t) zend_objects_destroy_object, \
      php_##type_name##_free, NULL TSRMLS_CC); \
      retval.handlers = (zend_object_handlers*) &php_##type_name##_handlers; \
      return retval; \
    } while(0)
# define PHP_ZEND_HASH_FIND(ht, key, len, res) (zend_hash_find(ht, key, (uint) len, (void**) &res) == SUCCESS)
# define PHP_ZEND_HASH_UPDATE(ht, key, len, val, val_size) (zend_hash_update(ht, key, (uint) len, (void*) &val, (uint) val_size, NULL) == SUCCESS)
# define PHP_ZEND_HASH_ADD(ht, key, len, val, val_size) (zend_hash_add(ht, key, len, (void*) &val, (uint) val_size, NULL) == SUCCESS)
# define PHP_RETURN_STRINGL(s, len) RETURN_STRINGL(s, len, 1)
#endif

#endif /* __PHP_5TO7_MACROS_H__ */
