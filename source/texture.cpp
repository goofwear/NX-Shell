#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glad/glad.h>

#include "imgui.h"
#include "texture.h"

GL_Texture folder_texture, file_texture, archive_texture, audio_texture, image_texture;

// Simple helper function to load an image into a OpenGL texture with common settings
bool Texture_LoadImage(const char *filename, GL_Texture *texture) {
	// Load from file
	int width = 0, height = 0;
	unsigned char *data = stbi_load(filename, &width, &height, NULL, 4);
	if (data == NULL)
		return false;
		
	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);
	
	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	// Upload pixels into texture
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	
	texture->tex_id = image_texture;
	texture->width = width;
	texture->height = height;
	return true;
}

void Textures_Load(void) {
	bool image_ret = Texture_LoadImage("romfs:/archive.png", &archive_texture);
	IM_ASSERT(image_ret);

	image_ret = Texture_LoadImage("romfs:/audio.png", &audio_texture);
	IM_ASSERT(image_ret);

	image_ret = Texture_LoadImage("romfs:/file.png", &file_texture);
	IM_ASSERT(image_ret);
	
	image_ret = Texture_LoadImage("romfs:/folder.png", &folder_texture);
	IM_ASSERT(image_ret);

	image_ret = Texture_LoadImage("romfs:/image.png", &image_texture);
	IM_ASSERT(image_ret);
}

void Textures_Free(void) {
	glDeleteTextures(1, &image_texture.tex_id);
	glDeleteTextures(1, &folder_texture.tex_id);
	glDeleteTextures(1, &file_texture.tex_id);
	glDeleteTextures(1, &audio_texture.tex_id);
	glDeleteTextures(1, &archive_texture.tex_id);
}
