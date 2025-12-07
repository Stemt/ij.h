#include <string.h>
#define NOB_IMPLEMENTATION
#include "nob.h"

#if 0
#define IJ_LOG_INFO(...)
#define IJ_LOG_ERROR(...)
#endif
#define IJ_IMPLEMENTATION
#include "ij.h"

#include <math.h>


#define COLOR_RESET "\e[0m"
#define RED "\e[0;31m"
#define GREEN "\e[0;32m"
#define LOG_SUCCESS(fmt, ...) fprintf(stderr, "["GREEN"SUCCESS"COLOR_RESET"] "__FILE__":%d %s: "fmt"\n", __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__)
#define LOG_FAIL(fmt, ...) fprintf(stderr, "["RED"FAIL"COLOR_RESET"] "__FILE__":%d %s: "fmt"\n", __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__)

#define ASSERT_FALSE(a)do{\
  bool cond = (a);\
  if(cond == false){\
    LOG_SUCCESS("("#a") == false");\
  }else{\
    LOG_FAIL("\n  expected ("#a") == false, got true");\
  }\
}while(0)

#define ASSERT_TRUE(a)do{\
  bool cond = (a);\
  if(cond){\
    LOG_SUCCESS("("#a") == true");\
  }else{\
    LOG_FAIL("\n  expected ("#a") == true, got false");\
    abort();\
  }\
}while(0)

#define ASSERT_FLEQ(a,b)do{\
  if(fabs((a)-(b)) < 0.000001){\
    LOG_SUCCESS(""#a" == "#b);\
  }else{\
    LOG_FAIL("\n  expected "#a" == "#b", got %f == %f",(a),(b));\
    abort();\
  }\
}while(0)

#define ASSERT_STREQ(a,b)do{\
  if(strcmp((a),(b)) != 0){\
    LOG_FAIL("\n  expected "#a" == "#b", got '%s' == '%s'",(a), (b));\
    abort();\
  }else{\
    LOG_SUCCESS(""#a" == "#b);\
  }\
}while(0)

#define ASSERT_STRNEQ(a,b)do{\
  if(strcmp((a),(b)) == 0){\
    LOG_FAIL("\n  expected "#a" != "#b", got '%s' != '%s'",(a), (b));\
    abort();\
  }else{\
    LOG_SUCCESS(""#a" != "#b);\
  }\
}while(0)

#define min(a,b) ((a) > (b) ? (b) : (a))
#define max(a,b) ((a) < (b) ? (b) : (a))

int test_read(void* ctx, char* buf, int len){
  char** str = ctx;
  int cpy_len = min(len, (int)strlen(*str));
  if(cpy_len != 0){
    strncpy(buf, *str, cpy_len);
    *str += cpy_len;
    return cpy_len;
  }else{
    *buf = '\0'; // end of input
    return 1;
  }
}

int test_write(void* ctx, char* buf, int len){
  char* str = ctx;
  fprintf(stderr, "buf: '%s'", str);
  fprintf(stderr, "write: '%.*s'", len, buf);
  // quick and dirty append to the end
  strncpy(str+strlen(str), buf, len);
  return len;
}

IJ_Stream test_stream(char* str){
  return (IJ_Stream){
    .ctx = str,
    .read = test_read,
    .write = test_write
  };
}

void utest_serialize_null(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  ij_null(&ij);

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "null");
}

void utest_serialize_bool_true(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  bool b = true;
  ij_bool(&ij, &b);
  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "true");
}

void utest_serialize_bool_false(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  bool b = false;
  ij_bool(&ij, &b);
  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "false");
}

void utest_serialize_double(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  double value = 123.1;
  ij_number(&ij, &value);
  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "123.100000");
}

void utest_serialize_string(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  const char* str = "test";
  ij_string(&ij, &str);
  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "\"test\"");
}

void utest_serialize_any_string(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  IJ_Any value = {
    .type = IJ_STRING,
    .as.String = "test"
  };

  ASSERT_TRUE(ij_any(&ij, &value));

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "\"test\"");
}

