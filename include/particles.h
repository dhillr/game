#include <wrapper.h>

#define PARTICLE_LIST_EMPTY (particle_list){NULL, 0}

typedef struct {
    float x, y;
    float vx, vy;
    polygon shape;
    vertex_info info;
    glptr shader_prog;

    uint32_t lifetime;
    float time_elapsed;

    void* __list_prev;
    void* __list_next;
} particle;

typedef struct {
    particle* head;
    size_t len;
} particle_list;

typedef struct {
    particle_list* particles;
    float gravity;
} particle_env;

particle square_particle(int x, int y, int size, vec3 col, int lifetime_milliseconds) {
    polygon shape = qtop((quad){x, y, x, y + size, x + size, y + size, x + size, y});

    return (particle){
        .x=x, .y=y,
        .vx=0, .vy=0,
        .shape=shape,
        .info=polygon_vertex_info(shape, GL_DYNAMIC_DRAW, 0, 0),
        .shader_prog=shader_program(
            "#version 330 core\nlayout(location=0)in vec2 pos;uniform vec2 a;void main(){gl_Position=vec4(pos-a,0,1);}",
            "#version 330 core\nout vec4 c;void main(){c=vec4(1,.8,0,1);}"
        ),
        .lifetime=lifetime_milliseconds,
        .time_elapsed=0.f,
        .__list_prev=NULL, .__list_next=NULL
    };
}

particle* get_particle(particle_list particles, size_t index) {
    particle* node = particles.head;

    for (int i = 0; i < index; i++) {
        node = (particle*)node->__list_next;
    }

    return node;
}

void splice_particle_p(particle_list* particles, particle* p) {
    if (p->__list_prev) {
        if (p->__list_next)
            ((particle*)p->__list_next)->__list_prev = p->__list_prev;
        ((particle*)p->__list_prev)->__list_next = p->__list_next;
    } else {
        if (p->__list_next)
            ((particle*)p->__list_next)->__list_prev = NULL;
        particles->head = p->__list_next;
    }

    particles->len--;


    free(p->shape.points);
    free(p);
}

void update_particle_list(particle_env env, float dt) {
    if (env.particles->len < 1)
        return;

    particle p_ = {.__list_next=env.particles->head};

    particle* p = &p_;
    particle* next = p->__list_next;

    while (next) {
        p = p->__list_next;
        next = p->__list_next;

        p->vy -= env.gravity;

        p->x += p->vx;
        p->y += p->vy;

        for (int i = 0; i < p->shape.numPoints; i++) {
            p->shape.points[i].x += p->vx;
            p->shape.points[i].y += p->vy;
        }

        p->info = polygon_vertex_info(p->shape, GL_DYNAMIC_DRAW, 0, 0);

        p->time_elapsed += 1000.f * dt;

        if (p->time_elapsed > p->lifetime)
            splice_particle_p(env.particles, p);
    }
}

void free_particle_list(particle_list particles) {
    particle* p = particles.head;

    if (!p)
        return;

    while (p->__list_next) {
        free(p->shape.points);
        free(p);
        p = p->__list_next;
    }
}

void add_particle(particle_list* particles, particle p) {
    particle* node = particles->head;

    if (node) {
        while (node->__list_next)
            node = (particle*)node->__list_next;
    }

    particle* p_ptr = malloc(sizeof(particle));
    p_ptr[0] = p;

    if (node) {
        p_ptr->__list_prev = node;
        node->__list_next = p_ptr;
    } else {
        particles->head = p_ptr;
    }

    particles->len++;
}

void spawn_particles(particle_list* particles, particle* arr, size_t num_spawned) {
    for (int i = 0; i < num_spawned; i++) {
        add_particle(particles, arr[i]);
    }
}

void draw_particles(particle_list particles, int cam_x, int cam_y) {
    particle* p = particles.head;

    if (!p)
        return;

    while (p->__list_next) {
        glUseProgram(p->shader_prog);
        glUniform2f(0, cam_x / 640.f, cam_y / 360.f);
        draw_vertex_info(p->info);

        p = p->__list_next;
    }
}