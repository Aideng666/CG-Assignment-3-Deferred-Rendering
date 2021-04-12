#include "FilmGrain.h"
#include "GLFW/glfw3.h"

	void FilmGrain::Init(unsigned width, unsigned height)
	{
		//Load the buffers
		int index = int(_buffers.size());
		_buffers.push_back(new Framebuffer());
		_buffers[index]->AddColorTarget(GL_RGBA8);
		//_buffers[index]->AddDepthTarget();
		_buffers[index]->Init(width, height);

		//Load the shaders
		index = int(_shaders.size());
		_shaders.push_back(Shader::Create());
		_shaders[index]->LoadShaderPartFromFile("Shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		_shaders[index]->LoadShaderPartFromFile("Shaders/Post/film_grain_frag.glsl", GL_FRAGMENT_SHADER);
		_shaders[index]->Link();

		_windowSize = glm::vec2(float(width), float(height));
	}

	void FilmGrain::ApplyEffect(PostEffect* buffer)
	{
		_time = glfwGetTime();

		BindShader(0);
		_shaders[0]->SetUniform("u_WindowSize", _windowSize);
		_shaders[0]->SetUniform("u_Strength", _strength);
		_shaders[0]->SetUniform("u_Time", _time);

		buffer->BindColorAsTexture(0, 0, 0);

		_buffers[0]->RenderToFSQ();

		buffer->UnbindTexture(0);

		UnbindShader();
	}

	glm::vec2 FilmGrain::GetWindowSize() const
	{
		return _windowSize;
	}

	float FilmGrain::GetStrength() const
	{
		return _strength;
	}

	float FilmGrain::GetTime() const
	{
		return _time;
	}

	void FilmGrain::SetStrength(float strength)
	{
		_strength = strength;
	}

	void FilmGrain::SetWindowSize(float width, float height)
	{
		_windowSize.x = width;
		_windowSize.y = height;
	}
