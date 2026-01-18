/*
 * MIT License
 * 
 * Copyright (c) 2026 Alaric de Ruiter
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#ifndef IJ_H_
#define IJ_H_
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef IJ_LOG_INFO
#define IJ_LOG_INFO(fmt, ...) fprintf(stderr, "[IJ_INFO]: " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#endif
#ifndef IJ_LOG_ERROR
#define IJ_LOG_ERROR(fmt, ...) fprintf(stderr, "[IJ_ERROR]: " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#endif

#ifndef IJ_DEFAULT_BUF_SIZE
#define IJ_DEFAULT_BUF_SIZE 1024
#endif 

#ifndef IJ_SB_APPENDF_BUF_SIZE
#define IJ_SB_APPENDF_BUF_SIZE 1024
#endif

#define IJ_IMPLEMENTATION

typedef enum{
  IJ_E_OK = 0,
  IJ_E_UNEXPECTED_TOKEN,
  IJ_E_BUF_FULL,
  IJ_E_WRITE_FAILURE,
  IJ_E_SB_APPENDF_BUF_TOO_SMALL,
  IJ_E_END_OF_INPUT,
  IJ_E_MORE_ELEMENTS_AVAILABLE,
  IJ_E_ARG_STRNDUP_REQUIRED,
  IJ_E_ARG_NO_INPUT_METHOD,
  IJ_E_ARG_NO_BUF,
} IJ_Error;

typedef int (*IJ_ReadCallback)(void* ctx, char* buf, int len);
typedef int (*IJ_WriteCallback)(void* ctx, char* buf, int len);

typedef struct{
  void* ctx;
  IJ_WriteCallback write;
  IJ_ReadCallback read;
} IJ_Stream;

int ij_stream_write(IJ_Stream* self, char* buf, size_t size);
int ij_stream_read(IJ_Stream* self, char* buf, size_t size);

#ifdef IJ_IMPLEMENTATION
int ij_stream_write(IJ_Stream* self, char* buf, size_t size){
  return self->write(self->ctx, buf, size);
}

int ij_stream_read(IJ_Stream* self, char* buf, size_t size){
  return self->read(self->ctx, buf, size);
}
#endif

typedef enum{
  IJ_TOKEN_UNKNOWN,
  IJ_TOKEN_KW_NULL,
  IJ_TOKEN_KW_TRUE,
  IJ_TOKEN_KW_FALSE,
  IJ_TOKEN_STRING,
  IJ_TOKEN_NUMBER,
  IJ_TOKEN_COMMA,
  IJ_TOKEN_COLON,
  IJ_TOKEN_CURLY_OPEN,
  IJ_TOKEN_CURLY_CLOSE,
  IJ_TOKEN_SQUARE_OPEN,
  IJ_TOKEN_SQUARE_CLOSE,
} IJ_TokenKind;

const char* IJ_TokenKind_str(IJ_TokenKind kind);

#ifdef IJ_IMPLEMENTATION
const char* IJ_TokenKind_str(IJ_TokenKind kind){
  switch (kind) {
    case IJ_TOKEN_UNKNOWN: return "IJ_TOKEN_UNKNOWN";
    case IJ_TOKEN_KW_NULL: return "IJ_TOKEN_KW_NULL";
    case IJ_TOKEN_KW_TRUE: return "IJ_TOKEN_KW_TRUE";
    case IJ_TOKEN_KW_FALSE: return "IJ_TOKEN_KW_FALSE";
    case IJ_TOKEN_STRING: return "IJ_TOKEN_STRING";
    case IJ_TOKEN_NUMBER: return "IJ_TOKEN_NUMBER";
    case IJ_TOKEN_COMMA: return "IJ_TOKEN_COMMA";
    case IJ_TOKEN_COLON: return "IJ_TOKEN_COLON";
    case IJ_TOKEN_CURLY_OPEN: return "IJ_TOKEN_CURLY_OPEN";
    case IJ_TOKEN_CURLY_CLOSE: return "IJ_TOKEN_CURLY_CLOSE";
    case IJ_TOKEN_SQUARE_OPEN: return "IJ_TOKEN_SQUARE_OPEN";
    case IJ_TOKEN_SQUARE_CLOSE: return "IJ_TOKEN_SQUARE_CLOSE";
  }
  return "<inavlid token>";
}
#endif // IJ_IMPLEMENTATION

typedef struct{
  IJ_TokenKind kind;
  char* str;
  int len;
} IJ_Token;

bool ij_token_str_eq(IJ_Token* token, const char* str);

#ifdef IJ_IMPLEMENTATION
bool ij_token_str_eq(IJ_Token* token, const char* str){
  return strncmp(token->str, str, token->len) == 0;
}
#endif // IJ_IMPLEMENTATION

typedef struct{
  char* begin;
  char* curr;
  char* end;
  IJ_Token token;
  IJ_Error error;
  IJ_Stream* stream;
} IJ_Lexer;

typedef struct{
  IJ_Lexer lexer;
} IJ_LexerSnapshot;

IJ_LexerSnapshot ij_lexer_snapshot(IJ_Lexer* lexer);
void ij_lexer_restore(IJ_Lexer* self, IJ_LexerSnapshot snapshot);
bool ij_lexer_is_letter(char c);
bool ij_lexer_is_whitespace(char c);
bool ij_lexer_is_digit(char c);
bool ij_lexer_next(IJ_Lexer* self);
bool ij_lexer_next_is(IJ_Lexer* self, IJ_TokenKind kind);
bool ij_lexer_expect(IJ_Lexer* self, IJ_TokenKind kind);
bool ij_lexer_expect_str(IJ_Lexer* self, const char* str);

#ifdef IJ_IMPLEMENTATION

void ij_lexer_init(IJ_Lexer* self, 
    char* buf, int len, 
    IJ_Stream* stream
){
  self->begin = buf;
  self->curr = buf;
  self->end = buf+len;
  self->stream = stream;
}

IJ_LexerSnapshot ij_lexer_snapshot(IJ_Lexer* lexer){
  return (IJ_LexerSnapshot){ .lexer = *lexer };
}

void ij_lexer_restore(IJ_Lexer* self, IJ_LexerSnapshot snapshot){
  if(self->token.kind == IJ_TOKEN_STRING){
    if(self->token.str + self->token.len < self->end){
      // restore null terminator of string to quote
      self->token.str[self->token.len] = '"';
    }
  }
  *self = snapshot.lexer;
}

bool ij_lexer_is_letter(char c){
  return c >= 'a' && c <= 'z';
}

bool ij_lexer_is_whitespace(char c){
  return c == ' ' || c == '\n';
}

bool ij_lexer_is_digit(char c){
  return c >= '0' && c <= '9';
}

bool ij_lexer_read_stream(IJ_Lexer* self){
  if(self->stream->read == NULL) return false;

  IJ_LOG_INFO("ij_lexer_read_stream: buffer %p %d '%.*s'", 
      self->begin,
      (int)(self->end-self->begin), 
      (int)(self->end-self->begin), 
      self->begin); 
  IJ_LOG_INFO("ij_lexer_read_stream: moving token '%.*s' to begin", 
      self->token.len, self->token.str);
  // copy current token to begin of buffer
  char* r_it = self->token.str;
  char* w_it = self->begin;
  if(r_it != w_it){
    while(r_it < self->end){
      *w_it = *r_it;
      r_it++;
      w_it++;
    }
  }

  if(w_it >= self->end){
    self->error = IJ_E_BUF_FULL;
    return false;
  }

  // continue reading token at new location
  self->token.str = self->begin;
  self->curr = w_it;

  int nread = self->stream->read(self->stream->ctx, w_it, self->end-w_it);
  if(nread <= 0){
    IJ_LOG_ERROR("lexer: read failed");
    self->error = IJ_E_END_OF_INPUT;
    return false;
  }
  IJ_LOG_INFO("ij_lexer_read_stream: '%.*s'", (int)(self->end-w_it), w_it);

  //self->end = self->begin + nread;
  return true;
}

bool ij_lexer_next_char(IJ_Lexer* self){
  self->curr++;
  if(self->curr >= self->end){
    if(self->stream->read == NULL){
      IJ_LOG_ERROR("lexer: no more chars available");
      self->error = IJ_E_END_OF_INPUT;
      return false;
    }else{
      return ij_lexer_read_stream(self);
    }
  }
  return true;
}

bool ij_lexer_next(IJ_Lexer* self){
  //IJ_LOG_INFO("lexer next '%.*s'", (int)(self->str_end-self->str), self->str);
  self->token.kind = IJ_TOKEN_UNKNOWN;
  self->token.len = 0;
  self->token.str = self->curr;

  while(ij_lexer_is_whitespace(*self->curr)){
    if(ij_lexer_next_char(self) == false) return false;
  }

  self->token.str = self->curr;

  if(*self->curr == '\0'){
    if(ij_lexer_read_stream(self) == true){
      return ij_lexer_next(self);
    }
    self->error = IJ_E_END_OF_INPUT;
    return false;
  }else if(*self->curr == '"'){
    self->token.kind = IJ_TOKEN_STRING;
    if(ij_lexer_next_char(self) == false) return false;
    while(*self->curr != '"'){
      if(ij_lexer_next_char(self) == false) return false;
    }
    if(*self->curr == '"'){
      if(ij_lexer_next_char(self) == false) return false;
    }
  }else if(ij_lexer_is_digit(*self->curr)
      || *self->curr == '-'
      || *self->curr == '+'
  ){
    self->token.kind = IJ_TOKEN_NUMBER;
    self->curr++;
    while(ij_lexer_is_digit(*self->curr)){
      if(ij_lexer_next_char(self) == false) return false;
    }
    if(*self->curr == '.'){
      if(ij_lexer_next_char(self) == false) return false;
      while(ij_lexer_is_digit(*self->curr)){
        if(ij_lexer_next_char(self) == false) return false;
      }
    }
  }else if(ij_lexer_is_letter(*self->curr)){
    // keywords
    while(ij_lexer_is_letter(*self->curr)){
      if(ij_lexer_next_char(self) == false) return false;
    }
    int len = self->curr-self->token.str;
    if(len == 4 || len == 5){
      if(strcmp(self->token.str, "null") == 0){
        self->token.kind = IJ_TOKEN_KW_NULL;
      }else if(strcmp(self->token.str, "true") == 0){
        self->token.kind = IJ_TOKEN_KW_TRUE;
      }else if(strcmp(self->token.str, "false") == 0){
        self->token.kind = IJ_TOKEN_KW_FALSE;
      }
    }
  }else if(*self->curr == ','){
    self->token.kind = IJ_TOKEN_COMMA;
    if(ij_lexer_next_char(self) == false) return false;
  }else if(*self->curr == ':'){
    self->token.kind = IJ_TOKEN_COLON;
    if(ij_lexer_next_char(self) == false) return false;
  }else if(*self->curr == '{'){
    self->token.kind = IJ_TOKEN_CURLY_OPEN;
    if(ij_lexer_next_char(self) == false) return false;
  }else if(*self->curr == '}'){
    self->token.kind = IJ_TOKEN_CURLY_CLOSE;
    if(ij_lexer_next_char(self) == false) return false;
  }else if(*self->curr == '['){
    self->token.kind = IJ_TOKEN_SQUARE_OPEN;
    if(ij_lexer_next_char(self) == false) return false;
  }else if(*self->curr == ']'){
    self->token.kind = IJ_TOKEN_SQUARE_CLOSE;
    if(ij_lexer_next_char(self) == false) return false;
  }else{
    IJ_LOG_ERROR("unknown char: '%c' (0x%02X)", *self->curr, *self->curr);
    self->error = IJ_E_UNEXPECTED_TOKEN;
    return false;
  }

  self->token.len = self->curr-self->token.str;
  IJ_LOG_INFO("token %s '%.*s'", 
      IJ_TokenKind_str(self->token.kind), 
      self->token.len, self->token.str);

  if(self->token.kind == IJ_TOKEN_STRING){
    // set last quote to null term
    self->token.str[self->token.len-1] = '\0';
    self->token.len-=2;
    self->token.str++;
  }

  return true;
}

bool ij_lexer_next_is(IJ_Lexer* self, IJ_TokenKind kind){
    IJ_LexerSnapshot snapshot = ij_lexer_snapshot(self);

    if(ij_lexer_next(self) == false) return false;
    if(self->token.kind != kind){
      ij_lexer_restore(self, snapshot);
      return false;
    }

    return true;
}

bool ij_lexer_expect(IJ_Lexer* self, IJ_TokenKind kind){
  IJ_LexerSnapshot snapshot = ij_lexer_snapshot(self);

  if(ij_lexer_next(self) == false) return false;
  if(self->token.kind != kind){
    self->error = IJ_E_UNEXPECTED_TOKEN;
    IJ_LOG_ERROR("expected %s, got %s", 
        IJ_TokenKind_str(kind),
        IJ_TokenKind_str(self->token.kind));
    ij_lexer_restore(self, snapshot);
    return false;
  }

  return true;
}

bool ij_lexer_expect_str(IJ_Lexer* self, const char* str){
  IJ_Lexer snapshot = *self;

  if(ij_lexer_expect(self, IJ_TOKEN_STRING) == false){
    *self = snapshot;
    return false;
  }

  if(ij_token_str_eq(&self->token, str) == false){
    *self = snapshot;
    return false;
  }

  return true;
}

#endif // IJ_IMPLEMENTATION

typedef struct{
  char* begin;
  char* curr;
  char* end;
  bool pretty;
  int indent;
  IJ_Error error;
  IJ_Stream* stream;
} IJ_StringBuilder;

void ij_sb_init(IJ_StringBuilder* self, 
    char* buf, int len,
    IJ_Stream* stream);
void ij_sb_increase_indent(IJ_StringBuilder* self);
void ij_sb_decrease_indent(IJ_StringBuilder* self);
bool ij_sb_append_indent(IJ_StringBuilder* self);
bool ij_sb_append_newline(IJ_StringBuilder* self);
bool ij_sb_append_cstr(IJ_StringBuilder* self, const char* cstr);
bool ij_sb_appendf(IJ_StringBuilder* self, const char *fmt, ...);

#ifdef IJ_IMPLEMENTATION

void ij_sb_init(IJ_StringBuilder* self, 
    char* buf, int len,
    IJ_Stream* stream
){
  self->begin = buf;
  self->curr = buf;
  self->end = buf+len;
  self->stream = stream;
}

bool ij_sb_reserve(IJ_StringBuilder* self, int n){
  if(self->curr+n >= self->end){
    if(self->stream->write != NULL){
      IJ_LOG_INFO("ij_sb_append_cstr: writing out buffer");
      assert(false && "TODO");
    }else{
      IJ_LOG_ERROR("ij_sb_append_cstr: buffer is full");
      self->error = IJ_E_BUF_FULL;
      return false;
    }
  }else{
    return true;
  }
}

bool ij_sb_flush(IJ_StringBuilder* self){
  IJ_LOG_INFO("ij_sb_put_char: flushing buffer");
  int nwrite = self->curr-self->begin;
  int nwrote = self->stream->write(self->stream->ctx, self->begin, nwrite);
  if(nwrite != nwrote){
    IJ_LOG_ERROR("ij_sb_put_char: failed to write all bytes");
    self->error = IJ_E_WRITE_FAILURE;
    return false;
  }
  self->curr = self->begin;
  return true;
}

bool ij_sb_put_char(IJ_StringBuilder* self, char c){
  if(self->curr < self->end){
    *self->curr = c;
    self->curr++;
    return true;
  }else{
    if(self->stream->write != NULL){
      if(ij_sb_flush(self) == false) return false;
      *self->curr = c;
      self->curr++;
      return true;
    }else if(false /* self->realloc != NULL */){
      IJ_LOG_INFO("ij_sb_append_cstr: resizing buffer");
      assert(false && "TODO");
    }else{
      IJ_LOG_ERROR("ij_sb_append_cstr: buffer is full");
      self->error = IJ_E_BUF_FULL;
      return false;
    }
  }
}

