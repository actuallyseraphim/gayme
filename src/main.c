#include "RGFW.h"
#include "glad/glad.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern size_t Mx437_Sharp_PC3K_Alt_size;
extern uint64_t Mx437_Sharp_PC3K_Alt[];

extern const char* shaders_screen_text_fsh;
extern const char* shaders_screen_texture_fsh;
extern const char* shaders_screen_vsh;
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

#define text_width 80
#define text_height 45
#define text_ammount (text_width*text_height)

uint32_t text_data[text_ammount] = {0};

void render_text() {
  memset(text_data, 0, text_ammount*sizeof(uint32_t));
  const char* str = "Hello World!";
  for (int i = 0; str[i] != 0; i++) {
    text_data[(i)%text_ammount] = str[i];
  }
}

int main() {
  RGFW_windowFlags window_hint = 0;
  window_hint |= RGFW_windowCenter;
  window_hint |= RGFW_windowOpenGL;

  int window_width = 640;
  int window_height = 480;
  
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
  glBindTexture(GL_TEXTURE_2D, 0);

  unsigned int screen_fbo;
  glGenFramebuffers(1, &screen_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, screen_fbo);
  
  unsigned int screen_texture;
  glGenTextures(1, &screen_texture);
  glBindTexture(GL_TEXTURE_2D, screen_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);
  
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_texture, 0);

  unsigned int screen_rbo;
  glGenRenderbuffers(1, &screen_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, screen_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_width, window_height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screen_rbo);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  unsigned int screen_program = compile_shader_program(shaders_screen_text_fsh, shaders_screen_vsh);
  unsigned int screen_render_program = compile_shader_program(shaders_screen_texture_fsh, shaders_screen_vsh);

  unsigned int font_ssbo;
  glGenBuffers(1, &font_ssbo);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, font_ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, Mx437_Sharp_PC3K_Alt_size, Mx437_Sharp_PC3K_Alt, GL_STATIC_DRAW);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, font_ssbo);  
  while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
	while (RGFW_window_checkEvent(win, &event)) {}
    RGFW_window_getSize(win, &window_width, &window_height);
    
    render_text();
    
    glViewport(0, 0, text_width*8, text_height*8);
    glBindFramebuffer(GL_FRAMEBUFFER, screen_fbo);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindTexture(GL_TEXTURE_2D, text_screen_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, text_width, text_height, 0, GL_RED_INTEGER,  GL_UNSIGNED_INT, text_data);
    glUseProgram(screen_program);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    int m = 1;
    if (window_height/text_height < window_width/text_width) {
      m = window_height/text_height;
    } else {
      m = window_width/text_width;
    }
    
    int vw = text_width*m;
    int vh = text_height*m;
    glViewport(window_width/2-vw/2, window_height/2-vh/2, vw, vh);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(screen_render_program);
    glBindTexture(GL_TEXTURE_2D, screen_texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    RGFW_window_swapBuffers_OpenGL(win);
  }
  
  RGFW_window_close(win);
  return 0;
}
