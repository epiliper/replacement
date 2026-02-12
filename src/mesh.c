#include "cglm/cglm.h"
#include "glad.h"
#include "stdio.h"
#include "utils.h"
#include <stddef.h>
#include "mesh.h"
#include "utils.h"

#include "stdio.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

static int modelLoaderInitialized = 0;

struct TextureCache {
	kvec_t(char*) loaded;
};

struct TextureCache TEXTURE_CACHE;


void textureCacheAdd(char *t)
{
	char *str = strdup(t);
	kv_push(char*, TEXTURE_CACHE.loaded, t);
}

int textureCacheContains(char *t) {
	for (int i = 0; i < TEXTURE_CACHE.loaded.n; i++) {
		if (!strcmp(t, TEXTURE_CACHE.loaded.a[i])) {
			return 1;
		}
	}

	return 0;
}

void modelLoaderInit() {
	if (modelLoaderInitialized) {
		return;
	}

	kv_init(TEXTURE_CACHE.loaded);
	modelLoaderInitialized = 1;
}

const char *modelVert = "#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec3 aNormal;\n"
"layout(location = 2) in vec2 aTexCoords;\n"
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"void main() {\n"
"TexCoords = aTexCoords;\n"
"gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"}\n";

const char *modelFrag = "#version 330 core\n"
"out vec4 fragColor;\n"
"in vec2 TexCoords;\n"
"uniform sampler2D texture_diffuse1;\n"
"void main() {\n"
"fragColor = texture(texture_diffuse1, TexCoords);\n"
"}";

enum TEXTURE_TYPE {
  T_SPECULAR,
  T_DIFFUSE,
  T_NORMAL,
  T_AMBIENT,
  T_OTHER,
  T_TYPES,
};

const char* textureNames[T_TYPES] = {
    "texture_specular",
    "texture_diffuse",
    "texture_normal",
    "texture_height",
    "texture_other",
};


void meshSetup(Mesh* dest) {
  unsigned int vbo, ebo;
  glGenVertexArrays(1, &dest->ri.vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(dest->ri.vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, dest->vertices.n * sizeof(MeshVertex),
               dest->vertices.a, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, dest->indices.n * sizeof(unsigned int),
               dest->indices.a, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (void*)offsetof(MeshVertex, normals));

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (void*)offsetof(MeshVertex, texcoords));
}

void meshDraw(Mesh* m, int shader) {
  unsigned int diffuseNr = 1;
  unsigned int specularNr = 1;
  char uniform[256];

  for (unsigned int i = 0; i < m->textures.n; i++) {
    GL glActiveTexture(GL_TEXTURE0 + i);

    switch (m->textures.a[i].type) {
      case (T_DIFFUSE):
        uniform[snprintf(uniform, 256, "%s%d", textureNames[T_DIFFUSE],
                         diffuseNr)] = '\0';
        diffuseNr++;
        break;
      case (T_SPECULAR):
        uniform[snprintf(uniform, 256, "%s%d",
                         textureNames[T_SPECULAR], specularNr)] = '\0';
        specularNr++;
        break;

      default:
      	continue;
    }

    shaderSetInt(shader, uniform, (int)i);
    GL glBindTexture(GL_TEXTURE_2D, m->textures.a[i].id);
  }

  GL glActiveTexture(GL_TEXTURE0);

  GL glBindVertexArray(m->ri.vao);
  GL glDrawElements(GL_TRIANGLES, m->indices.n, GL_UNSIGNED_INT, 0);
  GL glBindVertexArray(0);
}


void modelRender(Model *m, Body *body, RenderInfo ri, RenderMatrices rm, RenderMods *mods) {
	GL glUseProgram(ri.shader);

	mat4 model;
	glm_mat4_identity(model);
	glm_translate(model, body->pos);

	shaderSetMat4(ri.shader, "projection", *rm.proj);
	shaderSetMat4(ri.shader, "view", *rm.view);
	shaderSetMat4(ri.shader, "model", model);

	for (unsigned int i = 0; i < m->meshes.n; i++) {
		meshDraw(&m->meshes.a[i], ri.shader);
	}
}


char workbuf[1024];


unsigned int textureFromFile(const char* file, const char* directory,
                             bool gamma) {
  workbuf[snprintf(workbuf, 1024, "%s/%s", directory, file)] = '\0';

  stbi_set_flip_vertically_on_load(true);

  unsigned int texid;
  glGenTextures(1, &texid);

  int width, height, nrComponents;
  unsigned char* data = stbi_load(workbuf, &width, &height, &nrComponents, 0);
  if (!data) {
    log_error("Failed to load texture at path %s", workbuf);
    stbi_image_free(data);
    return -1;
  }

  GLenum format;
  if (nrComponents == 1) {
    format = GL_RED;
  } else if (nrComponents == 3) {
    format = GL_RGB;
  } else if (nrComponents == 4) {
    format = GL_RGBA;
  }

  glBindTexture(GL_TEXTURE_2D, texid);
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(data);

  return texid;
}