bool ij_sb_append_cstr(IJ_StringBuilder* self, const char* cstr){
  IJ_LOG_INFO("StringBuilder: append '%s'", cstr);
  while(*cstr != '\0'){
    if(ij_sb_put_char(self, *cstr) == false) return false;
    cstr++;
  }

  // assert entire cstr is appended
  assert(*cstr == '\0');

  return true;
}

bool ij_sb_appendf(IJ_StringBuilder* self, const char *fmt, ...){
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(NULL, 0, fmt, args);
  va_end(args);

  n+=1; // for null terminator
  
  if(n > IJ_SB_APPENDF_BUF_SIZE){
    IJ_LOG_ERROR("StringBuilder: buffer too small to fit format text");
    self->error = IJ_E_SB_APPENDF_BUF_TOO_SMALL;
    return false;
  }

  char buffer[IJ_SB_APPENDF_BUF_SIZE];
  va_start(args, fmt);
  vsnprintf(buffer, n, fmt, args);
  va_end(args);

  if(ij_sb_append_cstr(self, buffer) == false) return false;
  return true;
}

void ij_sb_increase_indent(IJ_StringBuilder* self){
  self->indent+=2;
}

void ij_sb_decrease_indent(IJ_StringBuilder* self){
  self->indent-=2;
}

