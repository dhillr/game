#include <stdio.h>
#include <string.h>
#include <math.h>

#include <wrapper.h>
#include <particles.h>
#include <point_alloc.h>

#define PI 3.141592653589793238462643383279502884197169399375105820974944592

#define LEFT    0b10000000
#define RIGHT   0b01000000
#define JUMP    0b00100000
#define ATTACK  0b00010000

#define PLAYER_WIDTH 8
#define PLAYER_HEIGHT 8

#define PLAYER_DAMAGE_COOLDOWN 500
#define ENEMY_DAMAGE_COOLDOWN 300

#define PLAYER_SPEED 0.125f
#define ENEMY_SPEED 0.07f

char* keys;
char* mouse;
float ss_size;

typedef unsigned char action;

typedef struct {
    int x, y;
    int width, height;
    int is_ground;
} hitbox;

typedef struct {
    int x, y;
    int width, height;
    float rotation;
    spritesheet_info ss_info;
} sprite;

typedef struct {
    int x, y;
    int prev_x, prev_y;
    float vx, vy;
    uint32_t fall_time;
    float damage_time;
    int health;
    action event;
} enemy;

typedef struct {
    hitbox* hitboxes;
    spritesheet_info* hitbox_ss_info;
    sprite* sprites;
    enemy* enemies;
    size_t hitbox_num, sprite_num, enemy_num;
} level;

int collide(hitbox a, hitbox b) {
    return (
        a.x < b.x + b.width &&
        a.x + a.width > b.x &&
        a.y < b.y + b.height &&
        a.y + a.height > b.y
    );
}

int touch(hitbox a, hitbox b) {
    return (
        a.x <= b.x + b.width &&
        a.x + a.width >= b.x &&
        a.y <= b.y + b.height &&
        a.y + a.height >= b.y
    );
}

int collides(hitbox h, hitbox* hitboxes, size_t num_hitboxes) {
    for (int i = 0; i < num_hitboxes; i++) {
        if (collide(h, hitboxes[i]))
            return 1;
    }

    return 0;
}

