#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform OverlayBufferObject {
	float visible;
	int level; // must match fragment shader
} oubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

void main() {
	gl_Position = vec4(inPosition * oubo.visible, 0.5, 1.0);
	fragUV = inUV;
}