#include <stdint.h>
#include <stdio.h>
#define nob_cc_flags(cmd) nob_cmd_append(cmd, "-Wall", "-Wextra", "-I./")

#define NOB_IMPLEMENTATION
#include "nob.h"

int just_build_obj(Cmd* cmd, const char* input, const char* output) {
  nob_cc(cmd);
  nob_cc_flags(cmd);
  cmd_append(cmd, "-c");
  nob_cc_output(cmd, temp_sprintf("build/%s", output));
  nob_cc_inputs(cmd, temp_sprintf("%s", input));
  if (!cmd_run(cmd)) return 1;
  return 0;
}

#include "font.c"
int generate_font() {
  FILE* in_fptr;
  FILE* out_fptr;
  if (!(in_fptr = fopen("Mx437_Sharp_PC3K_Alt.bdf", "r"))) {
    return 1;
  }
  if (!(out_fptr = fopen("build/Mx437_Sharp_PC3K_Alt.c", "w"))) {
    fclose(in_fptr);
    return 1;
  }
  bitmap_8x8font bmf = parse8x8Font(in_fptr);

  fprintf(out_fptr, "#include <stdint.h>\n");
  fprintf(out_fptr, "#include <stddef.h>\n");
  fprintf(out_fptr, "const size_t Mx437_Sharp_PC3K_Alt_size = %zu;\n", bmf.glyth_count*sizeof(uint64_t));
  fprintf(out_fptr, "const uint64_t Mx437_Sharp_PC3K_Alt[%zu] = {\n", bmf.glyth_count);
  for (int i = 0; i < bmf.glyth_count; i++) {
    fprintf(out_fptr, "  0x%016lX,\n", bmf.glyth_data[i]);
  }
  fprintf(out_fptr, "};\n");
  
  free(bmf.glyth_data);
  fclose(in_fptr);
  fclose(out_fptr);
  return 0;
}

int embed_file(Cmd* cmd, const char* name, const char* input, const char* output) {
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

int main(int argc, char **argv) {
  GO_REBUILD_URSELF_PLUS(argc, argv, "font.c");
  Cmd cmd = {0};

  if (!nob_mkdir_if_not_exists("build")) return 1;
  
  const char* rgfw_input[2] = {"RGFW.c", "RGFW.h"};
  if (needs_rebuild("build/RGFW.o", rgfw_input, 2)) {
    if (just_build_obj(&cmd, "RGFW.c", "RGFW.o")) return 1;
  }

  const char* glad_input[2] = {"glad.c", "glad/glad.h"};
  if (needs_rebuild("build/glad.o", glad_input, 2)) {
    if (just_build_obj(&cmd, "glad.c", "glad.o")) return 1;
  }

  if (generate_font()) return 1;
  if (just_build_obj(&cmd, "build/Mx437_Sharp_PC3K_Alt.c", "Mx437_Sharp_PC3K_Alt.o")) return 1;

  if (embed_file(&cmd, "shaders_screen_fsh", "shaders/screen.fsh", "build/screen_fsh.c")) return 1;
  if (just_build_obj(&cmd, "build/screen_fsh.c", "screen_fsh.o")) return 1;

  if (embed_file(&cmd, "shaders_screen_vsh", "shaders/screen.vsh", "build/screen_vsh.c")) return 1;
  if (just_build_obj(&cmd, "build/screen_vsh.c", "screen_vsh.o")) return 1;
  
  nob_cc(&cmd);
  nob_cc_flags(&cmd);
  nob_cc_output(&cmd, "main");
  nob_cc_inputs(&cmd, "main.c");
  nob_cc_inputs(&cmd, "build/RGFW.o");
  nob_cc_inputs(&cmd, "build/glad.o");
  nob_cc_inputs(&cmd, "build/Mx437_Sharp_PC3K_Alt.o");
  nob_cc_inputs(&cmd, "build/screen_fsh.o");
  nob_cc_inputs(&cmd, "build/screen_vsh.o");
  cmd_append(&cmd, "-lm", "-lX11", "-lXrandr", "-lGL");
  if (!cmd_run(&cmd)) return 1;
  return 0;
}
