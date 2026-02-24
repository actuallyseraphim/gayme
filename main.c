#include "RGFW.h"
#include "glad/glad.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

extern size_t Mx437_Sharp_PC3K_Alt_size;
extern uint64_t Mx437_Sharp_PC3K_Alt[];

extern const char* shaders_screen_fsh;
extern const char* shaders_screen_vsh;

#define window_width 800
#define window_height 600
#define text_screen_size window_width*window_height/64

uint32_t text_screen_data[text_screen_size] = {0};
const float screen_verts[] = {-1, -1, -1, 1, 1, 1, 1, 1, 1, -1, -1, -1};

unsigned int compile_shader(unsigned int shader_type, const char* shader_txt) {
  unsigned int shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &shader_txt, 0);
  glCompileShader(shader);

  int success;
  char info[4096];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 4096, NULL, info);
    fprintf(stderr, "Failed to compile shader\n%s", info);
    exit(1);
  }
  return shader;
}

unsigned int compile_shader_program(const char* fsh_txt, const char* vsh_txt) {
  unsigned int fsh = compile_shader(GL_FRAGMENT_SHADER, fsh_txt);
  unsigned int vsh = compile_shader(GL_VERTEX_SHADER, vsh_txt);
  unsigned int shader = glCreateProgram();
  glAttachShader(shader, fsh);
  glAttachShader(shader, vsh);
  glLinkProgram(shader);

  int success;
  char info[4096];
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if(!success) {
    glGetProgramInfoLog(shader, 4096, NULL, info);
    fprintf(stderr, "Failed to link shader:\n%s", info);
    exit(1);
  }
  glDeleteShader(fsh);
  glDeleteShader(vsh);
  return shader;
}

int main() {
  RGFW_windowFlags window_hint = 0;
  window_hint |= RGFW_windowCenter;
  window_hint |= RGFW_windowNoResize;
  window_hint |= RGFW_windowOpenGL;
    
  RGFW_window* win = RGFW_createWindow("gay-me", 0, 0, window_width, window_height, window_hint);
  if (!gladLoadGLLoader((GLADloadproc)RGFW_getProcAddress_OpenGL)) {
    return 1;
  }
  
  RGFW_event event;
  
  unsigned int text_screen_texture;
  glGenTextures(1, &text_screen_texture);
  glBindTexture(GL_TEXTURE_2D, text_screen_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  unsigned int screen_program = compile_shader_program(shaders_screen_fsh, shaders_screen_vsh);

  unsigned int font_ssbo;
  glGenBuffers(1, &font_ssbo);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, font_ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, Mx437_Sharp_PC3K_Alt_size, Mx437_Sharp_PC3K_Alt, GL_STATIC_DRAW);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, font_ssbo);

  const char* str = "Hello, World!";
  for (int i = 0; str[i] != 0; i++) {
    text_screen_data[i+2*800/8+2] = str[i];
  }
  
  while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
	while (RGFW_window_checkEvent(win, &event)) {}
    glViewport(0, 0, window_width, window_height);
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, window_width/8, window_height/8, 0, GL_RED_INTEGER,  GL_UNSIGNED_INT, text_screen_data);
    glUseProgram(screen_program);
    glBindTexture(GL_TEXTURE_2D, text_screen_texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    RGFW_window_swapBuffers_OpenGL(win);
  }
  
  RGFW_window_close(win);
  return 0;
}
