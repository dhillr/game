#include <wrapper.h>

#define PARTICLE_LIST_EMPTY (particle_list){NULL, 0}

typedef struct {
    int x, y;
    float vx, vy;
    polygon shape;
    vertex_info info;
    glptr shader_prog;

    int lifetime;
    float time_elapsed;

    void* __list_prev;
    void* __list_next;
} particle;

typedef struct {
    particle* head;
    size_t len;
} particle_list;

particle square_particle(int x, int y, int size, vec3 col, int lifetime_milliseconds) {
    polygon shape = qtop((quad){x, y, x, y + size, x + size, y + size, x + size, y});

    return (particle){
        .x=x, .y=y,
        .vx=0, .vy=0,
        .shape=shape,
        .info=polygon_vertex_info(shape, GL_DYNAMIC_DRAW, 0, 0),
        .shader_prog=shader_program(
            "#version 330 core\nlayout(location=0)in vec2 pos;uniform vec2 a;void main(){gl_Position=vec4(pos-a,0,1);}",
            "#version 330 core\nout vec4 c;void main(){c=vec4(1);}"
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
    if (p->__list_prev)
        ((particle*)p->__list_prev)->__list_next = p->__list_next;
    else
        particles->head = p->__list_next;

    particles->len--;
}

void update_particle_list(particle_list* particles, float dt) {
    if (particles->len < 1)
        return;

    particle* p = particles->head;

    while (p->__list_next) {
        p = p->__list_next;
        
        p->x += (int)(p->vx + 0.5);
        p->y += (int)(p->vy + 0.5);

        for (int i = 0; i < p->shape.numPoints; i++) {
            p->shape.points[i].x += (int)(p->vx + 0.5);
            p->shape.points[i].y += (int)(p->vy + 0.5);
        }

        p->info = polygon_vertex_info(p->shape, GL_DYNAMIC_DRAW, 0, 0);

        p->time_elapsed += 1000.f * dt;

        if (p->time_elapsed > p->lifetime)
            splice_particle_p(particles, p);
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
    for (int i = 0; i < particles.len; i++) {
        particle p = *get_particle(particles, i);

        glUniform2f(0, cam_x / 640.f, cam_y / 360.f);
        glUseProgram(p.shader_prog);
        draw_vertex_info(p.info);
    }
}