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

typedef struct{
  const char* data;
  size_t count;
} IJ_StringView;

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

typedef struct{
  IJ_StringView src;
  struct{
    IJ_Lexpect items[IJ_MAX_LEXPECTS];
    size_t count;
  } lexpects;
} IJ_Lexer;

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
  IJ_E_NO_MODE_CONFIGURED,
  IJ_E_SYNTAX_ERROR,
} IJ_Error;

typedef enum{
  IJ_SERIALIZE = (1<<0),
  IJ_DESERIALIZE = (0<<0),
  IJ_PRETTY = (1<<1),
} IJ_Flags;

typedef struct{
  IJ_Flags flags;
  IJ_Error error;
  IJ_Lexer lexer;
  IJ_StringBuilder sb;
  bool first_element;
} IJ;

#endif // IJ_H_

#define IJ_IMPLEMENTATION
#ifdef IJ_IMPLEMENTATION

bool ij_init(IJ* ij, char* buf, size_t bufsize, IJ_Flags flags){
  ij->flags = flags;
  ij->first_element = true;
  if(flags & IJ_SERIALIZE){
     
  }else{
    ij->lexer.src.data = buf;
    ij->lexer.src.count = bufsize;
  }
  return true;
}

bool ij_deserialize(IJ* ij, char* buf, size_t bufsize){
  return ij_init(ij, buf, bufsize, IJ_DESERIALIZE);  
}

bool ij_serialize(IJ* ij){
  return ij_init(ij, NULL, 0, IJ_SERIALIZE);  
}

const char* ij_buf(IJ* ij){
  if(ij->flags & IJ_SERIALIZE){
    return ij->sb.items;
  }else{
    return ij->lexer.src.data;
  }
}

size_t ij_bufsize(IJ* ij){
  if(ij->flags & IJ_SERIALIZE){
    return ij->sb.count;
  }else{
    return ij->lexer.src.count;
  }
}

bool ij_lex_is_ws(char ch);
void ij_lex_consume_ws(IJ_Lexer* lexer);
bool ij_deinit(IJ* ij){
  if(ij->sb.capacity > 0){
    IJ_FREE(ij->sb.items);
  }
  if(ij->lexer.lexpects.count > 0){
    IJ_LOG_ERROR("syntax errror, expected one of: ");
    for(size_t i = 0; i < ij->lexer.lexpects.count; ++i){
      IJ_LOG_ERROR("- '%s'", ij->lexer.lexpects.items[i].pattern);
    }

    ij_lex_consume_ws(&ij->lexer);
    int n = 0;
    while((size_t)n < ij->lexer.src.count
        && ij_lex_is_ws(ij->lexer.src.data[n]) == false
    ){
      n++;
    }
    
    IJ_LOG_ERROR("got: '%.*s'", n, ij->lexer.src.data);
  }
  memset(ij, 0, sizeof(*ij));
  return true;
}

bool ij_has_error(IJ* ij){
  return ij->error != IJ_OK;
}

// "borrowed" from nob.h
int ij_sb_appendf(IJ_StringBuilder*sb, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    int n = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    // NOTE: the new_capacity needs to be +1 because of the null terminator.
    // However, further below we increase sb->count by n, not n + 1.
    // This is because we don't want the sb to include the null terminator. The user can always sb_append_null() if they want it
    ij_da_reserve(sb, sb->count + n + 1);

    char *dest = sb->items + sb->count;
    va_start(args, fmt);
    vsnprintf(dest, n+1, fmt, args);
    va_end(args);

    sb->count += n;

    return n;
}

bool ij_sv_eq(IJ_StringView a, IJ_StringView b){
  if(a.count != b.count) return false;

  return memcmp(&a.count, &b.count, a.count) == 0;
}

void ij_lex_expect(IJ_Lexer* lexer, IJ_Lexpect lexpect){
  ij_fa_append(&lexer->lexpects, lexpect);
}

