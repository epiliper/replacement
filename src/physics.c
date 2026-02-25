#include "physics.h"
#include "utils.h"

#define N_STEPS 4.0f
#define TICK_RATE 1.0f / N_STEPS

void aabbMinMax(Body* b, vec3 min, vec3 max) {
  glm_vec3_sub(b->pos, b->halfsize, min);
  glm_vec3_add(b->pos, b->halfsize, max);
}

void aabbMinkowskiDifference(Body* a, Body* b, Body* dest) {
  glm_vec3_add(a->halfsize, b->halfsize, dest->halfsize);
  glm_vec3_sub(a->pos, b->pos, dest->pos);
}

void aabbNew(vec3* vertices, int n, Body* body) {
  vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX}, max = {FLT_MIN, FLT_MIN, FLT_MIN};

  for (int i = 0; i < n; i++) {
    min[0] = MIN(min[0], vertices[i][0]);
    min[1] = MIN(min[1], vertices[i][1]);
    min[2] = MIN(min[2], vertices[i][2]);

    max[0] = MAX(max[0], vertices[i][0]);
    max[1] = MAX(max[1], vertices[i][1]);
    max[2] = MAX(max[2], vertices[i][2]);
  }

  body->halfsize[0] = (max[0] - min[0]) / 2 * body->width;
  body->halfsize[1] = (max[1] - min[1]) / 2 * body->height;
  body->halfsize[2] = (max[2] - min[2]) / 2;
}

bool aabbCollide(Body* a, Body* b) {
  Body* mink;
  vec3 min, max;
  aabbMinkowskiDifference(a, b, mink);
  aabbMinMax(mink, min, max);

  return (min[0] <= 0 && max[0] <= 0 && min[1] <= 0 && max[1] >= 0 &&
          min[2] <= 0 && max[2] >= 0);
}

Hit aabbIntersectRay(vec3 pos, vec3 magnitude, Body* body) {
  Hit hit = {0};
  vec3 min, max;
  aabbMinMax(body, min, max);

  float last_entry = -INFINITY;
  float first_exit = INFINITY;

  for (uint8_t i = 0; i < 3; ++i) {
    if (magnitude[i] != 0) {
      float t1 = (min[i] - pos[i]) / magnitude[i];
      float t2 = (max[i] - pos[i]) / magnitude[i];

      last_entry = fmaxf(last_entry, fminf(t1, t2));
      first_exit = fminf(first_exit, fmaxf(t1, t2));
    } else if (pos[i] <= min[i] || pos[i] >= max[i]) {
      return hit;
    }
  }

  if (last_entry <= first_exit && first_exit >= 0.0f && last_entry <= 1.0f) {
    hit.pos[0] = pos[0] + magnitude[0] * last_entry;
    hit.pos[1] = pos[1] + magnitude[1] * last_entry;
    hit.pos[2] = pos[2] + magnitude[2] * last_entry;

    hit.is_hit = true;
    hit.time = last_entry;

    float dx = hit.pos[0] - body->pos[0];
    float dy = hit.pos[1] - body->pos[1];
    float dz = hit.pos[2] - body->pos[2];

    float px = body->halfsize[0] - fabsf(dx);
    float py = body->halfsize[1] - fabsf(dy);
    float pz = body->halfsize[2] - fabsf(dz);

    // we need to find the axis which has the earliest penetration (giggity)
    if (px < py && px < pz) {
      hit.normal[0] = (dx > 0) - (dx < 0);
    } else if (py < pz) {
      hit.normal[1] = (dy > 0) - (dy < 0);
    } else {
      hit.normal[2] = (dz > 0) - (dz < 0);
    }
  }

  return hit;
}

// Find the nearest collision (if any) of the body with a neighboring body
// (static or dynamic) and update its position and velocity accordingly. This is
// stage 1.
void sweepResponse(Body* b, vec3 velocity, kh_thing_t* things) {
  khiter_t k;
  Thing* other;

  Hit sweep_hit = {0};
  sweep_hit.time = INFINITY;

  Hit static_hit = {0};
  static_hit.time = INFINITY;

  for (int i = kh_begin(things); i < kh_end(things); i++) {
    if (!kh_exist(things, i)) continue;
    other = kh_val(things, i);

    // TODO: have body field for static vs dynamic. Right now we just update
    // sweephit.
    if (b == &other->loc) continue;

    Hit hit = aabbIntersectRay(b->pos, velocity, &other->loc);
    if (other->loc.is_dynamic) {
      if (hit.is_hit && hit.time < sweep_hit.time) sweep_hit = hit;
    } else if (hit.is_hit && hit.time < static_hit.time) {
      static_hit = hit;
    }
  }

  if (sweep_hit.is_hit) {
    // call on_hit callbacks for moving things here...
    // note that in this simple simulation we don't update things just yet.
  }

  // TODO: need to do callback on hit with moving body
  if (!static_hit.is_hit) {
    glm_vec3_add(b->pos, b->velocity, b->pos);
    return;
  }

  glm_vec3_copy(static_hit.pos, b->pos);

  if (static_hit.normal[0] != 0) b->velocity[0] = 0;
  if (static_hit.normal[1] != 0) b->velocity[1] = 0;
  if (static_hit.normal[2] != 0) b->velocity[2] = 0;
}

void physicsUpdate(kh_thing_t* things, double delta_time) {
  Body* b;
  Thing* t;
  for (int i = kh_begin(things); i < kh_end(things); i++) {
    if (!kh_exist(things, i)) continue;

    t = kh_val(things, i);
    b = &t->loc;

    vec3 scaled_velocity;
    glm_vec3_scale(b->velocity, delta_time * TICK_RATE, scaled_velocity);
    for (int j = 0; j < N_STEPS; j++) {
      sweepResponse(b, scaled_velocity, things);
    }
  }
}

// Once we're done moving our body around due to collisions with neighboring
// bodies, we need to check to make sure it's not overlapping with another body.
// If it is, we need to move it out. This is part 2.
void staticResponse(Body* b, kh_thing_t* things) {}
