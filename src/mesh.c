#include "cglm/cglm.h"
#include "glad.h"
#include "kvec.h"
#include "thing.h"
#include "stdio.h"
#include "utils.h"
#include "log.h"

#include "stdio.h"
#include "stbi_image.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <stddef.h>

typedef struct MeshVertex {
  vec3 pos;
  vec3 normals;
  vec2 texcoords;
  vec3 tangent;
  vec3 bitangent;
} MeshVertex;

enum TEXTURE_TYPE {
  T_SPECULAR,
  T_DIFFUSE,
  T_OTHER,
  T_TYPES,
};

const char* textureNames[T_TYPES] = {
    "texture_specular",
    "texture_diffuse",
    "texture_other",
};

typedef struct MeshTexture {
  unsigned int id;
  int type;
} MeshTexture;

typedef kvec_t(MeshVertex) mVertVec;
typedef kvec_t(MeshTexture) mTexVec;
typedef kvec_t(unsigned int) mIndVec;

typedef struct Mesh {
  mVertVec vertices;
  mTexVec textures;
  mIndVec indices;
  RenderInfo ri;
} Mesh;

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
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, dest->vertices.n * sizeof(unsigned int),
               dest->vertices.a, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (void*)offsetof(MeshVertex, normals));

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                        (void*)offsetof(MeshVertex, texcoords));
}

void meshDraw(Mesh* m) {
  unsigned int diffuseNr = 1;
  unsigned int specularNr = 1;
  char uniform[256];

  for (unsigned int i = 0; i < m->textures.n; i++) {
    glActiveTexture(GL_TEXTURE0 + i);

    switch (m->textures.a[i].type) {
      case (T_DIFFUSE):
        uniform[snprintf(uniform, 256, "material.%s%d", textureNames[T_DIFFUSE],
                         diffuseNr)] = '\0';
        diffuseNr++;
        break;
      case (T_SPECULAR):
        uniform[snprintf(uniform, 256, "material.%s%d",
                         textureNames[T_SPECULAR], specularNr)] = '\0';
        specularNr++;
        break;
      case (T_OTHER):
        break;
    }

    shaderSetInt(m->ri.shader, uniform, (int)i);
    glBindTexture(GL_TEXTURE_2D, m->textures.a[i].id);
  }

  glActiveTexture(GL_TEXTURE0);

  glBindVertexArray(m->ri.vao);
  glDrawElements(GL_TRIANGLES, m->indices.n, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

typedef kvec_t(Mesh) MeshVec;

typedef struct Model {
  MeshVec meshes;
} Model;

char workbuf[1024];

unsigned int textureFromFile(const char *file, const char *directory, bool gamma) {
	workbuf[snprintf(workbuf, 1024, "%s/%s", directory, file)] = '\0';

	unsigned int texid;
	glGenTextures(1, &texid);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(workbuf, &width, &height, &nrComponents, 0);
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
			}
	else if (nrComponents == 4) {
		format = GL_RGBA;
	}

	glBindTexture(GL_TEXTURE_2D, texid);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);

	return texid;
}

void loadMaterialTextures(struct aiMaterial *mat, enum aiTextureType type, const char *name, const char *dir, mTexVec *textures) {
	int texcount = aiGetMaterialTextureCount(mat, type);
	for (unsigned int i = 0; i < texcount; i++) {
		struct aiString str;
		aiGetMaterialTexture(mat, type, i, &str, NULL, NULL, NULL, NULL, NULL, NULL);
		MeshTexture *dest = (kv_pushp(MeshTexture, (*textures)));
		dest->id = textureFromFile(str.data, dir, 0);
		dest->type = type

		// TODO: check for already loaded textures
		/* kv_push(MeshTexture, *textures, */ 

	}
}

void processAssimpMesh(const struct aiMesh *mesh, const struct aiScene *scene, Mesh *dest) {
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
	struct aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

	/// Diffuse
	
	/// Specular
	
	/// Normal
	
	/// Height
};

void processAssimpNode(Model *model, struct aiNode *node, const struct aiScene *scene) {
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		Mesh* dest = (kv_pushp(Mesh, model->meshes));
		processAssimpMesh(mesh, scene, dest);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		processAssimpNode(model, node->mChildren[i], scene);
	}
}

Result modelLoadFromFile(Model *model, const char* path) {
	const struct aiScene *scene = aiImportFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		log_error("Failed to import scene from %s", path);
		return Err;
	}





	return Ok;
};