bool ij_sb_append_indent(IJ_StringBuilder* self){

  for(int i = 0; i < self->indent; ++i){
    if(!ij_sb_put_char(self, ' ')) return false;
  }

  return true;
}

bool ij_sb_append_newline(IJ_StringBuilder* self){
  if(self->pretty == false){
    return true;
  }

  return ij_sb_put_char(self,'\n')
      && ij_sb_append_indent(self);
}

#endif // IJ_IMPLEMENTATION

typedef struct{
  bool serialize;
  IJ_Lexer lexer;
  IJ_StringBuilder sb;
  bool first_element;
  IJ_Stream stream;
} IJ;

typedef enum{
  IJ_OBJ_BEGIN,
  IJ_OBJ_END,
  IJ_ARRAY_BEGIN,
  IJ_ARRAY_END,
  IJ_STRING,
  IJ_NUMBER,
  IJ_BOOL,
  IJ_NULL
}IJ_Type;

typedef struct{
  IJ_Type type;
  union{
    const char* String;
    double Number;
    bool Bool;
    int* ArrayCount;
  } as;
} IJ_Any;

typedef struct{
  char* buf;
  int buf_len;
  bool serialize;
  bool pretty;
  int indent;
  IJ_Stream stream;
}IJ_InitOpts;

#define ij_init(self, ...)\
  ij_init_opt(self, (IJ_InitOpts){ __VA_ARGS__ })