int touching(hitbox h, hitbox* hitboxes, size_t num_hitboxes) {
    for (int i = 0; i < num_hitboxes; i++) {
        if (touch(h, hitboxes[i]) && !hitboxes[i].is_ground)
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

    char* buf = calloc(file_len + 1, 1);

    fread(buf, 1, file_len, f);
    buf[file_len] = '\0';

    fclose(f);

    return buf;
}

void set_attr(float attributes[], void* item, int type) {
    switch (type) {
        case 0:
            ((hitbox*)item)->x = attributes[0];
            ((hitbox*)item)->y = attributes[1];
            ((hitbox*)item)->width = attributes[2];
            ((hitbox*)item)->height = attributes[3];
            ((hitbox*)item)->is_ground = attributes[4];
            break;
        case 1:
            ((spritesheet_info*)item)->x = attributes[0];
            ((spritesheet_info*)item)->y = attributes[1];
            ((spritesheet_info*)item)->width = attributes[2];
            ((spritesheet_info*)item)->height = attributes[3];
            break;
        case 2:
            ((sprite*)item)->x = attributes[0];
            ((sprite*)item)->y = attributes[1];
            ((sprite*)item)->width = attributes[2];
            ((sprite*)item)->height = attributes[3];
            ((sprite*)item)->rotation = attributes[4];
            (&((sprite*)item)->ss_info)->x = attributes[5];
            (&((sprite*)item)->ss_info)->y = attributes[6];
            (&((sprite*)item)->ss_info)->width = attributes[7];
            (&((sprite*)item)->ss_info)->height = attributes[8];
            break;
        case 3:
            ((enemy*)item)->x = attributes[0];
            ((enemy*)item)->y = attributes[1];
            ((enemy*)item)->prev_x = attributes[2];
            ((enemy*)item)->prev_y = attributes[3];
            ((enemy*)item)->vx = attributes[4];
            ((enemy*)item)->vy = attributes[5];
            ((enemy*)item)->fall_time = attributes[6];
            ((enemy*)item)->damage_time = attributes[7];
            ((enemy*)item)->health = attributes[8];
            ((enemy*)item)->event = attributes[9];
            break;
    }
}

level load_level(char* filepath) {
    level res = {malloc(sizeof(hitbox)), malloc(sizeof(spritesheet_info)), malloc(sizeof(sprite)), malloc(sizeof(enemy)), 1, 1, 1};
    int elem = 0, prop = 0, i = 0;
    int max_props;

    FILE* f = fopen(filepath, "rb");
    char c;

    char item[256];
    int item_char = 0;

    memset(item, 0, 256);

    float props[10];

    while ((c = fgetc(f)) != EOF) {
        max_props = (int[]){5, 4, 9, 10}[elem];

        if (c == ',' || c == '\n') {
            if (prop >= max_props) {
                if (elem == 0) {
                    set_attr(props, res.hitboxes + i, elem);
                } else if (elem == 1) {
                    set_attr(props, res.hitbox_ss_info + i, elem);
                    
                } else if (elem == 2) {
                    set_attr(props, res.sprites + i, elem);
                } else if (elem == 3) {
                    set_attr(props, res.enemies + i, elem);
                }

                

                prop = 0;
                memset(props, 0, 10 * sizeof(float));
                i++;

                if (elem == 0) {
                    res.hitbox_num++;
                    res.hitboxes = realloc(res.hitboxes, (i + 1) * sizeof(hitbox));
                } else if (elem == 1) {
                    res.hitbox_ss_info = realloc(res.hitbox_ss_info, (i + 1) * sizeof(spritesheet_info));
                } else if (elem == 2) {
                    res.sprite_num++;
                    res.sprites = realloc(res.sprites, (i + 1) * sizeof(sprite));
                } else if (elem == 3) {
                    res.enemy_num++;
                    res.enemies = realloc(res.enemies, (i + 1) * sizeof(enemy));
                }
            }
            
            props[prop] = atof(item);

            memset(item, 0, 256);
            item_char = 0;
            prop++;
        } else {
            item[item_char] = c;
            item_char++;
        }

        if (c == '\n') {
            prop = 0;
            i = 0;
            elem++;
        }

        // printf("%c", c);
    }

    return res;
}

void setOffsetModUniform(glptr uniform, spritesheet_info ss_info) {
    glUniform4f(uniform, (float)ss_info.x / ss_size, (float)ss_info.y / ss_size, (float)ss_info.width / ss_size, (float)ss_info.height / ss_size);
}

quad rect(int x, int y, int width, int height) {
    return (quad){
        x, y,
        x, y + height,
        x + width, y + height,
        x + width, y
    };
}

void print_bin(char bin) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", (bin & (1 << i)) >> i);
    }
    printf("\n");
}

void bind_action_bit(action* event, unsigned char bit, char value) {
    if (value)
        *event |= bit;
    else
        *event &= ~bit;
}

int chance(float rate) {
    return rand() < rate * 32767;
}

void update_player_quad(quad* q, int player_x, int player_y, int prev_x, int prev_y, float player_vx) {
    int diff_x = player_x - prev_x;

    (*q).x1 = player_x;
    (*q).y1 = player_y;
    (*q).x2 = player_x + 8;
    (*q).y2 = player_y;
    (*q).x3 = player_x + diff_x + 8;
    (*q).y3 = player_y + 8;
    (*q).x4 = player_x + diff_x;
    (*q).y4 = player_y + 8;

    if ((int)(player_vx + 0.5) > 0) {
        (*q).y1 = prev_y;
        (*q).y4 = prev_y + 8;
    } else if ((int)(player_vx + 0.5) < 0) {
        (*q).y2 = prev_y;
        (*q).y3 = prev_y + 8;
    } else {
        (*q).y1 = prev_y;
        (*q).y2 = prev_y;
    }
}

void on_key_event(GLFWwindow* window, int key, int scancode, int action, int mod_keys) {
    if (action == GLFW_PRESS)
        keys[key] = 1;

    if (action == GLFW_RELEASE)
        keys[key] = 0;
}

void on_mouse_event(GLFWwindow* window, int button, int action, int mod_keys) {
    if (action == GLFW_PRESS)
        mouse[button] = 1;

    if (action == GLFW_RELEASE)
        mouse[button] = 0;
}

