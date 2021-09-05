#include "PongMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

PongMode::PongMode() {

	//set up trail as if ball has been here for 'forever':
	// for (Ball& ball : balls)
	// {
	// 	ball.Ball_trail.clear();
	// 	ball.Ball_trail.emplace_back(ball.Position, ball.Trail_length);
	// 	ball.Ball_trail.emplace_back(ball.Position, 0.0f);
	// }
	
	//ball_trail.clear();
	//ball_trail.emplace_back(ball, trail_length);
	//ball_trail.emplace_back(ball, 0.0f);

	balls.emplace_back(Ball(glm::vec2(0.0f, 0.0f), glm::vec2(-1.0f, 0.0f), 1.3f, 0));

	
	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of PongMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}
}

PongMode::~PongMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool PongMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN && !isEnd)
	{
		glm::vec2 keyValue;
		switch (evt.key.keysym.sym)
		{
			case SDLK_w:
				keyValue = left_paddle + glm::vec2(0.0f, 1.0f);
				break;

			case SDLK_s:
				keyValue = left_paddle + glm::vec2(0.0f, -1.0f);
				break;

			case SDLK_a:
				keyValue = left_paddle + glm::vec2(-1.0f, 0.0f);
				break;
			
			case SDLK_d:
				keyValue = left_paddle + glm::vec2(1.0, 0.0f);
				break;
		}
		
		if (keyValue.y > court_radius.y - getCurrentLeftPaddleLen()) {
			keyValue.y = court_radius.y - getCurrentLeftPaddleLen();
		}
		if (keyValue.y < -court_radius.y + getCurrentLeftPaddleLen()) {
			keyValue.y = -court_radius.y + getCurrentLeftPaddleLen();
		}
		if (keyValue.x > court_radius.x - paddle_radius.x * 6.0f) {
			keyValue.x = court_radius.x - paddle_radius.x * 6.0f;
		}
		if (keyValue.x < -court_radius.x + paddle_radius.x * 2.0f) {
			keyValue.x = -court_radius.x + paddle_radius.x * 2.0f;
		}

		left_paddle.y = keyValue.y;
		left_paddle.x = keyValue.x;
	}


	return false;
}