bool ij_init_opt(IJ* self, IJ_InitOpts opts);
bool ij_deinit(IJ* self);
IJ_Error ij_error(IJ* self);

bool ij_obj_begin(IJ* self);
bool ij_obj_end(IJ* self);
bool ij_member(IJ* self, const char* name);
bool ij_array_begin(IJ* self);
bool ij_array_end(IJ* self, int* count);
bool ij_string(IJ* self, const char** value);
bool ij_number(IJ* self, double* value);
bool ij_bool(IJ* self, bool* value);
bool ij_null(IJ* self);
bool ij_any(IJ* self, IJ_Any* value);

#ifdef IJ_IMPLEMENTATION
bool ij_init_opt(IJ* self, IJ_InitOpts opts){
  self->serialize = opts.serialize;
  self->stream = opts.stream;

  if(opts.buf != NULL){
    if(opts.buf_len == 0){
      opts.buf_len = strlen(opts.buf)+1;
    }
  }else{
    IJ_LOG_ERROR("ij_init: no buffer provided");
    if(self->serialize){
      self->sb.error = IJ_E_ARG_NO_BUF;
    }else{
      self->lexer.error = IJ_E_ARG_NO_BUF;
    }
    return false;
  }

  if(self->serialize){
    ij_sb_init(&self->sb, 
        opts.buf, opts.buf_len,
        &self->stream);
    self->sb.pretty = opts.pretty;
    self->sb.indent = 0;
  }else{
    ij_lexer_init(&self->lexer, 
        opts.buf, opts.buf_len, 
        &self->stream);
  }

  self->first_element = true;
  return true;
}

