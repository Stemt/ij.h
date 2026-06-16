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

#define ASSERT_SIZEQ(a,b)do{\
  if(a == b){\
    LOG_SUCCESS(""#a" == "#b);\
  }else{\
    LOG_FAIL("\n  expected "#a" == "#b", got %zu == %zu",(a),(b));\
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
  if(sv_eq(a,b) == false){\
    LOG_FAIL("\n  expected "#a" == "#b", got '"SV_Fmt"' == '"SV_Fmt"'",SV_Arg(a), SV_Arg(b));\
    abort();\
  }else{\
    LOG_SUCCESS(""#a" == "#b);\
  }\
}while(0)

#define ASSERT_STRNEQ(a,b)do{\
  if(sv_eq((a),(b)) == true){\
    LOG_FAIL("\n  expected "#a" != "#b", got '"SV_Fmt"' != '"SV_Fmt"'",SV_Arg(a), SV_Arg(b));\
    abort();\
  }else{\
    LOG_SUCCESS(""#a" != "#b);\
  }\
}while(0)

#define min(a,b) ((a) > (b) ? (b) : (a))
#define max(a,b) ((a) < (b) ? (b) : (a))


void utest_deserialize_element_f64(void){
  char buf[] = "2.0";
  IJ ij = {0};
  ij_init(&ij, buf, sizeof(buf), IJ_DESERIALIZE, 0);
  
  double number = 0.0f;
  ASSERT_TRUE(ij_f64(&ij, &number));
  ASSERT_FLEQ(number, 2.0f);
  ASSERT_TRUE(ij.error == IJ_OK);

  ij_deinit(&ij);
}

void utest_deserialize_element_string(void){
  char buf[] = "\"test\"";
  IJ ij = {0};
  ij_deserialize(&ij, buf, sizeof(buf));

  String_View sv = {0};
  ASSERT_TRUE(ij_string(&ij, &sv.data, &sv.count));
  ASSERT_STREQ(sv, sv_from_cstr("test"));
  ASSERT_TRUE(ij.error == IJ_OK);
  
  ij_deinit(&ij);
}

void utest_deserialize_element_array(void){
  char buf[] = "[1,2,3]";
  IJ ij = {0};
  ij_deserialize(&ij, buf, sizeof(buf));

  size_t count = 0;
  if(ij_array_begin(&ij)) do{
    double number = 0.0f;
    (ij_f64(&ij, &number));
    (number, (double)count+1);
  }while(ij_array_end(&ij, &count) == false);

  ij_deinit(&ij);
  ASSERT_TRUE(ij.error == IJ_OK);
  ASSERT_SIZEQ(count, (size_t)3);

}

void utest_deserialize_element_object(void){
  char buf[] = "{\"id\":1,\"name\":\"Jack\"}";
  IJ ij = {0};
  ij_deserialize(&ij, buf, sizeof(buf));

  size_t id = 0;
  String_View name = {0};
  if(ij_obj_begin(&ij)) do{
    if(ij_obj_member(&ij, "id")) 
      ij_usize(&ij, &id);
    if(ij_obj_member(&ij, "name")) 
      ij_string(&ij, &name.data, &name.count);
  }while(ij_obj_end(&ij) == false);
  
  ij_deinit(&ij);

  ASSERT_TRUE(ij.error == IJ_OK);
  ASSERT_SIZEQ(id, (size_t)1);
  ASSERT_STREQ(name, sv_from_cstr("Jack"));

}

void utest_serialize_element_f64(void){
  char buf[1024];
  IJ ij = {0};
  ij_serialize(&ij, buf, sizeof(buf));
  
  double number = 2.0f;
  ASSERT_TRUE(ij_f64(&ij, &number));
  String_View out = sv_from_parts(ij_buf(&ij), ij_bufsize(&ij));
  ASSERT_STREQ(out, sv_from_cstr("2.000000"));

  ij_deinit(&ij);
}

void utest_serialize_element_string(void){
  char buf[1024];
  IJ ij = {0};
  ij_serialize(&ij, buf, sizeof(buf));

  String_View sv = sv_from_cstr("test");
  ASSERT_TRUE(ij_string(&ij, &sv.data, &sv.count));
  ASSERT_TRUE(ij.error == IJ_OK);
  String_View out = sv_from_parts(ij_buf(&ij), ij_bufsize(&ij));
  ASSERT_STREQ(out, sv_from_cstr("\"test\""));
  
  ij_deinit(&ij);
}

void utest_serialize_element_array(void){
  char buf[1024];
  IJ ij = {0};
  ij_serialize(&ij, buf, sizeof(buf));

  size_t count = 3;
  if(ij_array_begin(&ij)) do{
    size_t number = 4-count;
    ASSERT_TRUE(ij_usize(&ij, &number));
  }while(ij_array_end(&ij, &count) == false);

  ASSERT_TRUE(ij.error == IJ_OK);
  String_View out = sv_from_parts(ij_buf(&ij), ij_bufsize(&ij));
  ASSERT_STREQ(out, sv_from_cstr("[1,2,3]"));

  ij_deinit(&ij);
}

void utest_serialize_element_object(void){
  char buf[1024];
  IJ ij = {0};
  ij_serialize(&ij, buf, sizeof(buf));

  size_t id = 1;
  String_View name = sv_from_cstr("Jack");
  if(ij_obj_begin(&ij)) do{
    if(ij_obj_member(&ij, "id")) ij_usize(&ij, &id);
    if(ij_obj_member(&ij, "name")) ij_string(&ij, &name.data, &name.count);
  }while(ij_obj_end(&ij) == false);
  

  ASSERT_TRUE(ij.error == IJ_OK);
  String_View out = sv_from_parts(ij_buf(&ij), ij_bufsize(&ij));
  ASSERT_STREQ(out, sv_from_cstr("{\"id\":1,\"name\":\"Jack\"}"));

  ij_deinit(&ij);
}

void utest_pretty_serialize_element_object(void){
  char buf[1024];
  IJ ij = {0};
  ij_serialize(&ij, buf, sizeof(buf));
  *ij_flags(&ij) |= IJ_PRETTY;

  size_t id = 1;
  String_View name = sv_from_cstr("Jack");
  if(ij_obj_begin(&ij)) do{
    if(ij_obj_member(&ij, "id")) ij_usize(&ij, &id);
    if(ij_obj_member(&ij, "name")) ij_string(&ij, &name.data, &name.count);
  }while(ij_obj_end(&ij) == false);
  

  ASSERT_TRUE(ij.error == IJ_OK);
  String_View out = sv_from_parts(ij_buf(&ij), ij_bufsize(&ij));
  ASSERT_STREQ(out, sv_from_cstr(
    "{\n"
    "  \"id\":1,\n"
    "  \"name\":\"Jack\"\n"
    "}"
  ));

  ij_deinit(&ij);
}