void ij_lex_reset_expects(IJ_Lexer* lexer){
  lexer->lexpects.count = 0;
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

IJ_StringView ij_lex_consume(IJ_Lexer* lexer, size_t count){
  IJ_StringView token = lexer->src;
  token.count = count;
  lexer->src.count -= count;
  lexer->src.data += count;
  IJ_LOG_INFO("ij_lex_consume: '%.*s'", (int)token.count, token.data);
  ij_lex_reset_expects(lexer);
  return token;
}

void ij_lex_consume_ws(IJ_Lexer* lexer){
  while(lexer->src.count > 0
      && ij_lex_is_ws(lexer->src.data[0])
  ){
    lexer->src.count--;
    lexer->src.data++;
  }
}

bool ij_lex_match(IJ_Lexer* lexer, const char* match){
  ij_lex_consume_ws(lexer);

  ij_lex_expect(lexer, ((IJ_Lexpect){ 
      .kind=IJ_LEXPECT_MATCH,
      .pattern=match
  }));

  assert(match != NULL);
  assert(match[0] != '\0');

  size_t n = 0;
  while(n < lexer->src.count
      && match[n] != '\0'
  ){
    if(lexer->src.data[n] != match[n]) return false;
    n++;
  }

  ij_lex_consume(lexer, n);
  return true;
}

bool ij_lex_number(IJ_Lexer* lexer, IJ_StringView* sv){
  ij_lex_consume_ws(lexer);
  ij_lex_expect(lexer, ((IJ_Lexpect){ 
      .kind=IJ_LEXPECT_NUMBER,
      .pattern="<number>"
  }));
  
  if(lexer->src.count <= 0
      || ij_lex_is_digit(lexer->src.data[0]) == false
  ){
    return false;
  }

  size_t n = 0;
  while(n < lexer->src.count
      && ij_lex_is_digit(lexer->src.data[n])
  ){
    n++;
  }
  
  if(n < lexer->src.count
      && lexer->src.data[n] == '.'
  ){
    n++;
    while(n < lexer->src.count
        && ij_lex_is_digit(lexer->src.data[n])
    ){
      n++;
    }
  }

  *sv = ij_lex_consume(lexer, n);
  return true;
}

bool ij_lex_string(IJ_Lexer* lexer, IJ_StringView* sv){
  ij_lex_consume_ws(lexer);
  ij_lex_expect(lexer, ((IJ_Lexpect){ 
      .kind=IJ_LEXPECT_STRING,
      .pattern="\"<string>\""
  }));
  
  if(lexer->src.count <= 0
      || lexer->src.data[0] != '\"'
  ){
    return false;
  }

  size_t n = 1;
  while(n < lexer->src.count
      && (lexer->src.data[n] != '\"'
        || lexer->src.data[n-1] == '\\'
      )
  ){
    n++;
  }
  n++;

  assert(n >= 2);

  *sv = ij_lex_consume(lexer, n);
  sv->count -= 2;
  sv->data += 1;
  return true;
}

void ij_comma_check(IJ* ij){
  if(ij->flags & IJ_SERIALIZE){
    if(ij->first_element == false){
      ij_sb_appendf(&ij->sb, ",");
    }else{
      ij->first_element = false;
    }
  }
}


bool ij_f64(IJ* ij, double* value){
  if(ij_has_error(ij)) return false;

  if(ij->flags & IJ_SERIALIZE){
    ij_comma_check(ij);
    return ij_sb_appendf(&ij->sb, "%f", *value);
  }else{
    IJ_StringView sv;
    if(ij_lex_number(&ij->lexer, &sv) == false) return false;
    if(value != NULL) *value = strtod(sv.data, NULL);
    return true;
  }
}

bool ij_usize(IJ* ij, size_t* value){
  if(ij_has_error(ij)) return false;

  if(ij->flags & IJ_SERIALIZE){
    ij_comma_check(ij);
    return ij_sb_appendf(&ij->sb, "%zu", *value);
  }else{
    IJ_StringView sv;
    if(ij_lex_number(&ij->lexer, &sv) == false) return false;
    if(value != NULL) *value = strtod(sv.data, NULL);
    return true;
  }
}

bool ij_string(IJ* ij, const char** str, size_t* len){
  if(ij_has_error(ij)) return false;

  if(ij->flags & IJ_SERIALIZE){
    ij_comma_check(ij);
    return ij_sb_appendf(&ij->sb, "\"%.*s\"", *len, *str);
  }else{
    IJ_StringView sv;
    if(ij_lex_string(&ij->lexer, &sv) == false) return false;
    if(str != NULL) *str = sv.data;
    if(len != NULL) *len = sv.count;
    return true;
  }
}

bool ij_array_begin(IJ* ij){
  if(ij_has_error(ij)) return false;

  ij->first_element = true;
  if(ij->flags & IJ_SERIALIZE){
    ij_comma_check(ij);
    ij->first_element = true;
    return ij_sb_appendf(&ij->sb, "[");
  }else{
    return ij_lex_match(&ij->lexer, "[");
  }
}

bool ij_array_end(IJ* ij, size_t* count){
  // end array if error
  if(ij_has_error(ij)) return true;

  assert(count != NULL);

  if(ij->flags & IJ_SERIALIZE){
    if(*count > 1){
      *count -= 1;
      return false;
    }
    return ij_sb_appendf(&ij->sb, "]");
  }else{
    if(ij_lex_match(&ij->lexer, "]")){
      *count += 1;
      return true;
    }
    if(ij_lex_match(&ij->lexer, ",")){
      *count += 1;
      return false;
    }
    
    // error so we indicate the end of the array
    ij->error = IJ_E_SYNTAX_ERROR;
    return true;
  }
}

bool ij_obj_begin(IJ* ij){
  if(ij_has_error(ij)) return false;

  ij->first_element = true;
  if(ij->flags & IJ_SERIALIZE){
    ij_comma_check(ij);
    ij->first_element = true;
    return ij_sb_appendf(&ij->sb, "{");
  }else{
    return ij_lex_match(&ij->lexer, "{");
  }
}

bool ij_obj_end(IJ* ij){
  if(ij_has_error(ij)) return true;

  if(ij->flags & IJ_SERIALIZE){
    return ij_sb_appendf(&ij->sb, "}");
  }else{
    if(ij_lex_match(&ij->lexer, "}")) return true;
    if(ij_lex_match(&ij->lexer, ",")) return false;
    
    // error so we indicate the end of the object
    ij->error = IJ_E_SYNTAX_ERROR;
    return true;
  }
}

bool ij_obj_member(IJ* ij, const char* member){
  if(ij_has_error(ij)) return false;
  
  if(ij->flags & IJ_SERIALIZE){
    ij_comma_check(ij);
    ij->first_element = true;
    return ij_sb_appendf(&ij->sb, "\"%s\":", member);
  }else{
    IJ_Lexer snapshot = ij->lexer;
    if(ij_lex_match(&ij->lexer, "\"") == false) return false;

    if(ij_lex_match(&ij->lexer, member) == false
        || ij_lex_match(&ij->lexer, "\"") == false
    ){
      // this is valid as there might be a different member in this place
      // so we restore the lexer state to before the lexing
      ij->lexer = snapshot;
      return false;
    }
    
    if(ij_lex_match(&ij->lexer, ":") == false){
      ij->error = IJ_E_SYNTAX_ERROR;
      return false;
    }

    return true;
  }
}

#endif
