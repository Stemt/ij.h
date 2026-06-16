#ifndef IJ_H_
#define IJ_H_

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define IJ_LOG_INFO(fmt, ...) fprintf(stderr, "[IJ_INFO]: "fmt"\n" __VA_OPT__(,)  __VA_ARGS__);
#define IJ_LOG_ERROR(fmt, ...) fprintf(stderr, "[IJ_ERROR]: "fmt"\n" __VA_OPT__(,)  __VA_ARGS__);

#define IJ_REALLOC realloc
#define IJ_FREE free 
#define IJ_ASSERT assert 
#ifdef __cplusplus
#define IJ_DECLTYPE_CAST(T) (decltype(T))
#else
#define IJ_DECLTYPE_CAST(T)
#endif // __cplusplus

#define IJ_DA_INIT_CAP 256

#define IJ_MAX_LEXPECTS 16

#define IJ_INDENT_INCREMENT 2

typedef struct{
  char* data;
  size_t count;
} IJ_StringView;

IJ_StringView ij_sv_from_parts(char* data, size_t count){
  return (IJ_StringView){ .data=data, .count=count };
}

// "borrowed" from nob.h
typedef struct{
  char* items;
  size_t count;
  size_t capacity;
} IJ_StringBuilder;

#define ij_da_reserve(da, expected_capacity)                                              \
    do {                                                                                   \
        if ((expected_capacity) > (da)->capacity) {                                        \
            if ((da)->capacity == 0) {                                                     \
                (da)->capacity = IJ_DA_INIT_CAP;                                          \
            }                                                                              \
            while ((expected_capacity) > (da)->capacity) {                                 \
                (da)->capacity *= 2;                                                       \
            }                                                                              \
            (da)->items = IJ_DECLTYPE_CAST((da)->items)IJ_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            IJ_ASSERT((da)->items != NULL && "Buy more RAM lol");                         \
        }                                                                                  \
    } while (0)

#define IJ_ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

// The Fixed Array append. `items` fields must be a fixed size array. Its size determines the capacity.
#define ij_fa_append(fa, item) \
    (IJ_ASSERT((fa)->count < IJ_ARRAY_LEN((fa)->items)), \
     (fa)->items[(fa)->count++] = (item))

typedef enum{
  IJ_OK = 0,
  IJ_E_SYNTAX_ERROR,
} IJ_Error;

// lexpect is short for 'lexer expectation'
typedef enum{
  IJ_LEXPECT_MATCH,
  IJ_LEXPECT_STRING,
  IJ_LEXPECT_NUMBER
} IJ_LexpectKind;

typedef struct{
  IJ_LexpectKind kind;
  const char* pattern;
} IJ_Lexpect;

typedef enum{
  IJ_SERIALIZE,
  IJ_DESERIALIZE,
  IJ_MEASURE,
} IJ_Mode;

typedef enum{
  IJ_PRETTY = (1<<0),
} IJ_Flags;

typedef struct{
  IJ_Mode mode;
  IJ_Flags flags;
  IJ_Error error;
  struct{
    char* items;
    size_t count;
    size_t capacity;
  } buf;
  struct{
    IJ_Lexpect items[IJ_MAX_LEXPECTS];
    size_t count;
  } lexpects;
  bool first_element;
  bool member;
  int indent;
} IJ;

#endif // IJ_H_

#define IJ_IMPLEMENTATION
#ifdef IJ_IMPLEMENTATION

bool ij_init(IJ* self, char* buf, size_t bufsize, IJ_Mode mode, IJ_Flags flags){
  memset(self, 0, sizeof(*self));
  self->mode = mode;
  self->flags = flags;
  self->first_element = true;
  switch(mode){
    case IJ_DESERIALIZE:
      assert(buf != NULL);
      self->buf.items = buf;
      self->buf.count = bufsize;
    break;
    case IJ_SERIALIZE:
      assert(buf != NULL);
      self->buf.items = buf;
      self->buf.capacity= bufsize;
    break;
    case IJ_MEASURE:
      self->buf.items = NULL;
      self->buf.count = 0;
    break;
  }
  return true;
}