void PongMode::update(float elapsed) {
	
	static std::mt19937 mt; //mersenne twister pseudo-random number generator

	//----- paddle update -----
	if (!isEnd)
	{ //right player ai:
		ai_offset_update -= elapsed;
		if (ai_offset_update < elapsed) {
			//update again in [0.5,1.0) seconds:
			ai_offset_update = (mt() / float(mt.max())) * 0.5f + 0.5f;
			ai_offset = (mt() / float(mt.max())) * 2.5f - 1.25f;
		}
		if (right_paddle.y < balls[0].Position.y + ai_offset) {
			right_paddle.y = std::min(balls[0].Position.y + ai_offset, right_paddle.y + 2.0f * elapsed);
		} else {
			right_paddle.y = std::max(balls[0].Position.y + ai_offset, right_paddle.y - 2.0f * elapsed);
		}
	}

	//clamp paddles to court:
	right_paddle.y = std::max(right_paddle.y, -court_radius.y + paddle_radius.y);
	right_paddle.y = std::min(right_paddle.y,  court_radius.y - paddle_radius.y);


	//----- ball update -----

	//speed of ball doubles every four points:
	float speed_multiplier = 4.0f * std::pow(1.5f, (left_score) / 10.0f);

	//velocity cap, though (otherwise ball can pass through paddles):
	speed_multiplier = std::min(speed_multiplier, 10.0f);

	for (Ball& ball : balls)
	{
		ball.Position += elapsed * ball.Velocity * speed_multiplier;
	}
	
	
	//---- collision handling ----
	//paddles:
	auto paddle_vs_ball = [this](glm::vec2 const &paddle, Ball &ball, bool isPlayer) {

		//check the paddle len
		glm::vec2 currentPaddle_radius = paddle_radius;
		if (isPlayer) currentPaddle_radius.y = getCurrentLeftPaddleLen();
		
		//compute area of overlap:
		glm::vec2 min = glm::max(paddle - currentPaddle_radius, ball.Position - ball_radius);
		glm::vec2 max = glm::min(paddle + currentPaddle_radius, ball.Position + ball_radius);

		//if no overlap, no collision:
		if (min.x > max.x || min.y > max.y) return;
		else 
		{
			if (isPlayer)
			{
				if (ball.Type != 0) hitTimes++;
				else isKeyBallCollideLeftPaddle = true;

			}
			else if (ball.Type == 0)
			{
				isBallCollideRightPaddle = true;
			}
		}
		
		if (max.x - min.x > max.y - min.y) {
			//wider overlap in x => bounce in y direction:
			if (ball.Position.y > paddle.y) {
				ball.Position.y = paddle.y + currentPaddle_radius.y + ball_radius.y;
				ball.Velocity.y = std::abs(ball.Velocity.y);
			} else {
				ball.Position.y = paddle.y - currentPaddle_radius.y - ball_radius.y;
				ball.Velocity.y = -std::abs(ball.Velocity.y);
			}
		} else {
			//wider overlap in y => bounce in x direction:
			if (ball.Position.x > paddle.x) {
				ball.Position.x = paddle.x + currentPaddle_radius.x + ball_radius.x;
				ball.Velocity.x = std::abs(ball.Velocity.x);
			} else {
				ball.Position.x = paddle.x - currentPaddle_radius.x - ball_radius.x;
				ball.Velocity.x = -std::abs(ball.Velocity.x);
			}
			//warp y velocity based on offset from paddle center:
			float vel = (ball.Position.y - paddle.y) / (currentPaddle_radius.y + ball_radius.y);
			ball.Velocity.y = glm::mix(ball.Velocity.y, vel, 0.75f);
		}
	};

	for (Ball& ball : balls)
	{
		paddle_vs_ball(left_paddle, ball, true);
		paddle_vs_ball(right_paddle,  ball, false);

		//court walls:
		if (ball.Position.y > court_radius.y - ball_radius.y) {
			ball.Position.y = court_radius.y - ball_radius.y;
			if (ball.Velocity.y > 0.0f) {
				ball.Velocity.y = -ball.Velocity.y;
			}
		}
		if (ball.Position.y < -court_radius.y + ball_radius.y) {
			ball.Position.y = -court_radius.y + ball_radius.y;
			if (ball.Velocity.y < 0.0f) {
				ball.Velocity.y = -ball.Velocity.y;
			}
		}

		if (ball.Position.x > court_radius.x - ball_radius.x) {
			ball.Position.x = court_radius.x - ball_radius.x;
			if (ball.Velocity.x > 0.0f) {
				ball.Velocity.x = -ball.Velocity.x;
				//left_score += 1;
			}
		}
		if (ball.Position.x < -court_radius.x + ball_radius.x) {
			ball.Position.x = -court_radius.x + ball_radius.x;
			if (ball.Velocity.x < 0.0f) {
				ball.Velocity.x = -ball.Velocity.x;
				//right_score += 1;
			}
		}

		//----- gradient trails -----

		//age up all locations in ball trail:
		for (auto &t : ball.Ball_trail) {
			t.z += elapsed;
		}
		//store fresh location at back of ball trail:
		ball.Ball_trail.emplace_back(ball.Position, 0.0f);

		//trim any too-old locations from back of trail:
		//NOTE: since trail drawing interpolates between points, only removes back element if second-to-back element is too old:
		while (ball.Ball_trail.size() >= 2 && ball.Ball_trail[1].z > ball.Trail_length) {
			ball.Ball_trail.pop_front();
		}
	}


	if (isBallCollideRightPaddle)
	{
		balls.push_back(Ball(right_paddle + glm::vec2(-0.5f, 0.0f), glm::vec2(-1.0f, -1.0f), 0.2f, 2));
		balls.push_back(Ball(right_paddle + glm::vec2(-0.5f, 0.0f), glm::vec2(-1.0f, 1.0f), 0.2f, 2));
		left_score += 2;
	    isBallCollideRightPaddle = false;
	}

	if (isKeyBallCollideLeftPaddle)
	{
		uint32_t removeCount = std::min(std::max(hitTimes, (uint32_t)1), (uint32_t)3);
		for (uint32_t i = 0; i < removeCount; i++)
		{
			if (balls.size() == 1) break;
			balls.pop_back(); 
		}
		isKeyBallCollideLeftPaddle = false;
	}

	if (isEnd)
	{
		for (Ball& ball : balls)
		{
			ball.Velocity = glm::vec2(0.0f, 0.0f);
		}
	}

	if (hitTimes > 8 && !isEnd)
	{
		isEnd = true;
	}
}

void PongMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x000000ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0x00ff48ff);
	const glm::u8vec4 keyBall_color = HEX_TO_U8VEC4(0xff00fbff);
	const glm::u8vec4 fg_shadow_color = HEX_TO_U8VEC4(0xc8f5bfff);
	const glm::u8vec4 keyBall_shadow_color = HEX_TO_U8VEC4(0xff99fdff);

	const std::vector< glm::u8vec4 > fg_trail_colors = {
		HEX_TO_U8VEC4(0x71fa4b88),
		HEX_TO_U8VEC4(0xa0fa8788),
		HEX_TO_U8VEC4(0xcdf7c188),
	};
	const std::vector< glm::u8vec4 > keyBall_trail_colors = {
		HEX_TO_U8VEC4(0xf046d488),
		HEX_TO_U8VEC4(0xf78de688),
		HEX_TO_U8VEC4(0xfcb3f088),
	};
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.2f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	//shadows for everything (except the trail):

	glm::vec2 s = glm::vec2(+shadow_offset,-shadow_offset);

	draw_rectangle(left_paddle+s, glm::vec2(paddle_radius.x, getCurrentLeftPaddleLen()), keyBall_shadow_color);
	draw_rectangle(right_paddle+s, paddle_radius, fg_shadow_color);

	
	for (Ball& ball : balls)
	{
		const std::vector< glm::u8vec4 >& trail_colors = ball.Type == 0 ? keyBall_trail_colors : fg_trail_colors;
		//ball's trail:
		if (ball.Ball_trail.size() >= 2) {
			//start ti at second element so there is always something before it to interpolate from:
			std::deque< glm::vec3 >::iterator ti = ball.Ball_trail.begin() + 1;
			//draw trail from oldest-to-newest:
			constexpr uint32_t STEPS = 20;
			//draw from [STEPS, ..., 1]:
			for (uint32_t step = STEPS; step > 0; --step) {
				//time at which to draw the trail element:
				float t = step / float(STEPS) * ball.Trail_length;
				//advance ti until 'just before' t:
				while (ti != ball.Ball_trail.end() && ti->z > t) ++ti;
				//if we ran out of recorded tail, stop drawing:
				if (ti == ball.Ball_trail.end()) break;
				//interpolate between previous and current trail point to the correct time:
				glm::vec3 a = *(ti-1);
				glm::vec3 b = *(ti);
				glm::vec2 at = (t - a.z) / (b.z - a.z) * (glm::vec2(b) - glm::vec2(a)) + glm::vec2(a);

				//look up color using linear interpolation:
				//compute (continuous) index:
				float c = (step-1) / float(STEPS-1) * trail_colors.size();
				//split into an integer and fractional portion:
				int32_t ci = int32_t(std::floor(c));
				float cf = c - ci;
				//clamp to allowable range (shouldn't ever be needed but good to think about for general interpolation):
				if (ci < 0) {
					ci = 0;
					cf = 0.0f;
				}
				if (ci > int32_t(trail_colors.size())-2) {
					ci = int32_t(trail_colors.size())-2;
					cf = 1.0f;
				}
				//do the interpolation (casting to floating point vectors because glm::mix doesn't have an overload for u8 vectors):
				glm::u8vec4 color = glm::u8vec4(
					glm::mix(glm::vec4(trail_colors[ci]), glm::vec4(trail_colors[ci+1]), cf)
				);

				//draw:
				draw_rectangle(at, ball_radius * (1.0f - (float)step / (float)STEPS), color);
			}
		}
	}

	//solid objects:

	//walls:
	float unitWidth = 0.04f;
	float gap = 0.01f;
	glm::vec2 unit_radius = glm::vec2(unitWidth / 2.0f, unitWidth / 2.0f);

	uint32_t xCount = wall_radius * 2.0f / (unitWidth + gap);
	uint32_t yCount = (court_radius.y + 2.0f * wall_radius) * 2.0f / (unitWidth + gap);
	float xStart = -court_radius.x - wall_radius;
	float yStart = -court_radius.y - wall_radius * 2.0f;
	for (uint32_t i = 0; i < xCount; i++)
	{
		for(uint32_t j = 0; j < yCount; j++)
		{
			draw_rectangle(glm::vec2(xStart + (unitWidth + gap) * i, yStart + (unitWidth + gap) * j), unit_radius, fg_color);
		}
	}

	xStart = court_radius.x - wall_radius;
	for (uint32_t i = 0; i < xCount; i++)
	{
		for(uint32_t j = 0; j < yCount; j++)
		{
			draw_rectangle(glm::vec2(xStart + (unitWidth + gap) * i, yStart + (unitWidth + gap) * j), unit_radius, fg_color);
		}
	}
	
	xCount = (court_radius.x) * 2.0f / (unitWidth + gap);
	yCount = wall_radius * 2.0f / (unitWidth + gap);
	xStart = -court_radius.x + wall_radius;
	yStart = court_radius.y;
	for (uint32_t i = 0; i < xCount; i++)
	{
		for(uint32_t j = 0; j < yCount; j++)
		{
			draw_rectangle(glm::vec2(xStart + (unitWidth + gap) * i, yStart + (unitWidth + gap) * j), unit_radius, fg_color);
		}
	}

	yStart = -court_radius.y - wall_radius * 2.0f;
	for (uint32_t i = 0; i < xCount; i++)
	{
		for(uint32_t j = 0; j < yCount; j++)
		{
			draw_rectangle(glm::vec2(xStart + (unitWidth + gap) * i, yStart + (unitWidth + gap) * j), unit_radius, fg_color);
		}
	}
	

	//paddles:
	draw_rectangle(left_paddle, glm::vec2(paddle_radius.x, getCurrentLeftPaddleLen()), keyBall_color);
	draw_rectangle(right_paddle, paddle_radius, fg_color);
	

	//ball:
	for (Ball& ball : balls)
	{
		draw_rectangle(ball.Position, ball_radius, ball.Type == 0 ? keyBall_color : fg_color);
	}
	
	//scores:
	glm::vec2 score_radius = glm::vec2(0.1f, 0.1f);
	for (uint32_t i = 0; i < left_score; ++i) {
		draw_rectangle(glm::vec2( -court_radius.x + (2.0f + 3.0f * i) * score_radius.x, court_radius.y + 2.0f * wall_radius + 2.0f * score_radius.y), score_radius, keyBall_color);
	}


	//------ compute court-to-window transform ------

	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-court_radius.x - 2.0f * wall_radius - padding,
		-court_radius.y - 2.0f * wall_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		court_radius.x + 2.0f * wall_radius + padding,
		court_radius.y + 2.0f * wall_radius + 3.0f * score_radius.y + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);
	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, white_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.

}

float PongMode::getCurrentLeftPaddleLen()
{
	return std::max(0.0f, paddle_radius.y - 0.1f * hitTimes);
}
