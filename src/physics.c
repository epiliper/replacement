#include "physics.h"
#include "utils.h"

void aabbMinMax(Body* b, vec3 min, vec3 max)
{
  glm_vec3_sub(b->pos, b->halfsize, min);
  glm_vec3_add(b->pos, b->halfsize, max);
}

void aabbMinkowskiDifference(Body* a, Body* b, Body* dest)
{
  glm_vec3_add(a->halfsize, b->halfsize, dest->halfsize);
  glm_vec3_sub(a->pos, b->pos, dest->pos);
}

void aabbNew(vec3* vertices, int n, Body* body)
{
  vec3 min = {FLT_MAX, FLT_MAX, FLT_MAX}, max = {FLT_MIN, FLT_MIN, FLT_MIN};

  for (int i = 0; i < n; i++)
  {
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

bool aabbCollide(Body* a, Body* b)
{
  Body* mink;
  vec3 min, max;
  aabbMinkowskiDifference(a, b, mink);
  aabbMinMax(mink, min, max);

  return (min[0] <= 0 && max[0] <= 0 && min[1] <= 0 && max[1] >= 0 &&
          min[2] <= 0 && max[2] >= 0);
}

Hit aabbIntersectRay(vec3 pos, vec3 magnitude, Body* body)
{
  Hit hit = {0};
  vec3 min, max;
  aabbMinMax(body, min, max);

  float last_entry = -INFINITY;
  float first_exit = INFINITY;

  for (uint8_t i = 0; i < 3; ++i)
  {
    if (magnitude[i] != 0)
    {
      float t1 = (min[i] - pos[i]) / magnitude[i];
      float t2 = (max[i] - pos[i]) / magnitude[i];

      last_entry = fmaxf(last_entry, fminf(t1, t2));
      first_exit = fminf(first_exit, fmaxf(t1, t2));
    }
    else if (pos[i] <= min[i] || pos[i] >= max[i])
    {
      return hit;
    }
  }

  if (last_entry <= first_exit && first_exit >= 0.0f && last_entry <= 1.0f)
  {
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

    if (px < py)
    {
      hit.normal[0] = (dx > 0) - (dx < 0);
    }
    else
    {
      hit.normal[1] = (dy > 0) - (dy < 0);
    }
  }

  return hit;
}

void physicsUpdate(kh_thing_t* things) {}

void physicsUpdateStatic(Thing* t, kh_thing_t* things) {}
void physicsUpdateDynamic(Thing* t, kh_thing_t* things) {}