void utest_serialize_any_number(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  IJ_Any value = {
    .type = IJ_NUMBER,
    .as.Number = 1.0f
  };

  ASSERT_TRUE(ij_any(&ij, &value));

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "1.000000");
}

void utest_serialize_any_bool_true(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  IJ_Any value = {
    .type = IJ_BOOL,
    .as.Bool = true
  };

  ASSERT_TRUE(ij_any(&ij, &value));

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "true");
}

void utest_serialize_any_bool_false(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  IJ_Any value = {
    .type = IJ_BOOL,
    .as.Bool = false
  };

  ASSERT_TRUE(ij_any(&ij, &value));

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "false");
}

void utest_serialize_any_null(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  IJ_Any value = {
    .type = IJ_NULL,
  };

  ASSERT_TRUE(ij_any(&ij, &value));

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "null");
}

void utest_serialize_any_array_begin(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  IJ_Any value = {
    .type = IJ_ARRAY_BEGIN,
  };

  ASSERT_TRUE(ij_any(&ij, &value));

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "[");
}

void utest_serialize_any_array_end(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  IJ_Any value = {
    .type = IJ_ARRAY_END,
  };

  ASSERT_TRUE(ij_any(&ij, &value));

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "]");
}

void utest_serialize_any_obj_begin(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  IJ_Any value = {
    .type = IJ_OBJ_BEGIN,
  };

  ASSERT_TRUE(ij_any(&ij, &value));

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "{");
}

void utest_serialize_stream(void){
  char stream_buf[1024] = {0};
  char buf[5] = {0};

  IJ ij = {0};
  ij_init(&ij, 
      .buf=buf, .buf_len=sizeof(buf), 
      .stream = test_stream(stream_buf),
      .serialize=true);

  ASSERT_TRUE(ij_obj_begin(&ij)); 
    ASSERT_TRUE(ij_write_string(&ij, "str1")); 
    ASSERT_TRUE(ij_write_string(&ij, "str2")); 
    ASSERT_TRUE(ij_write_string(&ij, "str3")); 
  ASSERT_TRUE(ij_obj_end(&ij)); 

  ij_deinit(&ij);

  ASSERT_STREQ(stream_buf, "{\"str1\",\"str2\",\"str3\"}");
}

void utest_serialize_any_obj_end(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  IJ_Any value = {
    .type = IJ_OBJ_END,
  };

  ASSERT_TRUE(ij_any(&ij, &value));

  ij_deinit(&ij);
  
  ASSERT_STREQ(buf, "}");
}

void utest_serialize_array_empty(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  ij_array_begin(&ij);
  ij_array_end(&ij, NULL);

  ij_deinit(&ij);
  ASSERT_STREQ(buf, "[]");
}

void utest_serialize_array_static(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  ij_array_begin(&ij);
  ij_number(&ij, ((double[]){1.0}));
  ij_number(&ij, ((double[]){2.0}));
  ij_array_end(&ij, NULL);

  ij_deinit(&ij);
  ASSERT_STREQ(buf, "[1.000000,2.000000]");
}

void utest_serialize_array_dynamic(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  struct{
    double* items;
    int count;
    int capacity;
  } da = {0};

  nob_da_append(&da, 1);
  nob_da_append(&da, 2);

  ij_array_begin(&ij);
  do{
    ij_number(&ij, &da.items[da.count-1]);
  }while(ij_array_end(&ij, &da.count) == false);
  ij_deinit(&ij);
  ASSERT_STREQ(buf, "[2.000000,1.000000]");
  nob_da_free(da);
}

