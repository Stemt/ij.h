
#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"

bool is_letter(char c){
  return (c >= 'a' && c <= 'z') 
    || (c >= 'A' && c <= 'Z')
    || c == '_';
}

bool compile_tests(void){
  Nob_String_Builder in = {0};
  if(!nob_read_entire_file("test.c", &in)) return false;

  char* end = in.items + in.count;
  char* s = in.items;

  Nob_String_Builder out = {0};
  nob_sb_append_cstr(&out, "#include \"test.c\"\n");
  nob_sb_append_cstr(&out, "\n");
  nob_sb_append_cstr(&out, "int main(void){\n");

  const char* pattern = "utest_";

  while(s < end){
    if(strncmp(s, pattern, strlen(pattern)) == 0){
      char* pattern_start = s;
      while(is_letter(*s)) ++s; // take entire func name
      nob_sb_appendf(&out, "  %.*s();\n", (int)(s-pattern_start), pattern_start);
    }
    s++;
  }

  nob_sb_append_cstr(&out, "  fprintf(stderr, \"[\"GREEN\"ALL TESTS PASSED\"COLOR_RESET\"]\\n\\n\");\n");
  nob_sb_append_cstr(&out, "}\n");

  if(!nob_write_entire_file("test.main.c", out.items, out.count)) return false;

  return true;
}

int main(int argc, char** argv){
  NOB_GO_REBUILD_URSELF(argc, argv);

  if(!compile_tests()) return false;

  Cmd cmd = {0};

  nob_cc(&cmd);
  nob_cc_flags(&cmd);
  cmd_append(&cmd, "-g");
  nob_cc_inputs(&cmd, "test.main.c");
  nob_cc_output(&cmd, "./test");

  if(!cmd_run(&cmd)) return 1;

  cmd_append(&cmd, "./test");

  if(!cmd_run(&cmd)) return 1;

  return 0;
}
