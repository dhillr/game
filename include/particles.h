#include <wrapper.h>

typedef struct {
    int x, y;
    vertex_info info;
    glptr shader_prog;
} particle;

particle square_particle(int x, int y, vec3 col) {
    char* frag_source = malloc(strlen("#version 330 core\nout vec4 c;void main(){") + strlen);

    strcpy(frag_source, "#version 330 core\nout vec4 c;void main(){");

    glptr prog = shader_program("#version 330 core\nlayout(location=0)in vec2 pos;void main(){gl_Position=vec4(pos,0,1);}", frag_source);
}

void add_particle(particle** particles, size_t num_particles, particle p) {
    *particles = (particle*)realloc(*particles, (num_particles + 1) * sizeof(particle));
    (*particles)[num_particles] = p;
}

void spawn_particles(particle** particles, size_t num_particles, particle* arr, size_t num_spawned) {
    for (int i = 0; i < num_particles; i++) {
        add_particle(particles, num_particles, arr[i]);
    }
}