void utest_serialize_array_nested(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  ASSERT_TRUE(ij_array_begin(&ij));
    ASSERT_TRUE(ij_array_begin(&ij));
      ASSERT_TRUE(ij_array_begin(&ij));
      ASSERT_TRUE(ij_array_end(&ij, NULL));
    ASSERT_TRUE(ij_array_end(&ij, NULL));
    ASSERT_TRUE(ij_array_begin(&ij));
      ASSERT_TRUE(ij_array_begin(&ij));
      ASSERT_TRUE(ij_array_end(&ij, NULL));
      ASSERT_TRUE(ij_array_begin(&ij));
      ASSERT_TRUE(ij_array_end(&ij, NULL));
    ASSERT_TRUE(ij_array_end(&ij, NULL));
  ASSERT_TRUE(ij_array_end(&ij, NULL));
  ij_deinit(&ij);

  ASSERT_STREQ(buf, "[[[]],[[],[]]]");
}

void utest_serialize_obj_empty(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  ij_obj_begin(&ij);
  ij_obj_end(&ij);

  ij_deinit(&ij);

  ASSERT_STREQ(buf, "{}");
}

void utest_serialize_obj_static(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  ij_obj_begin(&ij);
  ij_member(&ij, "A"); ij_number(&ij, ((double[]){1.0}));
  ij_member(&ij, "B"); ij_number(&ij, ((double[]){2.0}));
  ij_obj_end(&ij);

  ij_deinit(&ij);

  ASSERT_STREQ(buf, "{\"A\":1.000000,\"B\":2.000000}");
}

void utest_serialize_obj_dynamic(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);
  
  double v1 = 1.0f;
  double v2 = 2.0f;
  
  ASSERT_TRUE(ij_obj_begin(&ij));
  do{
    if(ij_member(&ij, "1")){
      ASSERT_TRUE(ij_number(&ij, &v1));
    }
    if(ij_member(&ij, "2")){
      ASSERT_TRUE(ij_number(&ij, &v2));
    }
  } while(!ij_obj_end(&ij));

  ASSERT_STREQ(buf, "{\"1\":1.000000,\"2\":2.000000}");

  ij_deinit(&ij);
}

void utest_serialize_obj_nested(void){
  char buf[1024] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);
  
  ASSERT_TRUE(ij_obj_begin(&ij));
    ASSERT_TRUE(ij_obj_begin(&ij));
      ASSERT_TRUE(ij_obj_begin(&ij));
        ASSERT_TRUE(ij_obj_begin(&ij));
        ASSERT_TRUE(ij_obj_end(&ij));
      ASSERT_TRUE(ij_obj_end(&ij));
    ASSERT_TRUE(ij_obj_end(&ij));
    ASSERT_TRUE(ij_obj_begin(&ij));
      ASSERT_TRUE(ij_obj_begin(&ij));
      ASSERT_TRUE(ij_obj_end(&ij));
      ASSERT_TRUE(ij_obj_begin(&ij));
      ASSERT_TRUE(ij_obj_end(&ij));
    ASSERT_TRUE(ij_obj_end(&ij));
  ASSERT_TRUE(ij_obj_end(&ij));

  ASSERT_STREQ(buf, "{{{{}}},{{},{}}}");

  ij_deinit(&ij);
}

void utest_serialize_buffer_full_null(void){
  char buf[2] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);
  ASSERT_FALSE(ij_null(&ij));
  ASSERT_TRUE(ij_error(&ij) == IJ_E_BUF_FULL);
  ij_deinit(&ij);
}

void utest_serialize_buffer_full_true(void){
  char buf[2] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);
  bool value = true;
  ASSERT_FALSE(ij_bool(&ij, &value));
  ASSERT_TRUE(ij_error(&ij) == IJ_E_BUF_FULL);
  ij_deinit(&ij);
}

void utest_serialize_buffer_full_string(void){
  char buf[2] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);
  const char* value = "test";
  ASSERT_FALSE(ij_string(&ij, &value));
  ASSERT_TRUE(ij_error(&ij) == IJ_E_BUF_FULL);
  ij_deinit(&ij);
}

