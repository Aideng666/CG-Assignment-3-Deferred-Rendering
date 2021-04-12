//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

//TODO: New for this tutorial
#include <DirectionalLight.h>
#include <PointLight.h>
#include <UniformBuffer.h>
/////////////////////////////

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	bool drawPositionBuffer = false;
	bool drawColorBuffer = false;
	bool drawNormalBuffer = false;
	bool drawIllumBuffer = false;

	float t = 0.0f;
	float totalTime;

	float speed = 4.0f;

	bool showVolumes = true;

	glm::vec3 point1 = glm::vec3(-7.0f, 0.0f, 0.0f);
	glm::vec3 point2 = glm::vec3(7.0f, 0.0f, 0.0f);

	glm::vec3 currentPos = glm::vec3(0.0f, 0.0f, 0.0f);

	bool forwards = true;

	float distance = glm::distance(point2, point1);

	totalTime = distance / speed;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);
	
	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	
	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		Shader::sptr simpleDepthShader = Shader::Create();
		simpleDepthShader->LoadShaderPartFromFile("shaders/simple_depth_vert.glsl", GL_VERTEX_SHADER);
		simpleDepthShader->LoadShaderPartFromFile("shaders/simple_depth_frag.glsl", GL_FRAGMENT_SHADER);
		simpleDepthShader->Link();

		//Init gBuffer shader
		Shader::sptr gBufferShader = Shader::Create();
		gBufferShader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		gBufferShader->LoadShaderPartFromFile("shaders/gBuffer_pass_frag.glsl", GL_FRAGMENT_SHADER);
		gBufferShader->Link();
		
		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		//Directional Light Shader
		shader->LoadShaderPartFromFile("shaders/directional_blinn_phong_frag.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		////Creates our directional Light
		//DirectionalLight theSun;
		//UniformBuffer directionalLightBuffer;

		////Allocates enough memory for one directional light (we can change this easily, but we only need 1 directional light)
		//directionalLightBuffer.AllocateMemory(sizeof(DirectionalLight));
		////Casts our sun as "data" and sends it to the shader
		//directionalLightBuffer.SendData(reinterpret_cast<void*>(&theSun), sizeof(DirectionalLight));

		//directionalLightBuffer.Bind(0);

		//Basic effect for drawing to
		PostEffect* basicEffect;
		Framebuffer* shadowBuffer;
		GBuffer* gBuffer;
		IlluminationBuffer* illuminationBuffer;

		//Post Processing Effects
		int activeEffect = 0;
		std::vector<PostEffect*> effects;
		SepiaEffect* sepiaEffect;
		GreyscaleEffect* greyscaleEffect;
		ColorCorrectEffect* colorCorrectEffect;
		BloomEffect* bloomEffect;
		Pixelate* pixelateEffect;
		FilmGrain* filmGrainEffect;

		
		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Effect controls"))
			{

				ImGui::SliderInt("Chosen Effect", &activeEffect, 0, effects.size() - 1);

				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: Sepia Effect");

					SepiaEffect* temp = (SepiaEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Greyscale Effect");
					
					GreyscaleEffect* temp = (GreyscaleEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Color Correct Effect");

					ColorCorrectEffect* temp = (ColorCorrectEffect*)effects[activeEffect];
					static char input[BUFSIZ];
					ImGui::InputText("Lut File to Use", input, BUFSIZ);

					if (ImGui::Button("SetLUT", ImVec2(200.0f, 40.0f)))
					{
						temp->SetLUT(LUT3D(std::string(input)));
					}
				}
				if (activeEffect == 3)
				{
					ImGui::Text("Active Effect: Bloom Effect");

					BloomEffect* temp = (BloomEffect*)effects[activeEffect];
					float intensity = temp->GetThreshold();
					float blur = temp->GetDownscale();

					if (ImGui::SliderFloat("Threshold", &intensity, 0.0f, 1.0f))
					{
						temp->SetThreshold(intensity);
					}

					if (ImGui::SliderFloat("Blur", &blur, 1.0f, 5.0f))
					{
						temp->SetDownscale(blur);
					}
				}
				if (activeEffect == 4)
				{
					ImGui::Text("Active Effect: Film Grain Effect");

					FilmGrain* temp = (FilmGrain*)effects[activeEffect];
					float intensity = temp->GetStrength();

					if (ImGui::SliderFloat("Strength", &intensity, 0.0f, 64.0f))
					{
						temp->SetStrength(intensity);
					}
				}
				if (activeEffect == 5)
				{
					ImGui::Text("Active Effect: Pixelate Effect");

					Pixelate* temp = (Pixelate*)effects[activeEffect];
					float intensity = temp->GetPixelSize();

					if (ImGui::SliderFloat("Pixel Size", &intensity, 0.01f, 32.0f))
					{
						temp->SetPixelSize(intensity);
					}
				}
			}
			if (ImGui::CollapsingHeader("Environment generation"))
			{
				if (ImGui::Button("Regenerate Environment", ImVec2(200.0f, 40.0f)))
				{
					EnvironmentGenerator::RegenerateEnvironment();
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Direction/Position", glm::value_ptr(illuminationBuffer->GetLightRef()[0]._lightPos), 0.01f, -10.0f, 10.0f)) 
				{
				}
			}

			ImGui::Checkbox("Show Light Volumes", &showVolumes);

			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");
		
			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

		///////////////////////////////////// Texture Loading //////////////////////////////////////////////////
		#pragma region Texture

		// Load some textures from files
		Texture2D::sptr chickenDif = Texture2D::LoadFromFile("images/ChickenTex.png");
		Texture2D::sptr buttonDif = Texture2D::LoadFromFile("images/ButtonTex.png");
		Texture2D::sptr coilOffDif = Texture2D::LoadFromFile("images/TeslaCoilOffTex.png");
		Texture2D::sptr coilOnDif = Texture2D::LoadFromFile("images/TeslaCoilOnTex.png");
		Texture2D::sptr floorDif = Texture2D::LoadFromFile("images/FloorTexture.jpg");
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr noSpec = Texture2D::LoadFromFile("images/grassSpec.png");
		Texture2D::sptr boxDif = Texture2D::LoadFromFile("images/BoxTex.png");
		Texture2D::sptr icoDif = Texture2D::LoadFromFile("images/BoxTex.png");
		LUT3D testCube("cubes/BrightenedCorrection.cube");

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion
		//////////////////////////////////////////////////////////////////////////////////////////

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr chickenMat = ShaderMaterial::Create();  
		chickenMat->Shader = gBufferShader;
		chickenMat->Set("s_Diffuse", chickenDif);
		chickenMat->Set("s_Specular", noSpec);
		chickenMat->Set("u_Shininess", 2.0f);
		chickenMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr buttonMat = ShaderMaterial::Create();
		buttonMat->Shader = gBufferShader;
		buttonMat->Set("s_Diffuse", buttonDif);
		buttonMat->Set("s_Specular", noSpec);
		buttonMat->Set("u_Shininess", 2.0f);
		buttonMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr coilOffMat = ShaderMaterial::Create();
		coilOffMat->Shader = gBufferShader;
		coilOffMat->Set("s_Diffuse", coilOffDif);
		coilOffMat->Set("s_Specular", noSpec);
		coilOffMat->Set("u_Shininess", 8.0f);
		coilOffMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr coilOnMat = ShaderMaterial::Create();
		coilOnMat->Shader = gBufferShader;
		coilOnMat->Set("s_Diffuse", coilOnDif);
		coilOnMat->Set("s_Specular", noSpec);
		coilOnMat->Set("u_Shininess", 8.0f);
		coilOnMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr floorMat = ShaderMaterial::Create();
		floorMat->Shader = gBufferShader;
		floorMat->Set("s_Diffuse", floorDif);
		floorMat->Set("s_Specular", noSpec);
		floorMat->Set("u_Shininess", 8.0f);
		floorMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr grassMat = ShaderMaterial::Create();
		grassMat->Shader = gBufferShader;
		grassMat->Set("s_Diffuse", grass);
		grassMat->Set("s_Specular", noSpec);
		grassMat->Set("u_Shininess", 8.0f);
		grassMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr boxMat = ShaderMaterial::Create();
		boxMat->Shader = gBufferShader;
		boxMat->Set("s_Diffuse", boxDif);
		boxMat->Set("s_Specular", noSpec);
		boxMat->Set("u_Shininess", 8.0f);
		boxMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr icoMat = ShaderMaterial::Create();
		icoMat->Shader = gBufferShader;
		icoMat->Set("s_Diffuse", icoDif);
		icoMat->Set("s_Specular", noSpec);
		icoMat->Set("u_Shininess", 8.0f);
		icoMat->Set("u_TextureMix", 0.0f);


		GameObject obj1 = scene->CreateEntity("Ground"); 
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plane.obj");
			obj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(floorMat);
		}

		GameObject obj2 = scene->CreateEntity("monkey_quads");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey_quads.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(chickenMat);
			obj2.get<Transform>().SetLocalPosition(1000.0f, 1000.0f, 1000.0f);
			obj2.get<Transform>().SetLocalRotation(0.0f, 0.0f, -90.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2); 
		}

		GameObject obj3 = scene->CreateEntity("Chicken");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Chicken.obj");
			obj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(chickenMat);
			obj3.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj3.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
		}

		GameObject obj4 = scene->CreateEntity("Tesla Coil");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TeslaCoil.obj");
			obj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(coilOffMat);
			obj4.get<Transform>().SetLocalPosition(0.0f, -5.0f, 0.0f);
			obj4.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj4.get<Transform>().SetLocalScale(2.0f, 2.0f, 2.0f);
		}

		GameObject obj5 = scene->CreateEntity("Button 1");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Button.obj");
			obj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buttonMat);
			obj5.get<Transform>().SetLocalPosition(-8.0f, 0.0f, 0.0f);
			obj5.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj5.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
		}

		GameObject obj6 = scene->CreateEntity("Button 2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Button.obj");
			obj6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(buttonMat);
			obj6.get<Transform>().SetLocalPosition(8.0f, 0.0f, 0.0f);
			obj6.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj6.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
		}

		GameObject obj7 = scene->CreateEntity("Box 1");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Box.obj");
			obj7.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj7.get<Transform>().SetLocalPosition(-5.0f, -4.0f, 1.3f);
			obj7.get<Transform>().SetLocalRotation(0.0f, 0.0f, 45.0f);
			obj7.get<Transform>().SetLocalScale(0.4f, 0.4f, 0.4f);
		}

		GameObject obj8 = scene->CreateEntity("Box 2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Box.obj");
			obj8.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj8.get<Transform>().SetLocalPosition(5.0f, -4.0f, 1.3f);
			obj8.get<Transform>().SetLocalRotation(0.0f, 0.0f, .0f);
			obj8.get<Transform>().SetLocalScale(0.4f, 0.4f, 0.4f);
		}

		GameObject obj9 = scene->CreateEntity("Box 3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/Box.obj");
			obj9.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj9.get<Transform>().SetLocalPosition(5.0f, -4.0f, 3.3f);
			obj9.get<Transform>().SetLocalRotation(90.0f, 0.0f, 45.0f);
			obj9.get<Transform>().SetLocalScale(0.18f, 0.18f, 0.18f);
		}

		MeshBuilder<VertexPosNormTexCol> lightVolumeMesh;
		MeshFactory::AddIcoSphere(lightVolumeMesh, glm::vec3(0.0f), 1.0f);
		MeshFactory::InvertFaces(lightVolumeMesh);
		VertexArrayObject::sptr lightVolumeVAO = lightVolumeMesh.Bake();

		std::vector<GameObject> lightVolumes;
		for (int i = 0; i < 5; i++)
		{
			GameObject lightVolumeObj = scene->CreateEntity("Light Volume");
			//lightVolumeObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			lightVolumeObj.get_or_emplace<RendererComponent>().SetMesh(lightVolumeVAO).SetMaterial(icoMat).SetCastShadow(false);

			lightVolumes.push_back(lightVolumeObj);
		}

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);

		GameObject gBufferObject = scene->CreateEntity("G Buffer");
		{
			gBuffer = &gBufferObject.emplace<GBuffer>();
			gBuffer->Init(width, height);
		}

		GameObject illuminationbufferObject = scene->CreateEntity("Illumination Buffer");
		{
			illuminationBuffer = &illuminationbufferObject.emplace<IlluminationBuffer>();
			illuminationBuffer->Init(width, height);
		}

		int shadowWidth = 4096;
		int shadowHeight = 4096;

		GameObject shadowBufferObject = scene->CreateEntity("Shadow Buffer");
		{
			shadowBuffer = &shadowBufferObject.emplace<Framebuffer>();
			shadowBuffer->AddDepthTarget();
			shadowBuffer->Init(shadowWidth, shadowHeight);
		}

		GameObject framebufferObject = scene->CreateEntity("Basic Effect");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
			sepiaEffect->SetIntensity(0.0f);
		}
		effects.push_back(sepiaEffect);

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);
		
		GameObject colorCorrectEffectObject = scene->CreateEntity("Color Correct Effect");
		{
			colorCorrectEffect = &colorCorrectEffectObject.emplace<ColorCorrectEffect>();
			colorCorrectEffect->Init(width, height);
		}
		effects.push_back(colorCorrectEffect);

		GameObject bloomEffectObject = scene->CreateEntity("Bloom Effect");
		{
			bloomEffect = &bloomEffectObject.emplace<BloomEffect>();
			bloomEffect->Init(width, height);
		}
		effects.push_back(bloomEffect);

		GameObject filmGrainEffectObject = scene->CreateEntity("Film Grain Effect");
		{
			filmGrainEffect = &filmGrainEffectObject.emplace<FilmGrain>();
			filmGrainEffect->Init(width, height);
		}
		effects.push_back(filmGrainEffect);

		GameObject pixelateEffectObject = scene->CreateEntity("Pixelate Effect");
		{
			pixelateEffect = &pixelateEffectObject.emplace<Pixelate>();
			pixelateEffect->Init(width, height);
		}
		effects.push_back(pixelateEffect);

		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			//skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat).SetCastShadow(false);
		
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });
			
			//Toggles drawing specific buffers
			keyToggles.emplace_back(GLFW_KEY_1, [&]() { drawPositionBuffer = false; drawNormalBuffer = false; drawColorBuffer = false; drawIllumBuffer = false; });
			keyToggles.emplace_back(GLFW_KEY_3, [&]() { drawPositionBuffer = !drawPositionBuffer; drawNormalBuffer = false; drawColorBuffer = false; drawIllumBuffer = false; });
			keyToggles.emplace_back(GLFW_KEY_4, [&]() { drawNormalBuffer = !drawNormalBuffer; drawPositionBuffer = false; drawColorBuffer = false; drawIllumBuffer = false; });
			keyToggles.emplace_back(GLFW_KEY_5, [&]() { drawColorBuffer = !drawColorBuffer; drawPositionBuffer = false; drawNormalBuffer = false; drawIllumBuffer = false; });
			keyToggles.emplace_back(GLFW_KEY_6, [&]() { drawIllumBuffer = !drawIllumBuffer; drawPositionBuffer = false; drawNormalBuffer = false; drawColorBuffer = false; });

			controllables.push_back(obj2);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}

		float constant = 1.0f;
		float linear = 0.7f;
		float quadratic = 1.8f;
		float lightMax = std::fmaxf(std::fmaxf(illuminationBuffer->GetLightRef()[0]._lightCol.r, illuminationBuffer->GetLightRef()[0]._lightCol.g), illuminationBuffer->GetLightRef()[0]._lightCol.b);
		float radius =	(-linear + std::sqrtf(linear * linear - 4.0f * quadratic * (constant - (256.0f / 5.0f) * lightMax))) / (2.0f * quadratic);

		//illuminationBuffer->

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			if (forwards)
				t += time.DeltaTime / totalTime;
			else
				t -= time.DeltaTime / totalTime;

			if (t < 0.0f)
				t = 0.0f;

			if (t > 1.0f)
				t = 1.0f;

			if (t >= 1.0f || t <= 0.0f)
				forwards = !forwards;

			currentPos = glm::mix(point1, point2, t);
			
			obj3.get<Transform>().SetLocalPosition(currentPos.x, currentPos.y, currentPos.z);

			for (int i = 0; i < lightVolumes.size(); i++)
			{
				lightVolumes[i].get<Transform>().SetLocalPosition(illuminationBuffer->GetLightRef()[0]._lightPos);
				lightVolumes[i].get<Transform>().SetLocalScale(glm::vec3(radius));
			}

			if (forwards)
			{
				obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 90.0f);
				obj4.get<RendererComponent>().SetMaterial(coilOffMat);
			}
			else
			{
				obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
				obj4.get<RendererComponent>().SetMaterial(coilOnMat);
			}

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			// Clear the screen
			basicEffect->Clear();
			/*greyscaleEffect->Clear();
			sepiaEffect->Clear();*/
			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear();
			}
			shadowBuffer->Clear();
			gBuffer->Clear();
			illuminationBuffer->Clear();


			glClearColor(1.0f, 1.0f, 1.0f, 0.3f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});

			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			//Set up light space matrix
			glm::mat4 lightProjectionMatrix = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, -30.0f, 30.0f);
			glm::mat4 lightViewMatrix = glm::lookAt(glm::vec3(-illuminationBuffer->GetLightRef()[0]._lightPos), glm::vec3(), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightSpaceViewProj = lightProjectionMatrix * lightViewMatrix;

			//Set Shadow Stuff
			illuminationBuffer->SetLightSpaceViewProj(lightSpaceViewProj);
			glm::vec3 camPos = glm::inverse(view) * glm::vec4(0, 0, 0, 1);
			illuminationBuffer->SetCamPos(camPos);

			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;

				return false;
			});

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			glViewport(0, 0, shadowWidth, shadowHeight);
			shadowBuffer->Bind();

			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// Render the mesh
				if (renderer.CastShadows)
				{
					BackendHandler::RenderVAO(simpleDepthShader, renderer.Mesh, viewProjection, transform, lightSpaceViewProj);
				}
			});

			shadowBuffer->Unbind();

			glfwGetWindowSize(BackendHandler::window, &width, &height);

			glViewport(0, 0, width, height);
			gBuffer->Bind();
			// Iterate over the render group components and draw them
			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}

				if (renderer.Material == icoMat)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

				// Render the mesh
				if (renderer.Material == icoMat && !showVolumes)
				{

				}
				else
				{
					BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform, lightSpaceViewProj);
				}
				
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			});

			//shadowBuffer->UnbindTexture(30);
			gBuffer->Unbind();

			illuminationBuffer->BindBuffer(0);

			skybox->Bind();
			BackendHandler::SetupShaderForFrame(skybox, view, projection);
			skyboxMat->Apply();
			BackendHandler::RenderVAO(skybox, meshVao, viewProjection, skyboxObj.get<Transform>(), lightSpaceViewProj);
			skybox->UnBind();

			illuminationBuffer->UnbindBuffer();

			shadowBuffer->BindDepthAsTexture(30);

			illuminationBuffer->ApplyEffect(gBuffer);

			shadowBuffer->UnbindTexture(30);

			if (drawPositionBuffer)
				gBuffer->DrawPositionBuffer();
			else if (drawNormalBuffer)
				gBuffer->DrawNormalBuffer();
			else if (drawColorBuffer)
				gBuffer->DrawColorBuffer();
			else if (drawIllumBuffer)
				illuminationBuffer->DrawIllumBuffer();
			else
			{
				effects[activeEffect]->ApplyEffect(illuminationBuffer);
				effects[activeEffect]->DrawToScreen();
			}
				//illuminationBuffer->DrawToScreen();
			
			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}
		//directionalLightBuffer.Unbind(0);

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		//Clean up the environment generator so we can release references
		EnvironmentGenerator::CleanUpPointers();
		BackendHandler::ShutdownImGui();
	}	



	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}