void update_player(int* old_x, int* old_y, float* vx, float* vy, uint32_t* fall_time, float* damage_time, double dt, hitbox* hitboxes, size_t num_hitboxes, action event, float speed) {
    float x = *old_x;
    float y = *old_y;

    (*fall_time)++;

    if (event & LEFT)
        *vx -= speed;
    
    if (event & RIGHT)
        *vx += speed;

    if (event & JUMP && *fall_time <= 2)
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

    *damage_time += 1000.f / 120.f * (float)dt;
}

void walk(enemy* e, hitbox* hitboxes, size_t hitbox_num, double dt) {
    if (chance(.002f))
        e->event |= JUMP;

    if (chance(.1f))
        e->event &= ~JUMP;

    if (touching((hitbox){e->x, e->y, PLAYER_WIDTH, PLAYER_HEIGHT}, hitboxes, hitbox_num)) {
        e->y++;

        if (touching((hitbox){e->x, e->y, PLAYER_WIDTH, PLAYER_HEIGHT}, hitboxes, hitbox_num)) {
            unsigned char dir;
            unsigned char inv_dir;

            if (e->event & RIGHT) {
                dir = RIGHT;
                inv_dir = LEFT;
            }

            if (e->event & LEFT) {
                dir = LEFT;
                inv_dir = RIGHT;
            }

            e->event &= ~dir;
            e->event |= inv_dir;

            e->vx = 0;

            do {
                if (inv_dir == LEFT)
                    e->x--;
                else
                    e->x++;
                
                update_player(&(e->x), &(e->y), &(e->vx), &(e->vy), &(e->fall_time), &(e->damage_time), 120 * dt, hitboxes, hitbox_num, e->event, ENEMY_SPEED);
            } while (touching((hitbox){e->x, e->y, PLAYER_WIDTH, PLAYER_HEIGHT}, hitboxes, hitbox_num));
        }

        e->y--;
    }

    if (e->x < 10) {
        e->event &= ~LEFT;
        e->event |= RIGHT;
        e->x++;
    }
}

void attack_response(enemy* e, action player_event, hitbox weapon_hitbox, char attack_dir, particle_env particles) {
    if (collide(weapon_hitbox, (hitbox){e->x, e->y, PLAYER_WIDTH, PLAYER_HEIGHT}) && e->damage_time >= ENEMY_DAMAGE_COOLDOWN && player_event & ATTACK) {
        e->vy = 2.f;

        if (attack_dir == RIGHT)
            e->vx = 2.f;
        else
            e->vx = -2.f;

        e->health--;
        e->damage_time = 0.f;

        for (int i = 0; i < 10; i++) {
            float r = (float)rand() / 16384.f - 1.f;

            particle p = square_particle(e->x, e->y, 4, (vec3){0.f, 0.f, 0.f}, 500);
            p.vx = 2.f * r;
            p.vy = (float)rand() / 16384.f + 1.f;

            add_particle(particles.particles, p);
        }
    }

    if (e->damage_time < ENEMY_DAMAGE_COOLDOWN) {
        e->event = 0;
    } else {
        if (e->event == 0)
            e->event |= RIGHT;
    }
}

void remove_enemy(enemy* enemies, size_t enemy_num, uint32_t index) {
    int first_block_size = index;
    int last_block_size = enemy_num - index - 1;

    enemy* first_block = malloc(first_block_size * sizeof(enemy));
    enemy* last_block = malloc(last_block_size * sizeof(enemy));

    memcpy(first_block, enemies, first_block_size * sizeof(enemy));
    memcpy(last_block, enemies + index + 1, last_block_size * sizeof(enemy));

    memset(enemies, 0, enemy_num * sizeof(enemy));

    memcpy(enemies, first_block, first_block_size * sizeof(enemy));
    memcpy(enemies + index, last_block, last_block_size * sizeof(enemy));

    free(first_block);
    free(last_block);
}