void utest_serialize_buffer_write(void){
  char tmp[6] = {0};
  char buf[256] = {0};
  IJ ij = {0};
  ij_init(&ij, 
      .buf=tmp, .buf_len=sizeof(tmp), 
      .stream = {
        .write=test_write,
        .ctx=buf
      },
      .serialize=true);

  ASSERT_TRUE(ij_array_begin(&ij));
  ASSERT_TRUE(ij_write_string(&ij, "test1"));
  ASSERT_TRUE(ij_write_string(&ij, "test2"));
  ASSERT_TRUE(ij_write_string(&ij, "test3"));
  ASSERT_TRUE(ij_array_end(&ij, NULL));

  ij_deinit(&ij);

  ASSERT_STREQ(buf, "[\"test1\",\"test2\",\"test3\"]");
}

void utest_deserialize_null(void){
  char buf[1024] = "null";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);

  ASSERT_TRUE(ij_null(&ij));

  ij_deinit(&ij);
}

void utest_deserialize_true(void){
  char buf[1024] = "true";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);

  bool result = false;
  ASSERT_TRUE(ij_bool(&ij, &result));
  IJ_LOG_INFO("result: %d", result);
  ASSERT_TRUE(result);

  ij_deinit(&ij);
}

void utest_deserialize_false(void){
  char buf[1024] = "false";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);

  bool result = true;
  ASSERT_TRUE(ij_bool(&ij, &result));
  ASSERT_FALSE(result);

  ij_deinit(&ij);
}

void utest_deserialize_double(void){
  char buf[1024] = "123.0";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);

  double value = 0.0f;
  ASSERT_TRUE(ij_number(&ij, &value));
  ASSERT_FLEQ(value, 123.0f);

  ij_deinit(&ij);
}

void utest_deserialize_string(void){
  char buf[1024] = "\"test\"";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);
  
  const char* str = NULL;
  ASSERT_TRUE(ij_string(&ij, &str));
  ASSERT_STREQ(str, "test");

  ij_deinit(&ij);
}

void utest_deserialize_array_empty(void){
  char buf[1024] = "[]";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);
  
  ASSERT_TRUE(ij_array_begin(&ij));
  ASSERT_TRUE(ij_array_end(&ij, NULL));

  ij_deinit(&ij);
}

void utest_deserialize_obj_empty(void){
  char buf[1024] = "{}";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);
  
  ASSERT_TRUE(ij_obj_begin(&ij));
  ASSERT_TRUE(ij_obj_end(&ij));

  ij_deinit(&ij);
}

void utest_deserialize_obj_static(void){
  char buf[1024] = "{\"1\":1,\"2\":2}";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);
  
  double value = 0.0f;
  
  ASSERT_TRUE(ij_obj_begin(&ij));

  ASSERT_TRUE(ij_member(&ij, "1"));
  ASSERT_TRUE(ij_number(&ij, &value));
  ASSERT_FLEQ(value, 1.0f);
  
  ASSERT_TRUE(ij_member(&ij, "2"));
  ASSERT_TRUE(ij_number(&ij, &value));
  ASSERT_FLEQ(value, 2.0f);

  ASSERT_TRUE(ij_obj_end(&ij));

  ij_deinit(&ij);
}

void utest_deserialize_obj_dynamic(void){
  char buf[1024] = "{\"1\":1,\"2\":2}";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);
  
  double v1 = 0.0f;
  double v2 = 0.0f;
  
  ASSERT_TRUE(ij_obj_begin(&ij));
  do{
    if(ij_member(&ij, "1")){
      ASSERT_TRUE(ij_number(&ij, &v1));
    }
    if(ij_member(&ij, "2")){
      ASSERT_TRUE(ij_number(&ij, &v2));
    }
  } while(!ij_obj_end(&ij));

  ASSERT_FLEQ(v1, 1.0f);
  ASSERT_FLEQ(v2, 2.0f);

  ij_deinit(&ij);
}

void utest_deserialize_obj_consume_unhandled_members(void){
  char buf[1024] = "{\"1\":1,\"2\":2,\"3\":3}";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);
  
  double v2 = 0.0f;
  int count = 0;
  
  ASSERT_TRUE(ij_obj_begin(&ij));
  do{
    if(ij_member(&ij, "2")){
      ASSERT_TRUE(ij_number(&ij, &v2));
    }
    count++;
  } while(!ij_obj_end(&ij));

  ASSERT_FLEQ(v2, 2.0f);

  ij_deinit(&ij);
}

