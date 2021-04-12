#include "IlluminationBuffer.h"

void IlluminationBuffer::Init(unsigned width, unsigned height)
{
	//Composite Buffer
	int index = int(_buffers.size());
	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(width, height);

	//Illum buffer
	index = int(_buffers.size());
	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA16F);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(width, height);

	//Loads the directional gBuffer shader
	index = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index]->LoadShaderPartFromFile("shaders/gBuffer_directional_frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index]->Link();

	//Loads the ambient gBuffer shader
	index = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index]->LoadShaderPartFromFile("shaders/gBuffer_ambient_frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index]->Link();

	//Loads the HDR Shader
	index = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index]->LoadShaderPartFromFile("shaders/hdr_frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index]->Link();

	for (int i = 0; i < 5; i++)
	{
		PointLight light;

		light._lightPos.x = ((rand() % 100) / 100.0) * 6.0 - 3.0;
		light._lightPos.y = ((rand() % 100) / 100.0) * 6.0 - 3.0;
		light._lightPos.z = ((rand() % 100) / 100.0) * 6.0 - 3.0;

		light._lightCol.r = ((rand() % 100) / 200.0f) + 0.5;
		light._lightCol.g = ((rand() % 100) / 200.0f) + 0.5;
		light._lightCol.b = ((rand() % 100) / 200.0f) + 0.5;

		lights.push_back(light);
	}

	//Allocates sun buffer
	_sunBuffer.AllocateMemory(sizeof(PointLight) * 5);

	//If sun enabled, send data
	if (_sunEnabled)
	{
		_sunBuffer.SendData(reinterpret_cast<void*>(&lights[0]), sizeof(PointLight));
	//	_sunBuffer.SendData(reinterpret_cast<void*>(&lights[1]), sizeof(PointLight));
	}

	PostEffect::Init(width, height);
}

void IlluminationBuffer::ApplyEffect(GBuffer* gBuffer)
{
	
	_sunBuffer.SendData(reinterpret_cast<void*>(&lights[0]), sizeof(PointLight));
	//_sunBuffer.SendData(reinterpret_cast<void*>(&lights[1]), sizeof(PointLight));
	
	if (_sunEnabled)
	{
		//Binds directional light shader
		_shaders[Lights::DIRECTIONAL]->Bind();
		_shaders[Lights::DIRECTIONAL]->SetUniformMatrix("u_LightSpaceMatrix", _lightSpaceViewProj);
		_shaders[Lights::DIRECTIONAL]->SetUniform("u_CamPos", _camPos);

		float constant = 1.0f;
		float linear = 0.7f;
		float quadratic = 1.8f;
		float lightMax = std::fmaxf(std::fmaxf(lights[0]._lightCol.r, lights[0]._lightCol.g), lights[0]._lightCol.b);
		float radius =
			(-linear + std::sqrtf(linear * linear - 4.0f * quadratic * (constant - (256.0f / 5.0f) * lightMax)))
			/ (2.0f * quadratic);

		_shaders[Lights::DIRECTIONAL]->SetUniform("u_Radius", radius);

		//Send the directional light data and bind it
		_sunBuffer.Bind(0);

		gBuffer->BindLighting();

		//Binds and draws to the illumination buffer
		_buffers[1]->RenderToFSQ();

		gBuffer->UnbindLighting();

		//Unbinds the uniform buffer
		_sunBuffer.Unbind(0);

		//Unbind shader
		_shaders[Lights::DIRECTIONAL]->UnBind();
	}

	//HDR//
	_shaders[2]->Bind();

	float exposure = 5.0f;

	_shaders[2]->SetUniform("u_exposure", exposure);

	_sunBuffer.Bind(0);

	gBuffer->BindLighting();

	_buffers[1]->BindColorAsTexture(0, 0);

	_buffers[1]->RenderToFSQ();

	_buffers[1]->UnbindTexture(0);

	gBuffer->UnbindLighting();
	///////

	//Unbinds uniform buffer
	_sunBuffer.Unbind(0);

	_shaders[2]->UnBind();

	//Bind ambient shader
	_shaders[Lights::AMBIENT]->Bind();

	//Send the direcitonal light data
	_sunBuffer.Bind(0);

	gBuffer->BindLighting();
	_buffers[1]->BindColorAsTexture(0, 4);
	_buffers[0]->BindColorAsTexture(0, 5);

	_buffers[0]->RenderToFSQ();

	_buffers[0]->UnbindTexture(5);
	_buffers[1]->UnbindTexture(4);
	gBuffer->UnbindLighting();

	//Unbinds uniform buffer
	_sunBuffer.Unbind(0);

	_shaders[Lights::AMBIENT]->UnBind();
}

void IlluminationBuffer::DrawIllumBuffer()
{
	_shaders[_shaders.size() - 1]->Bind();

	_buffers[1]->BindColorAsTexture(0, 0);

	Framebuffer::DrawFullscreenQuad();

	_buffers[1]->UnbindTexture(0);

	_shaders[_shaders.size() - 1]->UnBind();
}

void IlluminationBuffer::SetLightSpaceViewProj(glm::mat4 lightSpaceViewProj)
{
	_lightSpaceViewProj = lightSpaceViewProj;
}

void IlluminationBuffer::SetCamPos(glm::vec3 camPos)
{
	_camPos = camPos;
}

DirectionalLight& IlluminationBuffer::GetSunRef()
{
	return _sun;
}

std::vector<PointLight>& IlluminationBuffer::GetLightRef()
{
	return lights;
}

void IlluminationBuffer::SetSun(DirectionalLight newSun)
{
	_sun = newSun;
}

void IlluminationBuffer::SetSun(glm::vec4 lightDir, glm::vec4 lightCol)
{
	_sun._lightDirection = lightDir;
	_sun._lightCol = lightCol;
}

void IlluminationBuffer::EnableSun(bool enabled)
{
	_sunEnabled = enabled;
}
