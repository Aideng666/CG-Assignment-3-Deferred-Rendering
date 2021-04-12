#pragma once

#include "PostEffect.h"

	class FilmGrain : public PostEffect
	{
	public:
		//Initializes the framebuffer with extra steps
		//*Sets size
		//*Initializes the framebuffer afterwards
		void Init(unsigned width, unsigned height) override;

		//Applies the effect to this screen
		//*Passes the framebuffer with the texture to apply as a parameter
		void ApplyEffect(PostEffect* buffer) override;

		//Getters
		glm::vec2 GetWindowSize() const;
		float GetStrength() const;
		float GetTime() const;

		//Setters
		void SetStrength(float strength);
		void SetWindowSize(float width, float height);

	private:
		glm::vec2 _windowSize;
		float _strength = 32.f;
		float _time;
	};
