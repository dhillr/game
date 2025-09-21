#include <stdio.h>
#include <string.h>

#include <wrapper.h>

char* keys;

int PLAYER_WIDTH = 8;
int PLAYER_HEIGHT = 8;

typedef struct {
    int x, y;
    int width, height;
} hitbox;

int collide(hitbox a, hitbox b) {
    return (
        a.x < b.x + b.width &&
        a.x + a.width > b.x &&
        a.y < b.y + b.height &&
        a.y + a.height > b.y
    );
}

int collides(hitbox h, hitbox* hitboxes, size_t num_hitboxes) {
    for (int i = 0; i < num_hitboxes; i++) {
        if (collide(h, hitboxes[i]))
            return 1;
    }

    return 0;
}

char* load_shader(char* filepath) {
    FILE* f = fopen(filepath, "rb");

    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    size_t file_len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = malloc(file_len + 1);

    fread(buf, 1, file_len, f);
    buf[file_len] = '\0';

    fclose(f);

    return buf;
}

void on_key_event(GLFWwindow* window, int key, int scancode, int action, int mod_keys) {
    if (action == GLFW_PRESS) {
        keys[key] = 1;
    }

    if (action == GLFW_RELEASE) {
        keys[key] = 0;
    }
}

void update_player(int* old_x, int* old_y, float* vx, float* vy, uint32_t* fall_time, double dt, hitbox* hitboxes, size_t num_hitboxes) {
    float x = *old_x;
    float y = *old_y;

    (*fall_time)++;

    if (keys[GLFW_KEY_A])
        *vx -= 0.125;
    
    if (keys[GLFW_KEY_D])
        *vx += 0.125;

    if (keys[GLFW_KEY_W] && *fall_time <= 2)
        *vy = 2.5;

    *vx *= 0.95;
    *vy -= 0.125;
    
    if (y < 0) {
        y = 0;
        *vy = 0;
        *fall_time = 0;
    }

    float steps = abs(*vx) + abs(*vy);

    float px, py;

    for (int i = 0; i < steps; i++) {
        px = x;
        py = y;

        x += *vx / steps;

        if (collides((hitbox){(int)(x + 0.5), (int)(y + 0.5), PLAYER_WIDTH, PLAYER_HEIGHT}, hitboxes, num_hitboxes)) {
            x = px;
            *vx = 0;
        }

        y += *vy / steps;

        if (collides((hitbox){(int)(x + 0.5), (int)(y + 0.5), PLAYER_WIDTH, PLAYER_HEIGHT}, hitboxes, num_hitboxes)) {
            y = py;

            if (*vy < 0) 
                *fall_time = 0;

            *vy = 0;
        }
    }


    *old_x = (int)(x + 0.5);
    *old_y = (int)(y + 0.5);
}