bool ij_deinit(IJ* self){
  if(self->serialize){
    if(ij_sb_put_char(&self->sb, '\0') == false) return false;
    if(self->stream.write != NULL){
      if(ij_sb_flush(&self->sb) == false) return false;
    }
  }
  return true;
}

IJ_Error ij_error(IJ* self){
  if(self->serialize){
    return self->sb.error;
  }else{
    return self->lexer.error;
  }
}

bool ij_put_comma_check(IJ* self){
  if(self->first_element == false){
    if(ij_sb_append_cstr(&self->sb, ",") == false) return false;
    if(ij_sb_append_newline(&self->sb) == false) return false;
  }
  self->first_element = false;
  return true;
}

bool ij_consume_comma_check(IJ* self){
  if(self->first_element == false){
    if(ij_lexer_expect(&self->lexer, IJ_TOKEN_COMMA) == false) return false;
  }
  self->first_element = false;
  return true;
}

void ij_consume_optional_comma(IJ* self){
  ij_lexer_next_is(&self->lexer, IJ_TOKEN_COMMA);
}

bool ij_obj_begin(IJ* self){
  if(self->serialize){
    if(ij_put_comma_check(self) == false) return false;
    if(ij_sb_append_cstr(&self->sb, "{") == false) return false;
    ij_sb_increase_indent(&self->sb);
    ij_sb_append_newline(&self->sb);
    self->first_element = true;
    return true;
  }else{
    return ij_lexer_expect(&self->lexer, IJ_TOKEN_CURLY_OPEN);
  }
}

