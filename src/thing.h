#ifndef THING
#define THING
#include <cglm/cglm.h>

// openGL handles to render an object
typedef struct {
  unsigned int vao, shader;
} RenderInfo;

typedef struct hit {
  float time;
  vec3 pos;
  vec3 normal;
  bool is_hit;
} Hit;

// matrices necessary for rendering
typedef struct {
  mat4* proj;
  mat4* view;
} RenderMatrices;

// thing types
enum {
  THING_TRIANGLE,
  THING_CUBE,
  THING_SQUARE,
  THING_BACKPACK,
};

typedef struct TriangleThing {
  vec4 color;
} TriangleThing;

typedef struct SquareThing {
  vec4 color;
} SquareThing;

typedef struct CubeThing {
  vec4 color;
} CubeThing;

// TODO
typedef struct {
} RenderMods;

// physical information about the object being rendered
typedef struct {
  vec3 pos;
  vec3 rot;
  float height, width;
  vec3 halfsize;
} Body;

// Function to render a particular thing
typedef void (*RenderFunc)(void* self, Body* body, RenderInfo ri,
                           RenderMatrices rm, RenderMods* mods);

// Function to initialize opengl data for a particular thing
typedef RenderInfo (*RenderInitFunc)();

// Interface for something that is renderable. If we haven't already created
// vaos, vbos, etc. for the type, then we call the union's init func, otherwise
// we get its renderinfo.
typedef struct {
  RenderFunc rfunc;
  union {
    RenderInfo ri;
    RenderInitFunc rinit;
  };
} Renderable;

// A thing
typedef struct {
  Body loc;
  int type;
  Renderable render;
  void* self;
  uint16_t id;
} Thing;
#endif

Thing* thingLoadFromData(void* data, int type, Body* loc);
void renderCube(CubeThing* self, Body* body, RenderInfo ri, RenderMatrices rm,
                RenderMods* mods);
void renderAABB(CubeThing* self, Body* body, RenderInfo ri, RenderMatrices rm,
                RenderMods* mods);

Hit aabbIntersectRay(vec3 pos, vec3 magnitude, Body* body);
void aabbMinkowskiDifference(Body* a, Body* b, Body* dest);