int main() {
    const char* tri_vert_shader = load_shader("src/shaders/vert_basic.glsl"); 
    const char* tri_frag_shader = load_shader("src/shaders/frag_basic.glsl");
    const char* framebuffer_vert_shader = load_shader("src/shaders/vert_framebuffer.glsl"); 
    const char* framebuffer_frag_shader = load_shader("src/shaders/frag_framebuffer.glsl");
    const char* player_frag_shader = load_shader("src/shaders/frag_player.glsl");

    keys = malloc(256);

    for (int i = 0; i < 256; i++) {
        keys[i] = 0;
    }

    GLFWwindow* window;

    if (!glfwInit()) return 1;

    double current_time = glfwGetTime();
    double prev_time = current_time;

    double delta = 0;
    double fps = 0;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(1280, 720, "gaem1!1 ! ! ! !1 1!1 !!!", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwGetWindowSize(window, &GAME_WIDTH, &GAME_HEIGHT);

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, on_key_event);
    // glfwSwapInterval(0); // to unlock fps

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return 1;

    glViewport(0, 0, 1280, 720);

    glptr prog = shader_program(tri_vert_shader, tri_frag_shader);
    glptr player_prog = shader_program(tri_vert_shader, player_frag_shader);
    glptr framebuffer_prog = shader_program(framebuffer_vert_shader, framebuffer_frag_shader);

    tri t = {0, 0, GAME_WIDTH, 0, 0, 100};
    quad player_quad = {0, 300, 100, 300, 150, 400, 50, 400};

    vertex_info info = polygon_vertex_info(qtop(player_quad), GL_DYNAMIC_DRAW, 0);

    texture tex0 = create_texture("src/images/xsheet.png", prog);

    GLuint tex_uniform = glGetUniformLocation(prog, "tex0");
    glUniform1i(tex_uniform, 0);

    framebuffer_info fb = create_framebuffer(320, 180, framebuffer_prog);

    int player_x = 0;
    int player_y = 0;

    int prev_x, prev_y;

    float player_vx = 0;
    float player_vy = 0;

    uint32_t fall_time = 0;

    size_t num = 5;
    hitbox* hitboxes = malloc(num * sizeof(hitbox));

    hitboxes[0] = (hitbox){12, 0, 12, 12};
    hitboxes[1] = (hitbox){37, 6, 48, 12};
    hitboxes[2] = (hitbox){100, 0, 16, 16};
    hitboxes[3] = (hitbox){150, 16, 16, 16};
    hitboxes[4] = (hitbox){800, 400, 50, 50};

    spritesheet_info* hitbox_ss_info = malloc(num * sizeof(spritesheet_info));

    hitbox_ss_info[0] = (spritesheet_info){0, 0, 16, 16};
    hitbox_ss_info[1] = (spritesheet_info){16, 0, 20, 16};
    hitbox_ss_info[2] = (spritesheet_info){0, 16, 16, 16};
    hitbox_ss_info[3] = (spritesheet_info){0, 16, 16, 16};
    hitbox_ss_info[4] = (spritesheet_info){0, 0, 16, 16};

    vertex_info* hitbox_info = malloc(num * sizeof(vertex_info));

    spritesheet_info s_info = {0, 0, 2, 16};
    
    for (int i = 0; i < num; i++) {
        hitbox h = hitboxes[i];

        polygon hitbox_p = qtop((quad){
            h.x, h.y,
            h.x, h.y + h.height,
            h.x + h.width, h.y + h.height,
            h.x + h.width, h.y
        });

        hitbox_info[i] = polygon_vertex_info(hitbox_p, GL_STATIC_DRAW, 1);
    }

    while (!glfwWindowShouldClose(window)) {
        current_time = glfwGetTime();
        delta = current_time - prev_time;
        fps = 1.f / delta;

        prev_x = player_x;
        prev_y = player_y;
        
        glfwGetWindowSize(window, &GAME_WIDTH, &GAME_HEIGHT);

        glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
        glClearColor(0.3f, 0.9f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(player_prog);

        glBindTexture(GL_TEXTURE_2D, tex0.tex);

        update_player(&player_x, &player_y, &player_vx, &player_vy, &fall_time, 120 * delta, hitboxes, num);
        // printf("%f\n", fps);

        int diff_x = player_x - prev_x;
        int diff_y = player_y - prev_y;

        player_quad.x1 = player_x;
        player_quad.y1 = player_y;
        player_quad.x2 = player_x + 8;
        player_quad.y2 = player_y;
        player_quad.x3 = prev_x + 8 + 2 * diff_x;
        player_quad.y3 = player_y + 8;
        player_quad.x4 = prev_x + 2 * diff_x;
        player_quad.y4 = player_y + 8;

        if ((int)(player_vx + 0.5) > 0) {
            player_quad.y1 = prev_y;
            player_quad.y4 = prev_y + 8;
        } else if ((int)(player_vx + 0.5) < 0) {
            player_quad.y2 = prev_y;
            player_quad.y3 = prev_y + 8;
        } else {
            player_quad.y1 = prev_y;
            player_quad.y2 = prev_y;
        }

        info = polygon_vertex_info(qtop(player_quad), GL_DYNAMIC_DRAW, 0);

        draw_vertex_info(info);

        glUseProgram(prog);

        GLuint offset_mod_uniform = glGetUniformLocation(prog, "offset_mod");
        
        for (int i = 0; i < num; i++) {
            spritesheet_info ss_info = hitbox_ss_info[i];
            float ss_size = SPRITESHEET_SIZE;

            glUniform4f(offset_mod_uniform, (float)ss_info.x / ss_size, (float)ss_info.y / ss_size, (float)ss_info.width / ss_size, (float)ss_info.height / ss_size);
            draw_vertex_info(hitbox_info[i]);
        }

        draw_framebuffer_rect(fb);
        glfwSwapBuffers(window);
        glfwPollEvents();
        prev_time = current_time;
    }

    delete_buffers(info);
    glDeleteProgram(prog);
    glDeleteTextures(1, &tex0.tex);

    free((void*)tri_vert_shader);
    free((void*)tri_frag_shader);
    free(keys);
    free(hitboxes);
    free(hitbox_info);
    free(hitbox_ss_info);

    glfwTerminate();
    return 0;
}