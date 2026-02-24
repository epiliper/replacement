#include "thing.h"

// update all bodies in the physics system
void physicsUpdate(kh_thing_t *things);

// update body of a thing with respect to all static things
void physicsUpdateStatic(Thing *t, kh_thing_t *things);

// update body of a thing with respect to all dynamic things
void physicsUpdateDynamic(Thing *t, kh_thing_t *things);

void aabbMinMax(Body *b, vec3 min, vec3 max);
void aabbMinkowskiDifference(Body *a, Body *b, Body *dest);
void aabbNew(vec3 *vertices, int n, Body *body);
bool aabbCollide(Body *a, Body *b);
Hit aabbIntersectRay(vec3 pos, vec3 magnitude, Body *body);
