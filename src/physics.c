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

  body->halfsize[0] = (max[0] - min[0]) / 2 * body->width;
  body->halfsize[1] = (max[1] - min[1]) / 2 * body->height;
  body->halfsize[2] = (max[2] - min[2]);
}

bool aabbCollide(Body* a, Body* b) {
  Body* mink;
  vec3 min, max;
  aabbMinkowskiDifference(a, b, mink);
  aabbMinMax(mink, min, max);

  return (min[0] <= 0 && max[0] <= 0 && min[1] <= 0 && max[1] >= 0 &&
          min[2] <= 0 && max[2] >= 0);
}

#define EPSILON 0.0001f
Hit aabbIntersectRay(vec3 pos, vec3 magnitude, Body* body) {
  Hit hit = {0};
  vec3 min, max;
  aabbMinMax(body, min, max);

  float tmin = 0.0f, tmax = 1.0f;

  for (uint8_t i = 0; i < 3; i++) {
    if (fabsf(magnitude[i]) > 1e-8f) {
      float t1 = (min[i] - pos[i]) / magnitude[i];
      float t2 = (max[i] - pos[i]) / magnitude[i];

      float tNear = fminf(t1, t2);
      float tFar = fmaxf(t1, t2);

      tmin = fmaxf(tmin, tNear);
      tmax = fminf(tmax, tFar);

      if (tmin > tmax) return hit;

    } else {
      // Ray is parallel to slab
      if (pos[i] < min[i] || pos[i] > max[i]) return hit;
    }
  }

  log_debug("TMIN: %f, TMAX: %f", tmin, tmax);
  if (tmin <= tmax) {
    hit.pos[0] = pos[0] + magnitude[0] * tmin;
    hit.pos[1] = pos[1] + magnitude[1] * tmin;
    hit.pos[2] = pos[2] + magnitude[2] * tmin;

    hit.is_hit = true;
    hit.time = tmin;

    // distance from edge of aabb to the central position of the body
    float dx = hit.pos[0] - body->pos[0];
    float dy = hit.pos[1] - body->pos[1];
    float dz = hit.pos[2] - body->pos[2];

    // face of boxes touching collision
    float px = body->halfsize[0] - fabsf(dx);
    float py = body->halfsize[1] - fabsf(dy);
    float pz = body->halfsize[2] - fabsf(dz);

    // we need to find the axis which has the earliest penetration (giggity)
    //
    // if pz is lowest, we've collided with a face on the z plane
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

  // 1. find axis along which we collided.
  // We need to remove velocity along that axis. For now we are just zero-ing
  // it.
  if (closest.normal[0] != 0) {
    velocity[0] = 0;
  } else if (closest.normal[1] != 0) {
    velocity[1] = 0;
  } else {
    velocity[2] = 0;
  }

  // 2. Update position of the body to reflect the collision.
  glm_vec3_scale(closest.normal, 1e-4f, closest.normal);
  glm_vec3_add(closest.pos, closest.normal, closest.pos);
  glm_vec3_copy(closest.pos, b->pos);
}

void physicsUpdate(kh_thing_t* things, double delta_time) {
  Body* b;
  Thing* t;
  for (int i = kh_begin(things); i < kh_end(things); i++) {
    if (!kh_exist(things, i)) continue;

    t = kh_val(things, i);
    b = &t->body;

    if (!b->is_dynamic) continue;

    vec3 scaled_velocity;
    glm_vec3_scale(b->velocity, delta_time, scaled_velocity);
    physicsSweep(b, scaled_velocity, things);
  }
}
