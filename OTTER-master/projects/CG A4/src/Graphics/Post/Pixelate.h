#pragma once

#include "PostEffect.h"


	class Pixelate : public PostEffect
	{
	public:
		//Initializes the framebuffer with extra steps
		//*Sets size
		//*Initializes the framebuffer afterwards
		void Init(unsigned width, unsigned height) override;

		//Applies the effect to this screen
		//*Pauses the framebuffer with the texture to apply as a parameter
		void ApplyEffect(PostEffect* buffer) override;

		//Getters
		glm::vec2 GetWindowSize() const;
		float GetPixelSize() const;

		//Setters
		void SetPixelSize(float size);
		void SetWindowSize(float width, float height);

	private:
		glm::vec2 _windowSize;
		float _pixelSize = 16.f;
	};
