#pragma once

#include "gui.h"
#include <geGL/OpenGLContext.h>

class Stats : public ge::gl::Context{
public:
	Stats(int history=300);
	~Stats();

	void startFrame();
	void endFrame();

	float getAvg();
	float getMax();
	float getMin();
	void draw();

protected:
	GLuint queries[2];
	float* data = 0;
	int history;
	int frames = 0;
};