#include "thing.h"
/* #include <cglm/affine-pre.h> */
#include "utils.h"
#include "log.h"
#include "mesh.h"
#include "physics.h"

/*
 * ===============
 * @THING MANAGER
 * =============
 */

Things THINGS = {0};

// Initialize the thing manager
void thingsInit()
{
  THINGS.things = kh_init_thing();
  THINGS.curid = 0;
  THINGS.init = 1;
}

Result thingAdd(Thing* t)
{
  if (!THINGS.init)
  {
    log_error("Thing manager not initialized!");
    return Err;
  }

  int ret = 0;
  khiter_t k;

  do
  {
    t->id = THINGS.curid++;
  } while ((k = kh_get_thing(THINGS.things, t->id)) != kh_end(THINGS.things));

  k = kh_put_thing(THINGS.things, t->id, &ret);
  kh_value(THINGS.things, k) = t;

  log_debug("added thing with id %d", t->id);

  return Ok;
}

Result thingDelete(int id)
{
  if (!THINGS.init)
  {
    log_error("Thing manager not initialized!");
    return Err;
  }

  khiter_t k;
  k = kh_get_thing(THINGS.things, id);

  if (k == kh_end(THINGS.things))
  {
    log_warn("Attempted to delete non-existent thing with id %d", id);
    return Err;
  }

  kh_del_thing(THINGS.things, k);

  return Ok;
}

/*
 * ===========
 * @PRIMITIVES
 * ===========
 */

// clang-format off
float TRIANGLE_VERTICES[] = {
    -0.5, -0.5, 0, 
    0.5, -0.5, 0, 
    0.0f, 0.5f, 0,
};

float SQUARE_VERTICES[] = {
	-0.5, -0.5, 0,
	0.5, -0.5, 0,
	0.5, 0.5, 0,
	-0.5, 0.5, 0
};

unsigned int SQUARE_INDICES[] = {
	0, 1, 2,
	2, 3, 0
};

// clang-format off
// x     y     z     tex_x tex_y
float CUBE_VERTICES[120] = {
  // front (z = 0.5)
  -0.5, -0.5,  0.1,   1.0f, 1.0f,  // 0
   0.5, -0.5,  0.1,   1.0f, 0.0f,  // 1
   0.5,  0.5,  0.1,   0.0f, 0.0f,  // 2
  -0.5,  0.5,  0.1,   0.0f, 1.0f,  // 3
  
  // back (z = -0.5)
  -0.5, -0.5, -0.1,   1.0f, 1.0f,  // 4
   0.5, -0.5, -0.1,   1.0f, 0.0f,  // 5
   0.5,  0.5, -0.1,   0.0f, 0.0f,  // 6
  -0.5,  0.5, -0.1,   0.0f, 1.0f,  // 7
  
  // left (x = -0.5)
  -0.5, -0.5, -0.1,   1.0f, 1.0f,  // 8
  -0.5, -0.5,  0.1,   1.0f, 0.0f,  // 9
  -0.5,  0.5,  0.1,   0.0f, 0.0f,  // 10
  -0.5,  0.5, -0.1,   0.0f, 1.0f,  // 11
  
  // right (x = 0.5)
   0.5, -0.5, -0.1,   1.0f, 1.0f,  // 12
   0.5, -0.5,  0.1,   1.0f, 0.0f,  // 13
   0.5,  0.5,  0.1,   0.0f, 0.0f,  // 14
   0.5,  0.5, -0.1,   0.0f, 1.0f,  // 15
  
  // top (y = 0.5)
  -0.5,  0.5,  0.1,   1.0f, 1.0f, // 16
   0.5,  0.5,  0.1,   1.0f, 0.0f, // 17
   0.5,  0.5, -0.1,   0.0f, 0.0f, // 18
  -0.5,  0.5, -0.1,   0.0f, 1.0f, // 19
  
  // bottom (y = -0.5)
  -0.5, -0.5,  0.1,   1.0f, 1.0f, // 20
   0.5, -0.5,  0.1,   1.0f, 0.0f, // 21
   0.5, -0.5, -0.1,   0.0f, 0.0f, // 22
  -0.5, -0.5, -0.1,   0.0f, 1.0f, // 23
};

