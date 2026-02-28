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
  vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX}, max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

  for (int i = 0; i < n; i++) {
    min[0] = MIN(min[0], vertices[i][0]);
    min[1] = MIN(min[1], vertices[i][1]);
    min[2] = MIN(min[2], vertices[i][2]);

    max[0] = MAX(max[0], vertices[i][0]);
    max[1] = MAX(max[1], vertices[i][1]);
    max[2] = MAX(max[2], vertices[i][2]);
  }

  body->halfsize[0] = (max[0] - min[0]) / 2;
  body->halfsize[1] = (max[1] - min[1]) / 2;
  body->halfsize[2] = (max[2] - min[2]) / 2;
  glm_vec3_mul(body->halfsize, body->scale, body->halfsize);
}

bool aabbCollide(Body* a, Body* b) {
  Body mink = {0};
  vec3 min, max;
  aabbMinkowskiDifference(a, b, &mink);
  aabbMinMax(&mink, min, max);

  return (min[0] <= 0 && max[0] <= 0 && min[1] <= 0 && max[1] >= 0 &&
          min[2] <= 0 && max[2] >= 0);
}

Hit aabbIntersectRay(vec3 pos, vec3 magnitude, Body* body) {
  Hit hit = {0};
  vec3 min, max;
  aabbMinMax(body, min, max);

  float tmin = 0.0f, tmax = 1.0f;
  float sign = -1.0f;
  int axis = -1;

  for (uint8_t i = 0; i < 3; i++) {
    if (fabsf(magnitude[i]) > 1e-8f) {
      float t1 = (min[i] - pos[i]) / magnitude[i];
      float t2 = (max[i] - pos[i]) / magnitude[i];

      float tNear = fminf(t1, t2);
      float tFar = fmaxf(t1, t2);

      if (tNear > tmin) {
        sign = (t1 < t2) ? -1.0f : 1.0f;
        axis = i;
      }

      tmin = fmaxf(tmin, tNear);
      tmax = fminf(tmax, tFar);

      if (tmin > tmax) return hit;

    } else {
      // Ray is parallel to slab
      if (pos[i] < min[i] || pos[i] > max[i]) return hit;
    }
  }

  if (tmin <= tmax) {
    log_debug("TMIN: %f, TMAX: %f", tmin, tmax);
    hit.pos[0] = pos[0] + magnitude[0] * tmin;
    hit.pos[1] = pos[1] + magnitude[1] * tmin;
    hit.pos[2] = pos[2] + magnitude[2] * tmin;

    if (axis != -1) {
      hit.normal[axis] = sign;
    }

    hit.is_hit = true;
    hit.time = tmin;
  }

  return hit;
}

void physicsSweep(Body* b, vec3 velocity, kh_thing_t* things) {
  Hit closest = {0};
  closest.time = INFINITY;
  Thing* t;
  Body* other;

  for (int i = kh_begin(things); i < kh_end(things); i++) {
    if (!kh_exist(things, i)) continue;
    t = kh_val(things, i);
    other = &t->body;

    if (other == b) continue;
    Body expanded = *other;
    glm_vec3_add(b->halfsize, other->halfsize, expanded.halfsize);

    Hit hit = aabbIntersectRay(b->pos, velocity, &expanded);
    if (hit.is_hit && hit.time < closest.time) {
      closest = hit;
    }
  }

  if (!closest.is_hit) {
    glm_vec3_add(b->pos, velocity, b->pos);
    return;
  }

  // we actually have a collision
  if (closest.normal[0] != 0) {
    b->velocity[0] = 0;
  } else if (closest.normal[1] != 0) {
    b->velocity[1] = 0;
    b->is_grounded = true;
  } else {
    b->velocity[2] = 0;
  }

  // 2. Update position of the body to reflect the collision.
  glm_vec3_scale(closest.normal, 1e-4f, closest.normal);

  glm_vec3_scale(velocity, closest.time, velocity);
  glm_vec3_add(velocity, closest.normal, velocity);
  glm_vec3_add(b->pos, velocity, b->pos);
}

void physicsUpdate(kh_thing_t* things, double delta_time) {
  Body* b;
  Thing* t;
  for (int i = kh_begin(things); i < kh_end(things); i++) {
    if (!kh_exist(things, i)) continue;

    t = kh_val(things, i);
    b = &t->body;

    if (!b->is_dynamic) continue;

    if (!b->is_grounded) {
      b->velocity[1] -= 9.8f * delta_time;
    }

    vec3 scaled_velocity;
    glm_vec3_scale(b->velocity, delta_time, scaled_velocity);
    physicsSweep(b, scaled_velocity, things);
  }
}