bool ij_deserialize(IJ* self, const char* buf, size_t bufsize){
  return ij_init(self, (char*)buf, bufsize, IJ_DESERIALIZE, 0);
}

bool ij_serialize(IJ* self, char* buf, size_t bufsize){
  return ij_init(self, buf, bufsize, IJ_SERIALIZE, 0);
}

bool ij_measure(IJ* self){
  return ij_init(self, NULL, 0, IJ_MEASURE, 0);
}

IJ_Flags* ij_flags(IJ* self){
  return &self->flags;
}

const char* ij_buf(IJ* self){
  switch(self->mode){
    case IJ_DESERIALIZE: return self->buf.items;
    case IJ_SERIALIZE: return self->buf.items;
    case IJ_MEASURE: return self->buf.items;
  }
  assert(false && "unreachable");
}

size_t ij_bufsize(IJ* self){
  switch(self->mode){
    case IJ_DESERIALIZE: return self->buf.count;
    case IJ_SERIALIZE: return self->buf.count;
    case IJ_MEASURE: return self->buf.count;
  }
  assert(false && "unreachable");
}

bool ij_lex_is_ws(char ch);
void ij_lex_consume_ws(IJ* self);
bool ij_deinit(IJ* self){
  if(self->lexpects.count > 0){
    IJ_LOG_ERROR("syntax errror, expected one of: ");
    for(size_t i = 0; i < self->lexpects.count; ++i){
      IJ_LOG_ERROR("- '%s'", self->lexpects.items[i].pattern);
    }

    ij_lex_consume_ws(self);
    int n = 0;
    while((size_t)n < self->buf.count
        && ij_lex_is_ws(self->buf.items[n]) == false
    ){
      n++;
    }
    
    IJ_LOG_ERROR("got: '%.*s'", n, self->buf.items);
  }
  return true;
}

bool ij_has_error(IJ* self){
  return self->error != IJ_OK;
}

bool ij_appendf(IJ* self, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    assert(n >= 0);
    va_end(args);

    if(self->buf.items == NULL){
      // just measure what the resulting string would be
      self->buf.count += n;
      return true;
    }

    if(self->buf.count + n + 1 >= self->buf.capacity){
      return false;
    }

    char *dest = self->buf.items + self->buf.count;
    va_start(args, fmt);
    assert(vsnprintf(dest, n+1, fmt, args) == n);
    va_end(args);

    self->buf.count += n;

    return true;
}

bool ij_append_indent(IJ* self){
  if((self->flags & IJ_PRETTY) == 0) return true;
  if(self->buf.count == 0) return true;
  if(self->member){
    self->member = false;
    return true;
  }
  return ij_appendf(self, "\n%*s", self->indent, "");
}


bool ij_sv_eq(IJ_StringView a, IJ_StringView b){
  if(a.count != b.count) return false;

  return memcmp(&a.count, &b.count, a.count) == 0;
}

void ij_lex_expect(IJ* self, IJ_Lexpect lexpect){
  ij_fa_append(&self->lexpects, lexpect);
}

void ij_lex_reset_expects(IJ* self){
  self->lexpects.count = 0;
}

bool ij_lex_is_ws(char ch){
  return ch == ' ' 
    || ch == '\r'
    || ch == '\n'
    || ch == '\t';
}

bool ij_lex_is_digit(char ch){
  return ch >= '0' && ch <= '9';
}

IJ_StringView ij_lex_consume(IJ* self, size_t count){
  IJ_StringView token = ij_sv_from_parts(self->buf.items, self->buf.count);
  token.count = count;
  self->buf.count -= count;
  self->buf.items += count;
  IJ_LOG_INFO("ij_lex_consume: '%.*s'", (int)token.count, token.data);
  ij_lex_reset_expects(self);
  return token;
}

void ij_lex_consume_ws(IJ* self){
  while(self->buf.count > 0
      && ij_lex_is_ws(self->buf.items[0])
  ){
    self->buf.count--;
    self->buf.items++;
  }
}