int main() {
    const char* tri_vert_shader = load_shader("src/shaders/vert_basic.glsl"); 
    const char* tri_frag_shader = load_shader("src/shaders/frag_basic.glsl");
    const char* framebuffer_vert_shader = load_shader("src/shaders/vert_framebuffer.glsl"); 
    const char* framebuffer_frag_shader = load_shader("src/shaders/frag_framebuffer.glsl");
    const char* player_frag_shader = load_shader("src/shaders/frag_player.glsl");
    const char* ui_frag_shader = load_shader("src/shaders/frag_ui.glsl");

    keys = malloc(256);
    mouse = malloc(8);
    ss_size = SPRITESHEET_SIZE;

    POINT_ALLOC_DEFAULT = (point_alloc){calloc(BLOCK_SIZE, sizeof(vec2*)), 0, 1};

    for (int i = 0; i < 256; i++) {
        keys[i] = 0;
    }

    for (int i = 0; i < 8; i++) {
        mouse[i] = 0;
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

    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(1280, 720, "gaem1!1 ! ! ! !1 1!1 !!!", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwGetWindowSize(window, &GAME_WIDTH, &GAME_HEIGHT);

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, on_key_event);
    glfwSetMouseButtonCallback(window, on_mouse_event);
    // glfwSwapInterval(0); // to unlock fps

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return 1;

    glViewport(0, 0, 1280, 720);

    glptr prog = shader_program(tri_vert_shader, tri_frag_shader);
    glptr player_prog = shader_program(tri_vert_shader, player_frag_shader);
    glptr framebuffer_prog = shader_program(framebuffer_vert_shader, framebuffer_frag_shader);
    glptr ui_prog = shader_program(framebuffer_vert_shader, ui_frag_shader);

    tri t = {0, 0, GAME_WIDTH, 0, 0, 100};
    quad player_quad;
    polygon player_p = qtop(rect(0, 0, 0, 0));

    vertex_info info = polygon_vertex_info(qtop(player_quad), GL_DYNAMIC_DRAW, 0, 0);

    texture tex0 = create_texture("src/images/xsheet.png", prog);

    GLuint tex_uniform = glGetUniformLocation(prog, "tex0");
    glUniform1i(tex_uniform, 0);

    framebuffer_info fb = create_framebuffer(320, 180, framebuffer_prog);

    int camera_x = 0;
    int camera_y = 0;

    int player_x = 0;
    int player_y = 72;

    int prev_x, prev_y;

    float player_vx = 0;
    float player_vy = 0;

    uint32_t fall_time = 0;
    float damage_time = PLAYER_DAMAGE_COOLDOWN;

    int player_health = 20;

    action player_action = 0;

    size_t hitbox_num = 9;
    size_t sprite_num = 4;
    size_t enemy_num = 2;

    hitbox* hitboxes = malloc(hitbox_num * sizeof(hitbox));
    sprite* sprites = malloc(sprite_num * sizeof(sprite));

    particle_list particle_l = PARTICLE_LIST_EMPTY;
    particle_env particles = {&particle_l, .1f};

    sprite weapon_sprite = {0, 0, 16, 16, 0, {0, 48, 16, 16}};
    float weapon_sprite_rot_target = 0;
    float weapon_sprite_xoff_target = 0;
    float weapon_sprite_xoff = 0;

    enemy* enemies = malloc(enemy_num * sizeof(enemy));
    enemies[0] = (enemy){192, 72, 192, 72, 0.f, 0.f, 0u, ENEMY_DAMAGE_COOLDOWN, 10, RIGHT};
    enemies[1] = (enemy){300, 72, 300, 72, 0.f, 0.f, 0u, ENEMY_DAMAGE_COOLDOWN, 10, LEFT};

    hitboxes[0] = (hitbox){0, 60, 600, 12, 1};
    hitboxes[1] = (hitbox){0, 52, 600, 16, 1};
    hitboxes[2] = (hitbox){0, 0, 600, 52, 1};
    hitboxes[3] = (hitbox){128, 72, 16, 16};
    hitboxes[4] = (hitbox){192, 88, 16, 16};
    hitboxes[5] = (hitbox){256, 72, 16, 16};
    hitboxes[6] = (hitbox){620, 52, 100, 12, 1};
    hitboxes[7] = (hitbox){620, 44, 100, 16, 1};
    hitboxes[8] = (hitbox){620, -8, 100, 52, 1};

    sprites[0] = (sprite){256, 72, 344, 32, 0, {32, 16, 16, 32}};
    sprites[1] = (sprite){240, 88, 16, 16, 0, {16, 16, 16, 16}};
    sprites[2] = (sprite){240, 72, 16, 16, 0, {32, 32, 16, 16}};
    sprites[3] = (sprite){224, 72, 16, 16, 0, {16, 32, 16, 16}};

    spritesheet_info* hitbox_ss_info = malloc(hitbox_num * sizeof(spritesheet_info));

    hitbox_ss_info[0] = (spritesheet_info){0, 0, 16, 16};
    hitbox_ss_info[1] = (spritesheet_info){16, 0, 20, 16};
    hitbox_ss_info[2] = (spritesheet_info){40, 0, 20, 16};
    hitbox_ss_info[3] = (spritesheet_info){0, 16, 16, 16};
    hitbox_ss_info[4] = (spritesheet_info){0, 16, 16, 16};
    hitbox_ss_info[5] = (spritesheet_info){0, 16, 16, 16};
    hitbox_ss_info[6] = (spritesheet_info){0, 0, 16, 16};
    hitbox_ss_info[7] = (spritesheet_info){16, 0, 20, 16};
    hitbox_ss_info[8] = (spritesheet_info){40, 0, 20, 16};

    vertex_info* hitbox_info = malloc(hitbox_num * sizeof(vertex_info));
    vertex_info* sprite_info = malloc(sprite_num * sizeof(vertex_info));
    
    for (int i = 0; i < hitbox_num; i++) {
        hitbox h = hitboxes[i];

        polygon hitbox_p = qtop(rect(h.x, h.y, h.width, h.height));

        hitbox_info[i] = polygon_vertex_info(hitbox_p, GL_STATIC_DRAW, 1, 0);

        // free(hitbox_p.points);
    }

    for (int i = 0; i < sprite_num; i++) {
        sprite s = sprites[i];

        polygon sprite_p = qtop(rect(s.x, s.y, s.width, s.height));

        sprite_info[i] = polygon_vertex_info(sprite_p, GL_STATIC_DRAW, 1, 0);

        // free(sprite_p.points);
    }

    level lvl = load_level("src/levels/1.lvl");

    hitboxes = lvl.hitboxes;
    sprites = lvl.sprites;
    enemies = lvl.enemies;

    polygon weapon_sprite_p;
    polygon health_bar_p = qtop(rect(3, 172, 42, 5));
    polygon health_amount_p = qtop(rect(30, 30, 20, 10));

    vertex_info weapon_sprite_info;

    glptr cam_offset_uniform = glGetUniformLocation(prog, "camera_offset");
    glptr cam_offset_uniform_p = glGetUniformLocation(player_prog, "camera_offset");

    glptr offset_mod_uniform = glGetUniformLocation(prog, "offset_mod");
    glptr rot_uniform = glGetUniformLocation(prog, "rotation_amount");
    glptr offset_uniform = glGetUniformLocation(prog, "offset");
    glptr stretch_uniform = glGetUniformLocation(prog, "stretch");
    glptr stretch_uniform_p = glGetUniformLocation(player_prog, "stretch");
    glptr is_enemy_uniform_p = glGetUniformLocation(player_prog, "is_enemy");

    glptr color_uniform = glGetUniformLocation(ui_prog, "color");

    while (!glfwWindowShouldClose(window)) {
        current_time = glfwGetTime();
        delta = current_time - prev_time;
        fps = 1.f / delta;

        prev_x = player_x;
        prev_y = player_y;

        for (int i = 0; i < enemy_num; i++) {
            enemy* e = enemies + i;
            
            e->prev_x = e->x;
            e->prev_y = e->y;
        }

        int PREV_GAME_WIDTH = GAME_WIDTH;
        int PREV_GAME_HEIGHT = GAME_HEIGHT;
        
        glfwGetWindowSize(window, &GAME_WIDTH, &GAME_HEIGHT);

        glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
        glClearColor(0.3f, 0.9f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindTexture(GL_TEXTURE_2D, tex0.tex);
        
        glUseProgram(prog);
        glUniform2f(cam_offset_uniform, camera_x / 640.f, camera_y / 360.f);
        glUniform1f(rot_uniform, 0);
        glUniform2f(offset_uniform, 0, 0);
        glUniform1f(stretch_uniform, 1);
        
        for (int i = 0; i < sprite_num; i++) {
            setOffsetModUniform(offset_mod_uniform, sprites[i].ss_info);
            draw_vertex_info(sprite_info[i]);
        }

        draw_particles(*particles.particles, camera_x, camera_y);

        glUseProgram(player_prog);
        glUniform2f(cam_offset_uniform_p, camera_x / 640.f, camera_y / 360.f);
        glUniform1f(stretch_uniform_p, 1);
        glUniform1i(is_enemy_uniform_p, 0);
        
        if ((player_x - camera_x > 300 || player_x - camera_x < 20) && player_x >= 20) {
            camera_x += (int)player_vx;

            while (player_x - camera_x > 300)
                camera_x++;

            while (player_x - camera_x < 20)
                camera_x--;
        }

        bind_action_bit(&player_action, LEFT, keys[GLFW_KEY_A]);
        bind_action_bit(&player_action, RIGHT, keys[GLFW_KEY_D]);
        bind_action_bit(&player_action, JUMP, keys[GLFW_KEY_W]);
        bind_action_bit(&player_action, ATTACK, mouse[0]);

        hitbox weapon_hitbox = {
            (int)(player_x + 4. * sin(-weapon_sprite.rotation) + 8 + weapon_sprite_xoff),
            (int)(player_y + 4. * cos(-weapon_sprite.rotation)),
            8, 8
        };

        for (int i = 0; i < enemy_num; i++) {
            enemy* e = enemies + i;

            walk(e, hitboxes, hitbox_num, delta);
            attack_response(e, player_action, weapon_hitbox, weapon_sprite_xoff > 0 ? RIGHT : LEFT, particles);

            if (e->health <= 0) {
                remove_enemy(enemies, enemy_num, i);
                enemy_num--;
            }

            update_player(&e->x, &e->y, &e->vx, &e->vy, &e->fall_time, &e->damage_time, 120 * delta, hitboxes, hitbox_num, e->event, ENEMY_SPEED);

            if (collide((hitbox){player_x, player_y, PLAYER_WIDTH, PLAYER_HEIGHT}, (hitbox){e->x, e->y, PLAYER_WIDTH, PLAYER_HEIGHT}) && damage_time >= PLAYER_DAMAGE_COOLDOWN) {
                player_vy = 2.f;

                if (e->vx > 0.f)
                    player_vx = 2.f;
                else
                    player_vx = -2.f;

                damage_time = 0;
                player_health--;
            }
        }

        update_player(&player_x, &player_y, &player_vx, &player_vy, &fall_time, &damage_time, 120 * delta, hitboxes, hitbox_num, player_action, PLAYER_SPEED);
        
        update_particle_list(particles, delta);

        if (player_health <= 0)
            break;

        if (player_x > 700) {
            // temp win logic
            hitbox_num = 2;
            enemy_num = 0;

            camera_x = 0;
            camera_y = 0;

            player_x = 0;
            player_y = 72;

            player_vx = 0;
            player_vy = 0;

            fall_time = 0;
            damage_time = PLAYER_DAMAGE_COOLDOWN;

            player_health = 20;

            player_action = 0;

            free(hitboxes);
            free(enemies);
            free(hitbox_ss_info);

            hitboxes = malloc(2 * sizeof(hitbox));
            hitboxes[0] = (hitbox){0, 60, 600, 12, 1};
            hitboxes[1] = (hitbox){0, 52, 600, 16, 1};
            
            hitbox_ss_info = malloc(2 * sizeof(spritesheet_info));

            hitbox_ss_info[0] = (spritesheet_info){0, 0, 16, 16};
            hitbox_ss_info[1] = (spritesheet_info){16, 0, 20, 16};

            for (int i = 0; i < 2; i++) {
                hitbox h = hitboxes[i];
                polygon hitbox_p = qtop(rect(h.x, h.y, h.width, h.height));
                hitbox_info[i] = polygon_vertex_info(hitbox_p, GL_STATIC_DRAW, 1, 0);
            }
        }

        // printf("%f\n", fps);

        update_player_quad(&player_quad, player_x, player_y, prev_x, prev_y, player_vx);
        quad_polygon(player_p, player_quad);

        info = polygon_vertex_info(player_p, GL_DYNAMIC_DRAW, 0, 0);

        draw_vertex_info(info);

        for (int i = 0; i < enemy_num; i++) {
            enemy e = enemies[i];
            quad e_quad = rect(e.x, e.y, PLAYER_WIDTH, PLAYER_HEIGHT);

            update_player_quad(&e_quad, e.x, e.y, e.prev_x, e.prev_y, e.vx);

            polygon e_p = qtop(e_quad);
            
            vertex_info e_info = polygon_vertex_info(e_p, GL_DYNAMIC_DRAW, 0, 0);
            free_p(e_p);

            glUniform1i(is_enemy_uniform_p, 1);
            draw_vertex_info(e_info);
        }

        glUseProgram(prog);
        glUniform2f(cam_offset_uniform, camera_x / 640.f, camera_y / 360.f);
        
        for (int i = 0; i < hitbox_num; i++) {
            setOffsetModUniform(offset_mod_uniform, hitbox_ss_info[i]);
            draw_vertex_info(hitbox_info[i]);
        }

        setOffsetModUniform(offset_mod_uniform, weapon_sprite.ss_info);

        weapon_sprite.x = player_x + 13 + weapon_sprite_xoff;
        weapon_sprite.y = player_y + 4;

        weapon_sprite_p = qtop((quad){
            -weapon_sprite.width / 2, -weapon_sprite.height / 2,
            -weapon_sprite.width / 2, weapon_sprite.height / 2,
            weapon_sprite.width / 2, weapon_sprite.height / 2,
            weapon_sprite.width / 2, -weapon_sprite.height / 2
        });

        weapon_sprite_info = polygon_vertex_info(weapon_sprite_p, GL_DYNAMIC_DRAW, 1, 1);

        free_p(weapon_sprite_p);

        if (player_action & ATTACK) {
            if (player_vx > 0) {
                weapon_sprite_rot_target = -0.5 * PI;
                weapon_sprite_xoff_target = 8;
            } else {
                weapon_sprite_rot_target = 0.5 * PI;
                weapon_sprite_xoff_target = -25;
            }
        } else {
            weapon_sprite_rot_target = 0;
            weapon_sprite_xoff_target = 0;
        }

        weapon_sprite.rotation += 0.125 * (weapon_sprite_rot_target - weapon_sprite.rotation);
        weapon_sprite_xoff += 0.125 * (weapon_sprite_xoff_target - weapon_sprite_xoff);

        glUniform1f(rot_uniform, weapon_sprite.rotation);
        glUniform2f(offset_uniform, weapon_sprite.x / 640.f - 1.f, weapon_sprite.y / 360.f - 1.f);
        glUniform1f(stretch_uniform, 16.f / 9.f);

        draw_vertex_info(weapon_sprite_info);

        free_p(health_amount_p);
        health_amount_p = qtop(rect(4, 173, player_health * 2, 3));

        glUseProgram(ui_prog);
        glUniform3f(color_uniform, .2f, 0.f, 0.f);
        draw_vertex_info(polygon_vertex_info(health_bar_p, GL_DYNAMIC_DRAW, 0, 0));

        glUniform3f(color_uniform, 1.f, 0.f, 0.f);
        draw_vertex_info(polygon_vertex_info(health_amount_p, GL_DYNAMIC_DRAW, 0, 0));

        draw_framebuffer_rect(fb);
        glfwSwapBuffers(window);
        glfwPollEvents();
        prev_time = current_time;

        if (GAME_WIDTH != PREV_GAME_WIDTH || GAME_HEIGHT != PREV_GAME_HEIGHT) {
            fb = create_framebuffer(GAME_WIDTH >> 2, GAME_HEIGHT >> 2, framebuffer_prog);
            glfwMakeContextCurrent(window);
        }
    }

    delete_buffers(info);
    glDeleteProgram(prog);
    glDeleteTextures(1, &tex0.tex);

    free((void*)tri_vert_shader);
    free((void*)tri_frag_shader);
    free((void*)framebuffer_vert_shader); 
    free((void*)framebuffer_frag_shader);
    free((void*)player_frag_shader);
    free(keys);
    free(mouse);
    free(hitboxes);
    free(hitbox_info);
    free(hitbox_ss_info);
    free(sprites);
    free(sprite_info);

    free(lvl.hitboxes);
    free(lvl.hitbox_ss_info);
    free(lvl.sprites);
    free(lvl.enemies);

    for (int i = 0; i < POINT_ALLOC_DEFAULT.index; i++) {
        if (POINT_ALLOC_DEFAULT.block[i])
            free(POINT_ALLOC_DEFAULT.block[i]);
    }

    free(POINT_ALLOC_DEFAULT.block);
    
    // free(weapon_sprite_p.points);

    free_particle_list(*particles.particles);

    glfwTerminate();
    return 0;
}