float CUBE_TEX[48] = {
  1.0f, 1.0f,
  1.0f, 0.0f,
  0.0f, 0.0f, 
  0.0f, 1.0f,

  1.0f, 1.0f,
  1.0f, 0.0f,
  0.0f, 0.0f, 
  0.0f, 1.0f,

  1.0f, 1.0f,
  1.0f, 0.0f,
  0.0f, 0.0f, 
  0.0f, 1.0f,

  1.0f, 1.0f,
  1.0f, 0.0f,
  0.0f, 0.0f, 
  0.0f, 1.0f,

  1.0f, 1.0f,
  1.0f, 0.0f,
  0.0f, 0.0f, 
  0.0f, 1.0f,

  1.0f, 1.0f,
  1.0f, 0.0f,
  0.0f, 0.0f, 
  0.0f, 1.0f,
};

unsigned int CUBE_INDICES[36] = {
  // front
  0, 1, 2,
  0, 2, 3,
  
  // back
  5, 4, 7,
  5, 7, 6,
  
  // left
  8, 9, 10,
  8, 10, 11,
  
  // right
  12, 13, 14,
  12, 14, 15,
  
  // top
  16, 17, 18,
  16, 18, 19,
  
  // bottom
  21, 20, 23,
  21, 23, 22,
};

// clang-format on

const char* triangleVert =
    "#version 330 core\n"
    "layout (location = 0) in vec3 pos;\n"
    "uniform mat4 proj;\n"
    "uniform mat4 view;\n"
    "uniform mat4 model;\n"
    "void main() {\n"
    "gl_Position = proj * view * model * vec4(pos.xyz, 1.0f);\n"
    "}";

const char* triangleFrag =
    "#version 330 core\n"
    "out vec4 fragColor;\n"
    "uniform vec4 color;\n"
    "void main() {\n"
    "fragColor = color;\n"
    "}";

RenderInfo renderInitTriangle()
{
  RenderInfo ret = {0};

  unsigned int vao, vbo;

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE_VERTICES), TRIANGLE_VERTICES,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  ret.vao = vao;
  ret.shader = shaderFromCharVF(triangleVert, triangleFrag);

  checkGlError();
  return ret;
}

RenderInfo renderInitSquare()
{
  RenderInfo ri;

  unsigned int vbo, ebo;
  glGenVertexArrays(1, &ri.vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(ri.vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(SQUARE_VERTICES), SQUARE_VERTICES,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(SQUARE_INDICES), SQUARE_INDICES,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  ri.shader = shaderFromCharVF(triangleVert, triangleFrag);
  checkGlError();

  return ri;
};

RenderInfo renderInitCube()
{
  RenderInfo ri;
  unsigned int vbo, ebo;
  GL glGenVertexArrays(1, &ri.vao);
  GL glGenBuffers(1, &vbo);
  GL glGenBuffers(1, &ebo);

  GL glBindVertexArray(ri.vao);
  GL glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GL glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_VERTICES), CUBE_VERTICES,
                  GL_STATIC_DRAW);

  GL glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  GL glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CUBE_INDICES), CUBE_INDICES,
                  GL_STATIC_DRAW);

  GL glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                           (void*)0);
  GL glEnableVertexAttribArray(0);

  ri.shader = shaderFromCharVF(triangleVert, triangleFrag);
  return ri;
}

void renderCube(CubeThing* self, Body* body, RenderInfo ri, RenderMatrices rm,
                RenderMods* mods)
{
  GL glUseProgram(ri.shader);
  GL glBindVertexArray(ri.vao);

  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, body->pos);

  glm_rotate(model, glm_rad(body->rot[0]), (vec3){1, 0, 0});
  glm_rotate(model, glm_rad(body->rot[1]), (vec3){0, 1, 0});

  glm_scale(model, (vec3){body->width, body->height, 1});

  shaderSetMat4(ri.shader, "proj", *rm.proj);
  shaderSetMat4(ri.shader, "view", *rm.view);
  shaderSetMat4(ri.shader, "model", model);

  shaderSetVec4(ri.shader, "color", self->color);
  GL glDrawElements(GL_TRIANGLES, sizeof(CUBE_VERTICES), GL_UNSIGNED_INT, 0);
}