void loadMaterialTextures(struct aiMaterial* mat, enum aiTextureType type,
                          int typeName, const char* dir, mTexVec* textures) {
  int texcount = aiGetMaterialTextureCount(mat, type);
  for (unsigned int i = 0; i < texcount; i++) {
    struct aiString str;
    aiGetMaterialTexture(mat, type, i, &str, NULL, NULL, NULL, NULL, NULL,
                         NULL);

    if (textureCacheContains(str.data))
    	continue;

    log_debug("Loading texture %s", str.data);
    MeshTexture* dest = (kv_pushp(MeshTexture, (*textures)));
    dest->id = textureFromFile(str.data, dir, 0);
    dest->type = typeName;

    textureCacheAdd(str.data);
  }
}

void processAssimpMesh(const struct aiMesh* mesh, const struct aiScene* scene,
                       const char *directory, Mesh* dest) {
  vec3 vector;
  MeshVertex vert;
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    vector[0] = mesh->mVertices[i].x;
    vector[1] = mesh->mVertices[i].y;
    vector[2] = mesh->mVertices[i].z;
    glm_vec3_copy(vector, vert.pos);

    if (mesh->mNormals) {
      vector[0] = mesh->mNormals[i].x;
      vector[1] = mesh->mNormals[i].y;
      vector[2] = mesh->mNormals[i].z;
      glm_vec3_copy(vector, vert.normals);
    }

    if (mesh->mTextureCoords[0]) {
      vec2 vec;
      vec[0] = mesh->mTextureCoords[0][i].x;
      vec[1] = mesh->mTextureCoords[0][i].y;
      glm_vec2_copy(vec, vert.texcoords);

      vector[0] = mesh->mTangents[i].x;
      vector[1] = mesh->mTangents[i].y;
      vector[2] = mesh->mTangents[i].z;
      glm_vec3_copy(vector, vert.tangent);

      vector[0] = mesh->mBitangents[i].x;
      vector[1] = mesh->mBitangents[i].y;
      vector[2] = mesh->mBitangents[i].z;
      glm_vec3_copy(vector, vert.bitangent);

    } else {
      glm_vec2_copy((vec2){0.0f, 0.0f}, vert.texcoords);
    }

    kv_push(MeshVertex, dest->vertices, vert);
  }

  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    struct aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      kv_push(unsigned int, dest->indices, face.mIndices[j]);
    }
  }

  // materials
  struct aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

  /// Diffuse
  loadMaterialTextures(material, aiTextureType_DIFFUSE, T_DIFFUSE,
                       directory, &dest->textures);
  /// Specular
  loadMaterialTextures(material, aiTextureType_SPECULAR, T_SPECULAR,
  		directory, &dest->textures);

  /// Normal
  loadMaterialTextures(material, aiTextureType_HEIGHT, T_NORMAL,
  		directory, &dest->textures);

  /// Height
  loadMaterialTextures(material, aiTextureType_AMBIENT, T_AMBIENT,
  		directory, &dest->textures);

  meshSetup(dest);
}

static int node_count = 0;
static int mesh_count = 0;;

void processAssimpNode(Model* model, struct aiNode* node,
                       const struct aiScene* scene) {
  node_count++;
	if (node_count % 100 == 0) {
    log_debug("Processed %d nodes, %d meshes so far", node_count, mesh_count);
  }

  log_debug("Processing node %s with %d meshes", node->mName.data, node->mNumMeshes);

  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    struct aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
    mesh_count++;
    Mesh* dest = (kv_pushp(Mesh, model->meshes));

    kv_init(dest->indices);
    kv_init(dest->textures);
    kv_init(dest->vertices);

    processAssimpMesh(mesh, scene, model->directory, dest);
  }

  for (unsigned int i = 0; i < node->mNumChildren; i++) {
  	log_debug("Processing child %d: %s", i, node->mChildren[i]->mName);
    processAssimpNode(model, node->mChildren[i], scene);
  }
}

Result modelLoadFromFile(Model* model, char* path) {
	if (!modelLoaderInitialized) {
		log_error("Attempted to load model when modelLoader not initialized!");
		return Err;
	}
	kv_init(model->meshes);
  const struct aiScene* scene =
      aiImportFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    log_error("Failed to import scene from %s", path);
    return Err;
  }

  model->directory = rSplitOnce(path, "/", 0);
  log_info("using directory %s for model %s", model->directory, path);
  processAssimpNode(model, scene->mRootNode, scene);

  return Ok;
};