bool ij_obj_end(IJ* self){
  if(self->serialize){
    ij_sb_decrease_indent(&self->sb);
    if(ij_sb_append_newline(&self->sb) == false) return false;
    if(ij_sb_append_cstr(&self->sb, "}") == false) return false;
    self->first_element = false;
    return true;
  }else{
    if(ij_lexer_next_is(&self->lexer, IJ_TOKEN_CURLY_CLOSE)){
      return true;
    }else if(ij_lexer_next_is(&self->lexer, IJ_TOKEN_COMMA)){
      IJ_LOG_INFO("ij_obj_end: more elements are available");
      self->lexer.error = IJ_E_MORE_ELEMENTS_AVAILABLE;
      return false;
    }else{
      IJ_LexerSnapshot snapshot = ij_lexer_snapshot(&self->lexer);
      if(ij_lexer_next_is(&self->lexer, IJ_TOKEN_STRING)){
        if(ij_lexer_next_is(&self->lexer, IJ_TOKEN_COLON)){
          IJ_LOG_INFO("unhandled member: '%.*s'", 16, self->lexer.curr);
          ij_lexer_next(&self->lexer);
          return false;
        }else{
          if(ij_lexer_next(&self->lexer) == false) return true;
          IJ_LOG_ERROR("expected ':', got %s", 
              IJ_TokenKind_str(self->lexer.token.kind));
          ij_lexer_restore(&self->lexer, snapshot);
          return true;
        }
      }else{
        if(ij_lexer_next(&self->lexer) == false) return true;
        IJ_LOG_ERROR("expected ',' or '}', got %s", 
            IJ_TokenKind_str(self->lexer.token.kind));
        ij_lexer_restore(&self->lexer, snapshot);
        return true;
      }
    }
  }
}

bool ij_member(IJ* self, const char* name){
  if(self->serialize){
    if(ij_put_comma_check(self) == false) return false;
    self->first_element = true;

    if(ij_sb_appendf(&self->sb, "\"%s\":", name) == false){
      assert(false && "TODO: request more memory from user");
    }
    return true;
  }else{
    ij_consume_optional_comma(self);
    IJ_LexerSnapshot snapshot = ij_lexer_snapshot(&self->lexer);

    if(ij_lexer_expect(&self->lexer, IJ_TOKEN_STRING) == false){
      ij_lexer_restore(&self->lexer, snapshot);
      return false;
    }

    if(ij_token_str_eq(&self->lexer.token, name) == false){
      ij_lexer_restore(&self->lexer, snapshot);
      return false;
    }

    if(ij_lexer_expect(&self->lexer, IJ_TOKEN_COLON) == false){
      IJ_LOG_ERROR("expected ':' between member key and value, got '%.*s'", 
          self->lexer.token.len, self->lexer.token.str);
      ij_lexer_restore(&self->lexer, snapshot);
      return false;
    }

    return true;
  }
}

bool ij_array_begin(IJ* self){
  if(self->serialize){
    if(ij_put_comma_check(self) == false) return false;
    const char* str = "[";
    if(ij_sb_append_cstr(&self->sb, str) == false){
      assert(false && "TODO: request more memory from user");
    }
    ij_sb_increase_indent(&self->sb);
    ij_sb_append_newline(&self->sb);
    self->first_element = true;
    return true;
  }else{
    self->first_element = true;
    return ij_lexer_expect(&self->lexer, IJ_TOKEN_SQUARE_OPEN);
  }
}

