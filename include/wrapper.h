#include <stdlib.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#define min(a, b) a < b ? a : b
#define max(a, b) a > b ? a : b

int GAME_WIDTH = 1280;
int GAME_HEIGHT = 720;

int SPRITESHEET_SIZE = 64;

typedef GLuint glptr;

typedef struct {
    int x, y;
    int width, height;
} spritesheet_info;

typedef struct {
    float x, y;
} vec2;

typedef struct {
    vec2* points;
    size_t numPoints;
} polygon;

typedef struct {
    int x1, y1;
    int x2, y2;
    int x3, y3;
} tri;

typedef struct {
    int x1, y1;
    int x2, y2;
    int x3, y3;
    int x4, y4;
} quad;

typedef struct {
    glptr vao, vbo, ebo;
    polygon p;
} vertex_info;

typedef struct {
    glptr fbo;
    glptr rect_vao, rect_vbo;
    glptr color_tex;
    glptr shader_prog;
} framebuffer_info;

typedef struct {
    int width, height;
    glptr tex;
} texture;

void print_shader_info_log(glptr shader) {
    char log[1024];
    int exit_code = 0;

    glGetShaderiv(shader, GL_COMPILE_STATUS, (GLuint*)&exit_code);

    if (!exit_code) {
        glGetShaderInfoLog(shader, 1024, NULL, log);
        fprintf(stderr, "%s", log);
    }
}

void draw_vertex_info(vertex_info info) {
    glBindVertexArray(info.vao);
    glDrawElements(GL_TRIANGLES, (info.p.numPoints - 2) * 3, GL_UNSIGNED_INT, 0);
}

void draw_framebuffer_rect(framebuffer_info fb) {
    glUseProgram(fb.shader_prog);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(fb.rect_vao);
    glBindTexture(GL_TEXTURE_2D, fb.color_tex);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void delete_buffers(vertex_info info) {
    glDeleteVertexArrays(1, &info.vao);
    glDeleteBuffers(1, &info.vbo);
    glDeleteBuffers(1, &info.ebo);
}

texture create_texture(char* filepath, glptr prog) {
    int img_width, img_height, num_col_chan;
    unsigned char* img_bytes = stbi_load("src/images/xsheet.png", &img_width, &img_height, &num_col_chan, 0);

    glptr tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0); // let us use texture slot 0
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // make it repeat on x axis
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // make it repeat on y axis

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img_width, img_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_bytes);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(img_bytes);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLuint tex_uniform = glGetUniformLocation(prog, "tex0");
    glUniform1i(tex_uniform, 0);

    return (texture){
        .width=img_width,
        .height=img_height,
        .tex=tex
    };
}

polygon ttop(tri t) {
    polygon res = (polygon){
        .points=malloc(3 * sizeof(vec2)),
        .numPoints=3
    };

    res.points[0] = (vec2){t.x1, t.y1};
    res.points[1] = (vec2){t.x2, t.y2};
    res.points[2] = (vec2){t.x3, t.y3};

    return res;
}

polygon qtop(quad q) {
    polygon res = (polygon){
        .points=malloc(4 * sizeof(vec2)),
        .numPoints=4
    };

    res.points[0] = (vec2){q.x1, q.y1};
    res.points[1] = (vec2){q.x2, q.y2};
    res.points[2] = (vec2){q.x3, q.y3};
    res.points[3] = (vec2){q.x4, q.y4};

    return res;
}

float get_width(polygon p) {
    float min_x = 1.f / 0.f;
    float max_x = -1.f / 0.f;

    for (int i = 0; i < p.numPoints; i++) {
        min_x = min(min_x, p.points[i].x);
        max_x = max(max_x, p.points[i].x);
    }

    return max_x - min_x;
}

float get_height(polygon p) {
    float min_y = 1.f / 0.f;
    float max_y = -1.f / 0.f;

    for (int i = 0; i < p.numPoints; i++) {
        min_y = min(min_y, p.points[i].y);
        max_y = max(max_y, p.points[i].y);
    }

    return max_y - min_y;
}

