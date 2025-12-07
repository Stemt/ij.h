#define IJ_IMPLEMENTATION
#include "ij.h"
#define NOB_IMPLEMENTATION
#include "nob.h"

typedef struct{
  double number;
  const char* string;
  bool condition;
} Data;

bool data_serde(Data* self, IJ* ij){
  if(!ij_obj_begin(ij)) return false;
  do{
    if(ij_member(ij, "number")){
      if(!ij_double(ij, &self->number)) return false;
    }
    if(ij_member(ij, "string")){
      if(!ij_string(ij, &self->string)) return false;
    }
    if(ij_member(ij, "condition")){
      if(!ij_bool(ij,  &self->condition)) return false;
    }
  }while(!ij_obj_end(ij));
  return true;
}

bool data_array_serde(Data* items, int* count, IJ* ij){
  if(!ij_array_begin(ij)) return false;
  do{
    printf("serde element: %d\n", *count);
    if(!data_serde(&items[*count], ij)) return false;
  }while(!ij_array_end(ij, count));
  return true;
}

int main(void){

  char buf[1024] = {0};

  IJ ij = {0};
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);

  Data data = {
    .number = 123,
    .string = "test",
    .condition = true
  };

  Data data_array[] = {
    data,
    data,
  };

  int array_len = NOB_ARRAY_LEN(data_array);
  if(!data_array_serde(data_array, &array_len, &ij)){
    printf("serialize failure: %s\n", buf);
    return 1;
  }
  ij_deinit(&ij);

  printf("serialize success: %s\n", buf);

  ij_init(&ij, .buf=buf, .serialize=false);
  if(!data_array_serde(data_array, &array_len, &ij)){
    printf("deserialize failure: %s\n", buf);
    for(int i = 0; i < (int)sizeof(buf); ++i){
      printf("%02X ", buf[i]);
    }
    printf("\n");
    return 1;
  }
  ij_deinit(&ij);
 
  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);
  if(!data_array_serde(data_array, &array_len, &ij)){
    return 1;
  }
  printf("deserialize success: %s\n", buf);
  ij_deinit(&ij);
  return 0;

  ij_init(&ij, .buf=buf, .buf_len=sizeof(buf), .serialize=true);
  ij_obj_begin(&ij);
  ij_member(&ij, "1"); data_serde(&data_array[0], &ij);
  ij_member(&ij, "2"); data_serde(&data_array[1], &ij);
  ij_member(&ij, "3"); data_serde(&data_array[2], &ij);
  ij_obj_end(&ij);
  ij_deinit(&ij);

  printf("deserialize success: %s\n", buf);

  return 0;
}