bool ij_lex_match(IJ* self, const char* match){
  ij_lex_consume_ws(self);

  ij_lex_expect(self, ((IJ_Lexpect){ 
      .kind=IJ_LEXPECT_MATCH,
      .pattern=match
  }));

  assert(match != NULL);
  assert(match[0] != '\0');

  size_t n = 0;
  while(n < self->buf.count
      && match[n] != '\0'
  ){
    if(self->buf.items[n] != match[n]) return false;
    n++;
  }

  ij_lex_consume(self, n);
  return true;
}

bool ij_lex_number(IJ* self, IJ_StringView* sv){
  ij_lex_consume_ws(self);
  ij_lex_expect(self, ((IJ_Lexpect){ 
      .kind=IJ_LEXPECT_NUMBER,
      .pattern="<number>"
  }));
  
  if(self->buf.count <= 0
      || ij_lex_is_digit(self->buf.items[0]) == false
  ){
    return false;
  }

  size_t n = 0;
  while(n < self->buf.count
      && ij_lex_is_digit(self->buf.items[n])
  ){
    n++;
  }
  
  if(n < self->buf.count
      && self->buf.items[n] == '.'
  ){
    n++;
    while(n < self->buf.count
        && ij_lex_is_digit(self->buf.items[n])
    ){
      n++;
    }
  }

  *sv = ij_lex_consume(self, n);
  return true;
}

bool ij_lex_string(IJ* self, IJ_StringView* sv){
  ij_lex_consume_ws(self);
  ij_lex_expect(self, ((IJ_Lexpect){ 
      .kind=IJ_LEXPECT_STRING,
      .pattern="\"<string>\""
  }));
  
  if(self->buf.count <= 0
      || self->buf.items[0] != '\"'
  ){
    return false;
  }

  size_t n = 1;
  while(n < self->buf.count
      && (self->buf.items[n] != '\"'
        || self->buf.items[n-1] == '\\'
      )
  ){
    n++;
  }
  n++;

  assert(n >= 2);

  *sv = ij_lex_consume(self, n);
  sv->count -= 2;
  sv->data += 1;
  return true;
}

bool ij_comma_check(IJ* self){
  switch(self->mode){
    case IJ_SERIALIZE:
    case IJ_MEASURE:
      if(self->first_element == false){
        return ij_appendf(self, ",");
      }else{
        self->first_element = false;
        return true;
      }
    break;
    case IJ_DESERIALIZE:
      if(self->first_element == false){
        return ij_lex_match(self, ",");
      }else{
        self->first_element = false;
        return true;
      }
    break;
  }
  assert(false && "unreachable");
}


bool ij_f64(IJ* self, double* value){
  if(ij_has_error(self)) return false;
  if(ij_comma_check(self) == false) return false;
  if(ij_append_indent(self) == false) return false;

  switch(self->mode){
    case IJ_MEASURE:
    case IJ_SERIALIZE:
    return ij_appendf(self, "%f", *value);
    case IJ_DESERIALIZE:{
      IJ_StringView sv;
      if(ij_lex_number(self, &sv) == false) return false;
      char* endptr = NULL;
      if(value != NULL) *value = strtod(sv.data, &endptr);
      assert(endptr != sv.data);
    }return true;
  }
  assert(false && "unreachable");
}

bool ij_usize(IJ* self, size_t* value){
  if(ij_has_error(self)) return false;
  if(ij_comma_check(self) == false) return false;
  if(ij_append_indent(self) == false) return false;

  switch(self->mode){
    case IJ_MEASURE:
    case IJ_SERIALIZE:
    return ij_appendf(self, "%zu", *value);
    case IJ_DESERIALIZE:{
      IJ_StringView sv;
      if(ij_lex_number(self, &sv) == false) return false;
      char* endptr = NULL;
      if(value != NULL) *value = strtoul(sv.data, &endptr, 10);
      assert(endptr != sv.data);
    }return true;
  }
  assert(false && "unreachable");
}