glptr shader_program(const char* vertex_shader_source, const char* fragment_shader_source) {
    glptr vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (char const* const*)&vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    print_shader_info_log(vertex_shader);

    glptr fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (char const* const*)&fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    print_shader_info_log(fragment_shader);

    glptr res = glCreateProgram();
    glAttachShader(res, vertex_shader);
    glAttachShader(res, fragment_shader);

    glLinkProgram(res); // wrap up the shader program

    // free
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return res;
}

vertex_info polygon_vertex_info(polygon p, GLenum mode, int textured) {
    GLfloat verts[4*p.numPoints];
    GLuint indices[(p.numPoints-2)*3];

    float aspect_ratio, scale;

    if (textured) {
        aspect_ratio = get_width(p) / get_height(p);
        scale = get_height(p) / SPRITESHEET_SIZE;
    }

    for (int i = 0; i < p.numPoints; i++) {
        verts[4*i] = p.points[i].x / (0.5 * GAME_WIDTH) - 1;
        verts[4*i+1] = p.points[i].y / (0.5 * GAME_HEIGHT) - 1;

        // temp
        if (textured) {
            if (i == 0) {
                verts[4*i+2] = 0.0f;
                verts[4*i+3] = 1.0f * scale;
            } else if (i == 1) {
                verts[4*i+2] = 0.0f;
                verts[4*i+3] = 0.0f;
            } else if (i == 2) {
                verts[4*i+2] = 1.0f * aspect_ratio * scale;
                verts[4*i+3] = 0.0f;
            } else if (i == 3) {
                verts[4*i+2] = 1.0f * aspect_ratio * scale;
                verts[4*i+3] = 1.0f * scale;
            }
        }
    }

    for (int i = 0; i < p.numPoints - 2; i++) {
        indices[3*i] = 0;
        indices[3*i+1] = i + 1;
        indices[3*i+2] = i + 2;
    }

    glptr vao, vbo, ebo;

    glGenVertexArrays(1, &vao); // make 1 vertex array and put it in vao
    glGenBuffers(1, &vbo); // make 1 buffer and put it in vbo
    glGenBuffers(1, &ebo); // same thing here

    glBindVertexArray(vao); // make it the current buffer

    glBindBuffer(GL_ARRAY_BUFFER, vbo); // make it the current buffer
    // GL_STATIC_DRAW: verts don't change
    // GL_DYNAMIC_DRAW: verts change
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, mode); // store verts in current buffer

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, mode);

    // put data in layout (location = 0) in the vertex shader
    // stride is in BYTES
    // here we're putting 2 FLOAT numbers per point with a stride of 4 * sizeof(float)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); // enable it!
    // put data in layout (location = 1) in the vertex shader, this time for texture uv
    // here we're beginning at 2 * sizeof(float) putting 2 FLOAT numbers per point with a stride of 4 * sizeof(float)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return (vertex_info){
        .vao=vao,
        .vbo=vbo,
        .ebo=ebo,
        .p=p
    };
}

framebuffer_info create_framebuffer(int width, int height, glptr shader_prog) {
    glptr fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glptr framebuffer_col_tex;
    glGenTextures(1, &framebuffer_col_tex);
    glBindTexture(GL_TEXTURE_2D, framebuffer_col_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer_col_tex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (status != GL_FRAMEBUFFER_COMPLETE)
        fprintf(stderr, "framebuffer error %d", status);

    GLfloat rect_verts[] = {
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,

        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f
    };
    
    glptr rect_vao, rect_vbo;

    glGenVertexArrays(1, &rect_vao);
    glGenBuffers(1, &rect_vbo);
    glBindVertexArray(rect_vao);
    glBindBuffer(GL_ARRAY_BUFFER, rect_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect_verts), rect_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return (framebuffer_info){
        .fbo=fbo,
        .rect_vao=rect_vao,
        .rect_vbo=rect_vbo,
        .color_tex=framebuffer_col_tex,
        .shader_prog=shader_prog
    };
}