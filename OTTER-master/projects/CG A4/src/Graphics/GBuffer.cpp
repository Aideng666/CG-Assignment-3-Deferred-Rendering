#include "GBuffer.h"

void GBuffer::Init(unsigned width, unsigned height)
{
	//Stores window width and window height
	_windowWidth = width;
	_windowHeight = height;

	//Adds color target to GBuffer
	_gBuffer.AddColorTarget(GL_RGBA8); // Albedo buffer needs all channels
	_gBuffer.AddColorTarget(GL_RGB8);  //Normals buffer, doesnt need alpha
	_gBuffer.AddColorTarget(GL_RGB8);  //Specular buffer, only needs one channel\

	//Important: You can obtain the positional data using the depth buffer (There's a calculation that you can do),
	//But here, we're going to use POSITION buffer
	_gBuffer.AddColorTarget(GL_RGB32F);

	//Add a depth buffer
	_gBuffer.AddDepthTarget();

	//Inits Frame Buffer
	_gBuffer.Init(width, height);

	//Inits pass through shader
	_passThrough = Shader::Create();
	_passThrough->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_passThrough->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
	_passThrough->Link();
}

void GBuffer::Bind()
{
	_gBuffer.Bind();
}

void GBuffer::BindLighting()
{
	_gBuffer.BindColorAsTexture(Target::ALBEDO, 0);
	_gBuffer.BindColorAsTexture(Target::NORMAL, 1);
	_gBuffer.BindColorAsTexture(Target::SPECULAR, 2);
	_gBuffer.BindColorAsTexture(Target::POSITION, 3);
}

void GBuffer::Clear()
{
	_gBuffer.Clear();
}

void GBuffer::Unbind()
{
	_gBuffer.Unbind();
}

void GBuffer::UnbindLighting()
{
	_gBuffer.UnbindTexture(0);
	_gBuffer.UnbindTexture(1);
	_gBuffer.UnbindTexture(2);
	_gBuffer.UnbindTexture(3);
}

void GBuffer::DrawBuffersToScreen()
{
	//Binds passthrough shader
	_passThrough->Bind();

	//Set Viewport to top left
	glViewport(0, _windowHeight / 2.0f, _windowWidth / 2.f, _windowHeight / 2.f);
	_gBuffer.BindColorAsTexture(Target::ALBEDO, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);

	//Set Viewport to top right
	glViewport(_windowWidth / 2.f, _windowHeight / 2.0f, _windowWidth / 2.f, _windowHeight / 2.f);
	_gBuffer.BindColorAsTexture(Target::NORMAL, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);

	//Set Viewport to bottom Left
	glViewport(0, 0, _windowWidth / 2.f, _windowHeight / 2.f);
	_gBuffer.BindColorAsTexture(Target::SPECULAR, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);

	//Set Viewport to bottom right
	glViewport(_windowWidth / 2.f, 0, _windowWidth / 2.f, _windowHeight / 2.f);
	_gBuffer.BindColorAsTexture(Target::POSITION, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);

	//Unbind Passthrough shader
	_passThrough->UnBind();
}

void GBuffer::DrawPositionBuffer()
{
	//Binds passthrough shader
	_passThrough->Bind();

	_gBuffer.BindColorAsTexture(Target::POSITION, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);

	//Unbind Passthrough shader
	_passThrough->UnBind();
}

void GBuffer::DrawNormalBuffer()
{
	//Binds passthrough shader
	_passThrough->Bind();

	_gBuffer.BindColorAsTexture(Target::NORMAL, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);

	//Unbind Passthrough shader
	_passThrough->UnBind();
}

void GBuffer::DrawColorBuffer()
{
	//Binds passthrough shader
	_passThrough->Bind();

	_gBuffer.BindColorAsTexture(Target::ALBEDO, 0);
	_gBuffer.DrawFullscreenQuad();
	_gBuffer.UnbindTexture(0);

	//Unbind Passthrough shader
	_passThrough->UnBind();
}

void GBuffer::Reshape(unsigned width, unsigned height)
{
	//Stores new width and height
	_windowWidth = width;
	_windowHeight = height;

	//Reshapes the framebuffer
	_gBuffer.Reshape(width, height);
}
