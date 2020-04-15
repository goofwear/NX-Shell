#include <glad/glad.h>

typedef struct {
    GLuint tex_id = 0;
    int width = 0;
    int height = 0;
} GL_Texture;

extern GL_Texture folder_texture, file_texture, archive_texture, audio_texture, image_texture;

void Textures_Load(void);
void Textures_Free(void);