bool ij_string(IJ* self, const char** str, size_t* len){
  if(ij_has_error(self)) return false;
  if(ij_comma_check(self) == false) return false;;
  if(ij_append_indent(self) == false) return false;

  switch(self->mode){
    case IJ_MEASURE:
    case IJ_SERIALIZE:
    return ij_appendf(self, "\"%.*s\"", *len, *str);
    case IJ_DESERIALIZE:{
      IJ_StringView sv;
      if(ij_lex_string(self, &sv) == false) return false;
      if(str != NULL) *str = sv.data;
      if(len != NULL) *len = sv.count;
    }return true;
  }
  assert(false && "unreachable");
}

bool ij_array_begin(IJ* self){
  if(ij_has_error(self)) return false;
  if(ij_comma_check(self) == false) return false;;
  if(ij_append_indent(self) == false) return false;

  self->first_element = true;
  switch(self->mode){
    case IJ_MEASURE: 
    case IJ_SERIALIZE:
      self->first_element = true;
    return ij_appendf(self, "[");
    case IJ_DESERIALIZE:
    return ij_lex_match(self, "[");
  }
  assert(false && "unreachable");
}

bool ij_array_end(IJ* self, size_t* count){
  // end array if error
  if(ij_has_error(self)) return true;

  assert(count != NULL);

  switch(self->mode){
    case IJ_MEASURE:
    case IJ_SERIALIZE:
      if(*count > 1){
        *count -= 1;
        return false;
      }
    return ij_appendf(self, "]");
    case IJ_DESERIALIZE:
      if(ij_lex_match(self, "]")){
        *count += 1;
        return true;
      }else{
      //if(ij_lex_match(self, ",")){
        *count += 1;
        return false;
      }
      
      // error so we indicate the end of the array
      self->error = IJ_E_SYNTAX_ERROR;
    return true;
  }
  assert(false && "unreachable");
}

bool ij_obj_begin(IJ* self){
  if(ij_has_error(self)) return false;
  if(ij_comma_check(self) == false) return false;;
  if(ij_append_indent(self) == false) return false;

  self->first_element = true;
  switch(self->mode){
    case IJ_MEASURE:
    case IJ_SERIALIZE:
      self->first_element = true;
      if(ij_appendf(self, "{") == false) return false;
      self->indent += IJ_INDENT_INCREMENT;
    return true;
    case IJ_DESERIALIZE:
      if(ij_lex_match(self, "{") == false) return false;
    return true;
  }
  assert(false && "unreachable");
}

bool ij_obj_end(IJ* self){
  if(ij_has_error(self)) return true;
    
  self->indent -= IJ_INDENT_INCREMENT;
  if(ij_append_indent(self) == false) return false;

  switch(self->mode){
    case IJ_MEASURE:
    case IJ_SERIALIZE:
      if(ij_appendf(self, "}") == false) return false;
    return true;
    case IJ_DESERIALIZE:
      if(ij_lex_match(self, "}") == false) return false;
    return true;
  }
  assert(false && "unreachable");
}

bool ij_obj_member(IJ* self, const char* member){
  if(ij_has_error(self)) return false;
  if(ij_comma_check(self) == false) return false;;
  if(ij_append_indent(self) == false) return false;
  
  switch(self->mode){
    case IJ_MEASURE:
    case IJ_SERIALIZE:
      self->first_element = true;
      self->member = true;
    return ij_appendf(self, "\"%s\":", member);
    case IJ_DESERIALIZE:{
      struct{
        char* data;
        size_t count;
        size_t lexpects;
      } snapshot = {
        .data = self->buf.items,
        .count = self->buf.count,
        .lexpects = self->lexpects.count,
      };

      if(ij_lex_match(self, "\"") == false) return false;

      if(ij_lex_match(self, member) == false
          || ij_lex_match(self, "\"") == false
      ){
        // this is valid as there might be a different member in this place
        // so we restore the lexer state to before returning
        self->buf.items = snapshot.data;
        self->buf.count = snapshot.count;
        self->lexpects.count = snapshot.lexpects;
        return false;
      }
      
      if(ij_lex_match(self, ":") == false){
        self->error = IJ_E_SYNTAX_ERROR;
        return false;
      }
      self->first_element = true;
    }return true;
  }
}

#endif
