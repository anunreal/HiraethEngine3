#include "heRenderer.h"

void heRenderD3Instance(HeD3Instance* instance, const HeD3Camera* camera) {

	// load camera
	heBindShader(instance->material.shader);
	heLoadShaderUniform(instance->material.shader, "u_viewMat", camera->viewMatrix);
	heLoadShaderUniform(instance->material.shader, "u_projMat", camera->projectionMatrix);

	// TODO(Victor): load material to shader
	heBindTexture(instance->material.diffuseTexture, heGetShaderSamplerLocation(instance->material.shader, "t_diffuse", 0));

	heLoadShaderUniform(instance->material.shader, "u_transMat", hm::createTransformationMatrix(instance->transformation.position,
		instance->transformation.rotation, instance->transformation.scale));
	heBindVao(instance->mesh);
	heRenderVao(instance->mesh);

}