bool ij_array_end(IJ* self, int* count){
  if(self->serialize){
    self->first_element = false;
    if(count != NULL && (*count)-1 > 0){
      (*count)--;
      return false;
    }else{
      ij_sb_decrease_indent(&self->sb);
      ij_sb_append_newline(&self->sb);
      if(ij_sb_append_cstr(&self->sb, "]") == false){
        assert(false && "TODO: request more memory from user");
      }
      return true;
    }
  }else{
    if(ij_lexer_expect(&self->lexer, IJ_TOKEN_SQUARE_CLOSE)){
      return true;
    }else{
      if(ij_error(self) == IJ_E_OK){
        if(count != NULL) (*count)++;
        return false;
      }else{
        // return true on error
        return true;
      }
    }
  }
}

bool ij_write_string(IJ* self, const char* str){
  if(ij_put_comma_check(self) == false) return false;
  if(ij_sb_append_cstr(&self->sb, "\"") == false) return false;
  if(ij_sb_append_cstr(&self->sb, str) == false) return false;
  if(ij_sb_append_cstr(&self->sb, "\"") == false) return false;
  return true;
}

bool ij_read_string(IJ* self, const char** str){
  if(ij_consume_comma_check(self) == false) return false;
  if(ij_lexer_expect(&self->lexer, IJ_TOKEN_STRING) == false) return false;
  if(str != NULL) *str = self->lexer.token.str;
  return true;
}

bool ij_string(IJ* self, const char** str){
  if(self->serialize){
    return ij_write_string(self, *str);
  }else{
    return ij_read_string(self, str);
  }
}

bool ij_write_number(IJ* self, double value){
  if(ij_put_comma_check(self) == false) return false;
  if(ij_sb_appendf(&self->sb, "%f", value) == false) return false;
  return true;
}

bool ij_read_number(IJ* self, double* value){
  if(ij_lexer_expect(&self->lexer, IJ_TOKEN_NUMBER) == false){
    return false;
  }

  if(value != NULL){
    char* endptr = NULL;
    *value = strtod(self->lexer.token.str, &endptr);
    if(endptr == self->lexer.token.str){
      IJ_LOG_ERROR("ij_number: failed to parse number: %.*s", 
          self->lexer.token.len, self->lexer.token.str);
      return false;
    }
  }
  return true;
}

bool ij_number(IJ* self, double* value){
  if(self->serialize){
    if(value == NULL){
      return false;
    }
    return ij_write_number(self, *value);
  }else{
    return ij_read_number(self, value);
  }
}

bool ij_write_bool(IJ* self, bool* value){
  if(ij_put_comma_check(self) == false) return false;
  if(ij_sb_appendf(&self->sb, "%s", *value ? "true" : "false") == false){
    return false;
  }
  return true;
}

bool ij_read_bool(IJ* self, bool* value){
  if(ij_lexer_next_is(&self->lexer, IJ_TOKEN_KW_TRUE)){
    *value = true;
    return true;
  }else if(ij_lexer_next_is(&self->lexer, IJ_TOKEN_KW_FALSE)){
    *value = false;
    return true;
  }else{
    return false;
  }
}

bool ij_bool(IJ* self, bool* value){
  if(self->serialize){
    return ij_write_bool(self, value);
  }else{
    return ij_read_bool(self, value);
  }
}

bool ij_null(IJ* self){
  if(self->serialize){
    if(ij_put_comma_check(self) == false) return false;
    if(ij_sb_append_cstr(&self->sb, "null") == false){
      return false;
    }
    return true;
  }else{
    return ij_lexer_expect(&self->lexer, IJ_TOKEN_KW_NULL);
  }
}

bool ij_any(IJ* self, IJ_Any* value){
  if(self->serialize){
    switch(value->type){
      case IJ_OBJ_BEGIN: return ij_obj_begin(self);
      case IJ_OBJ_END: return ij_obj_end(self);
      case IJ_ARRAY_BEGIN: return ij_array_begin(self);
      case IJ_ARRAY_END: return ij_array_end(self, value->as.ArrayCount);
      case IJ_STRING: return ij_string(self, &value->as.String);
      case IJ_NUMBER: return ij_number(self, &value->as.Number);
      case IJ_BOOL: return ij_bool(self, &value->as.Bool);
      case IJ_NULL: return ij_null(self);
    };
    return false;
  }else{
    return false;
  }
}
#endif // IJ_IMPLEMENTATION

#endif // IJ_H_
