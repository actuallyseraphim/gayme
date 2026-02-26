#include <stdint.h>
#include <stdio.h>

#define BUILD_DIR "build/"
#define SRC_DIR "src/"
#define LIB_DIR "lib/"
#define INCLUDE_DIR "include/"
#define RESOURCES_DIR "resources/"

#define nob_cc_flags(cmd) nob_cmd_append(cmd, "-Wall", "-Wextra", "-I./"INCLUDE_DIR, "-I./"LIB_DIR)

#define NOB_IMPLEMENTATION
#include "lib/nob.h"

int just_build_obj(Cmd* cmd, const char* input, const char* output) {
  nob_cc(cmd);
  nob_cc_flags(cmd);
  cmd_append(cmd, "-c");
  nob_cc_output(cmd, temp_sprintf("%s", output));
  nob_cc_inputs(cmd, temp_sprintf("%s", input));
  if (!cmd_run(cmd)) return 1;
  return 0;
}

#include "font.c"
int generate_font(const char* font, const char* input, const char* output) {
  FILE* in_fptr;
  FILE* out_fptr;
  if (!(in_fptr = fopen(input, "r"))) {
    return 1;
  }
  if (!(out_fptr = fopen(output, "w"))) {
    fclose(in_fptr);
    return 1;
  }
  bitmap_8x8font bmf = parse8x8Font(in_fptr);

  fprintf(out_fptr, "#include <stdint.h>\n");
  fprintf(out_fptr, "#include <stddef.h>\n");
  fprintf(out_fptr, "const size_t %s_size = %zu;\n", font, bmf.glyth_count*sizeof(uint64_t));
  fprintf(out_fptr, "const uint64_t %s[%zu] = {\n", font, bmf.glyth_count);
  for (int i = 0; i < bmf.glyth_count; i++) {
    fprintf(out_fptr, "  0x%016lX,\n", bmf.glyth_data[i]);
  }
  fprintf(out_fptr, "};\n");
  
  free(bmf.glyth_data);
  fclose(in_fptr);
  fclose(out_fptr);
  return 0;
}

int embed_txt_file(Cmd* cmd, const char* name, const char* input, const char* output) {
  FILE* in_fptr;
  FILE* out_fptr;
  if (!(in_fptr = fopen(input, "r"))) {
    return 1;
  }
  if (!(out_fptr = fopen(output, "w"))) {
    fclose(in_fptr);
    return 1;
  }

  fprintf(out_fptr, "const char* %s = \"", name);
  char c = 0;
  while (fread(&c, 1, 1, in_fptr)) {
    fprintf(out_fptr, "\\x%X", c);
  }
  fprintf(out_fptr, "\";");
  
  fclose(in_fptr);
  fclose(out_fptr);
  return 0;
}

typedef struct {
  Cmd* cmd;
  Nob_File_Paths object_files;
} build_t;

void add_obj_to_build(build_t* build, const char* obj) {
  da_append(&build->object_files, obj);
}

char* cify_path(const char* path) {
  char* out = temp_strdup(path);
  for (int i = 0; out[i] != 0; i++) {
    if (out[i] == '/' || out[i] == '.' || out[i] == ' ') {
      out[i] = '_';
    }
  }
  return out;
};

int add_lib_to_build(build_t* build, const char* cfile, const char* header) {
  const char* input[2] = {
    temp_sprintf("%s", cfile),
    temp_sprintf("%s", header),
  };
  char* output = temp_sprintf(BUILD_DIR"%s.o", cify_path(cfile));
  if (needs_rebuild(output, input, 2)) {
    if (just_build_obj(build->cmd, input[0], output)) return 1;
  }
  add_obj_to_build(build, output);
  return 0;
}

int add_font_to_build(build_t* build) {
  const char* font = "Mx437_Sharp_PC3K_Alt";
  char* input_bdf = temp_sprintf(RESOURCES_DIR"%s.bdf", font);
  char* output_c = temp_sprintf(BUILD_DIR"%s.c", font);
  char* output_o = temp_sprintf(BUILD_DIR"%s.o", font);

  if (generate_font(font, input_bdf, output_c)) return 1;
  if (just_build_obj(build->cmd, output_c, output_o)) return 1;
  add_obj_to_build(build, output_o);
  return 0;
}

int add_text_to_build(build_t* build, const char* file) {
  char* symbol = cify_path(file);
  char* input_text = temp_sprintf(RESOURCES_DIR"%s", file);
  char* output_c = temp_sprintf(BUILD_DIR"%s.c", symbol);
  char* output_o = temp_sprintf(BUILD_DIR"%s.o", symbol);

  if (embed_txt_file(build->cmd, symbol, input_text, output_c)) return 1;
  if (just_build_obj(build->cmd, output_c, output_o)) return 1;

  add_obj_to_build(build, output_o);
  return 0;
}

int main(int argc, char **argv) {
  GO_REBUILD_URSELF_PLUS(argc, argv, "font.c");
  Cmd cmd = {0};
  build_t build = {
    .cmd = &cmd,
    .object_files = {0},
  };

  if (!nob_mkdir_if_not_exists("build")) return 1;

  add_lib_to_build(&build, "lib/RGFW.c", "lib/RGFW.h");
  add_lib_to_build(&build, "lib/glad.c", "lib/glad/glad.h");
  add_font_to_build(&build);

  add_text_to_build(&build, "shaders/screen_text.fsh");
  add_text_to_build(&build, "shaders/screen_texture.fsh");
  add_text_to_build(&build, "shaders/screen.vsh");
  
  nob_cc(&cmd);
  nob_cc_flags(&cmd);
  nob_cc_output(&cmd, "main");
  nob_cc_inputs(&cmd, SRC_DIR"main.c");
  da_foreach(const char*, file, &build.object_files) {
    nob_cc_inputs(&cmd, *file);
  }
  cmd_append(&cmd, "-lm", "-lX11", "-lXrandr", "-lGL");
  if (!cmd_run(&cmd)) return 1;
  return 0;
}