void utest_deserialize_number_unexpected_end_of_input(void){
  char buf[1024] = "1.";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=strlen(buf), .serialize=false);
  
  double value = 0.0f;
  ASSERT_FALSE(ij_number(&ij, &value));

  ASSERT_TRUE(ij_error(&ij) == IJ_E_END_OF_INPUT);

  ij_deinit(&ij);
}

void utest_deserialize_string_unexpected_end_of_input(void){
  char buf[1024] = "\"test";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);
  
  ASSERT_FALSE(ij_string(&ij,NULL));

  ASSERT_TRUE(ij_error(&ij) == IJ_E_END_OF_INPUT);

  ij_deinit(&ij);
}

void utest_deserialize_array_unexpected_end_of_input(void){
  char buf[1024] = "[";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);
  
  ASSERT_TRUE(ij_array_begin(&ij));
  ASSERT_TRUE(ij_array_end(&ij, NULL));

  ASSERT_TRUE(ij_error(&ij) == IJ_E_END_OF_INPUT);

  ij_deinit(&ij);
}

void utest_deserialize_obj_unexpected_end_of_input(void){
  char buf[1024] = "{";
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .serialize=false);
  
  ASSERT_TRUE(ij_obj_begin(&ij));
  ASSERT_TRUE(ij_obj_end(&ij));

  ASSERT_TRUE(ij_error(&ij) == IJ_E_END_OF_INPUT);

  ij_deinit(&ij);
}

void utest_deserialize_string_stream_lifetime_error(void){
  char in[] = "[\"str1\",\"str2\",\"str3\"]";
  char* in_p = in;
  char buf[8];
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), 
      .stream = {
        .ctx = &in_p,
        .read = test_read,
      },
      .serialize=false);
 
  const char* str1 = NULL;
  const char* str2 = NULL;
  const char* str3 = NULL;
  ASSERT_TRUE(ij_array_begin(&ij));
  ASSERT_TRUE(ij_string(&ij,&str1));
  ASSERT_STREQ(str1, "str1");
  ASSERT_TRUE(ij_string(&ij,&str2));
  ASSERT_STREQ(str2, "str2");
  ASSERT_TRUE(ij_string(&ij,&str3));
  ASSERT_STREQ(str3, "str3");
  ASSERT_TRUE(ij_array_end(&ij,NULL));

  ij_deinit(&ij);

  ASSERT_STRNEQ(str1, "str1"); // overwritten on read
  ASSERT_STRNEQ(str2, "str2"); // overwritten on read
  ASSERT_STREQ(str3, "str3");
}

void utest_deserialize_string_stream_lifetime_fixed(void){
  char in[] = "[\"str1\",\"str2\",\"str3\"]";
  char* in_p = in;
  char buf[8] = {0};
  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), 
      .stream = {
        .ctx = &in_p,
        .read = test_read,
      },
      .serialize=false);
 
  const char* str1 = NULL;
  const char* str2 = NULL;
  const char* str3 = NULL;

  ASSERT_TRUE(ij_array_begin(&ij));
  ASSERT_TRUE(ij_string(&ij,&str1));
  ASSERT_STREQ(str1, "str1");
  str1 = strdup(str1);
  ASSERT_TRUE(ij_string(&ij,&str2));
  ASSERT_STREQ(str2, "str2");
  str2 = strdup(str2);
  ASSERT_TRUE(ij_string(&ij,&str3));
  ASSERT_STREQ(str3, "str3");
  str3 = strdup(str3);
  ASSERT_TRUE(ij_array_end(&ij,NULL));

  ij_deinit(&ij);

  ASSERT_STREQ(str1, "str1");
  ASSERT_STREQ(str2, "str2");
  ASSERT_STREQ(str3, "str3");
}