void renderAABB(CubeThing* self, Body* body, RenderInfo ri, RenderMatrices rm,
                RenderMods* mods)
{
  GL glUseProgram(ri.shader);
  GL glBindVertexArray(ri.vao);

  vec3 min, max;
  aabbMinMax(body, min, max);

  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, body->pos);

  glm_rotate(model, glm_rad(body->rot[0]), (vec3){1, 0, 0});
  glm_rotate(model, glm_rad(body->rot[1]), (vec3){0, 1, 0});

  glm_scale(model, (vec3){body->width, body->height, 1});

  shaderSetMat4(ri.shader, "proj", *rm.proj);
  shaderSetMat4(ri.shader, "view", *rm.view);
  shaderSetMat4(ri.shader, "model", model);

  shaderSetVec4(ri.shader, "color", self->color);
  GL glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
};

void renderTriangle(TriangleThing* self, Body* body, RenderInfo ri,
                    RenderMatrices rm, RenderMods* mods)
{
  GL glBindVertexArray(ri.vao);
  GL glUseProgram(ri.shader);
  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, body->pos);

  glm_rotate(model, glm_rad(body->rot[0]), (vec3){1, 0, 0});
  glm_rotate(model, glm_rad(body->rot[1]), (vec3){0, 1, 0});

  glm_scale(model, (vec3){body->width, body->height, 1});

  shaderSetMat4(ri.shader, "proj", *rm.proj);
  shaderSetMat4(ri.shader, "view", *rm.view);
  shaderSetMat4(ri.shader, "model", model);

  shaderSetVec4(ri.shader, "color", self->color);

  GL glDrawArrays(GL_TRIANGLES, 0, 3);
}

void renderSquare(SquareThing* self, Body* body, RenderInfo ri,
                  RenderMatrices rm, RenderMods* mods)
{
  GL glUseProgram(ri.shader);
  GL glBindVertexArray(ri.vao);

  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, body->pos);

  glm_rotate(model, glm_rad(body->rot[0]), (vec3){1, 0, 0});
  glm_rotate(model, glm_rad(body->rot[1]), (vec3){0, 1, 0});

  glm_scale(model, (vec3){body->width, body->height, 1});

  shaderSetMat4(ri.shader, "proj", *rm.proj);
  shaderSetMat4(ri.shader, "view", *rm.view);
  shaderSetMat4(ri.shader, "model", model);

  shaderSetVec4(ri.shader, "color", self->color);
  GL glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

Thing* thingLoadFromData(void* data, int type, Body* loc)
{
  Renderable render;
  Thing* dest = malloc(sizeof(Thing));

  if (!dest)
  {
    log_error("failed to allocate memory for thing with type: %d", type);
    return NULL;
  }

  switch (type)
  {
    case THING_TRIANGLE:
      render.rfunc = (RenderFunc)renderTriangle;
      render.rinit = (RenderInitFunc)renderInitTriangle;
      aabbNew((vec3*)TRIANGLE_VERTICES, 3, loc);
      break;
    case THING_CUBE:
      render.rfunc = (RenderFunc)renderCube;
      render.rinit = (RenderInitFunc)renderInitCube;
      break;
    case THING_SQUARE:
      render.rfunc = (RenderFunc)renderSquare;
      render.rinit = (RenderInitFunc)renderInitSquare;
      aabbNew((vec3*)SQUARE_VERTICES, 4, loc);
      break;
    case THING_BACKPACK:
      // We assume the model has already been loaded.
      render.rfunc = (RenderFunc)renderModel;
      render.rinit = (RenderInitFunc)renderInitModel;
      break;
    default:
      log_error("Unknown type id: %d", type);
      return NULL;
  }

  dest->type = type;
  dest->self = data;
  dest->loc = *loc;
  dest->render = render;

  return dest;
}
