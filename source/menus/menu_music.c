#include <dirent.h>
#include <switch.h>

#include "common.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_music.h"
#include "SDL_helper.h"
#include "status_bar.h"
#include "textures.h"
#include "touch_helper.h"
#include "utils.h"

#include "mp3.h"

#define MUSIC_STATUS_BAR_COLOUR SDL_MakeColour(97, 97, 97, 255)
#define MUSIC_STATUS_BG_COLOUR  SDL_MakeColour(43, 53, 61, 255)
#define MUSIC_SEPARATOR_COLOUR  SDL_MakeColour(34, 41, 48, 255)

char playlist[255][500];
static int count;

static void Menu_GetMusicList(void)
{
	DIR *dir;
	struct dirent *entries;
	dir = opendir(cwd);

	if (dir != NULL)
	{
		while ((entries = readdir (dir)) != NULL) 
		{
			int length = strlen(entries->d_name);
			if ((strncasecmp(FS_GetFileExt(entries->d_name), "mp3", 3) == 0) || (strncasecmp(FS_GetFileExt(entries->d_name), "ogg", 3) == 0)
					|| (strncasecmp(FS_GetFileExt(entries->d_name), "wav", 3) == 0) || (strncasecmp(FS_GetFileExt(entries->d_name), "mod", 3) == 0))
			{
				strcpy(playlist[count], cwd);
				strcpy(playlist[count] + strlen(playlist[count]), entries->d_name);
				count++;
			}
		}

		closedir(dir);
	}
}

static int Music_GetCurrentIndex(char *path)
{
	for(int i = 0; i < count; ++i)
	{
		if(!strcmp(playlist[i], path))
		{
			return i;
		}
	}
}

static void Music_HandleNext(Mix_Music *audio, bool forward)
{
	Mix_HaltMusic();
	Mix_FreeMusic(audio);

	if (forward)
	{
		if (position < (count + 1))
			position++;
		else
			position = 0;
	}
	else
	{
		if (position > 0)
			position--;
		else
			position = count;
	}

	switch(Mix_GetMusicType(audio))
	{
		case MUS_MP3:
			MP3_Exit();
			break;
	}

	//Mix_CloseAudio();
	Menu_PlayMusic(playlist[position]);
}

static void Music_HandlePause(bool status)
{
	if (status)
	{
		Mix_PauseMusic();
		status = false;
	}
	else
	{
		Mix_ResumeMusic();
		status = true;
	}
}

void Menu_PlayMusic(char *path)
{
	Result ret = 0;

	if (R_FAILED(ret = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096)))
		return;

	Mix_Music *audio = Mix_LoadMUS(path);

	if (audio == NULL)
		return;

	Menu_GetMusicList();

	switch(Mix_GetMusicType(audio))
	{
		case MUS_CMD:
			break;
		case MUS_WAV:
			break;
		case MUS_MID:
			break;
		case MUS_MOD:
		case MUS_OGG:
		case MUS_MP3:
			MP3_Init(path);
			break;
		case MUS_NONE:
			break;
	}

	if (R_FAILED(ret = Mix_PlayMusic(audio, 1)))
		return;

	int title_height = 0;
	TTF_SizeText(Roboto_large, Utils_Basename(path), NULL, &title_height);

	bool isPlaying = true;

	int btn_width = 0, btn_height = 0;
	SDL_QueryTexture(btn_pause, NULL, NULL, &btn_width, &btn_height);

	TouchInfo touchInfo;
	Touch_Init(&touchInfo);

	int position = Music_GetCurrentIndex(path);

	while(appletMainLoop())
	{
		SDL_ClearScreen(RENDERER, MUSIC_STATUS_BG_COLOUR);
		SDL_RenderClear(RENDERER);

		SDL_DrawRect(RENDERER, 0, 0, 1280, 40, MUSIC_STATUS_BAR_COLOUR); // Status bar
		SDL_DrawRect(RENDERER, 0, 40, 1280, 100, MUSIC_STATUS_BG_COLOUR); // Menu bar
		SDL_DrawRect(RENDERER, 0, 140, 1280, 1, MUSIC_SEPARATOR_COLOUR); // Separator

		SDL_DrawImage(RENDERER, icon_back, 40, 66, 48, 48);

		SDL_DrawText(Roboto_large, 128, 40 + ((100 - title_height)/2), WHITE, strlen(ID3.title) == 0? strupr(Utils_Basename(path)) : strupr(ID3.title)); // Audio filename

		SDL_DrawRect(RENDERER, 0, 141, 560, 560, SDL_MakeColour(158, 158, 158, 255)); // Draw album art background
		SDL_DrawImage(RENDERER, default_artwork, 0, 141, 560, 560); // Default album art

		SDL_DrawRect(RENDERER, 570, 141, 710, 559, SDL_MakeColour(45, 48, 50, 255)); // Draw info box (outer)
		SDL_DrawRect(RENDERER, 575, 146, 700, 549, SDL_MakeColour(46, 49, 51, 255)); // Draw info box (inner)

		if (strlen(ID3.artist) != 0)
			SDL_DrawText(Roboto_large, 590, 161, WHITE, ID3.artist);
		if (strlen(ID3.album) != 0)
			SDL_DrawText(Roboto_large, 590, 201, WHITE, ID3.album);
		if (strlen(ID3.year) != 0)
			SDL_DrawText(Roboto_large, 590, 241, WHITE, ID3.year);
		if (strlen(ID3.genre) != 0)
			SDL_DrawText(Roboto_large, 590, 281, WHITE, ID3.genre);

		SDL_DrawCircle(RENDERER, 615 + ((710 - btn_width) / 2), 186 + ((559 - btn_height) / 2), 80, SDL_MakeColour(98, 100, 101, 255)); // Render outer circle
		SDL_DrawCircle(RENDERER, (615 + ((710 - btn_width) / 2)), (186 + ((559 - btn_height) / 2)), 60, SDL_MakeColour(46, 49, 51, 255)); // Render inner circle

		if (isPlaying)
			SDL_DrawImage(RENDERER, btn_pause, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Playing
		else
			SDL_DrawImage(RENDERER, btn_play, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Paused

		SDL_DrawImage(RENDERER, btn_rewind, (570 + ((710 - btn_width) / 2)) - (btn_width * 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Rewind
		SDL_DrawImage(RENDERER, btn_forward, (570 + ((710 - btn_width) / 2)) + (btn_width * 2), 141 + ((559 - btn_height) / 2), btn_width, btn_height); // Forward

		StatusBar_DisplayTime();

		SDL_RenderPresent(RENDERER);

		hidScanInput();
		Touch_Process(&touchInfo);
		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);

		if ((kDown & KEY_B) || (!Mix_PlayingMusic()))
		{
			Mix_HaltMusic();
			break;
		}

		if (kDown & KEY_A)
			Music_HandlePause(isPlaying);

		if (kDown & KEY_LEFT)
			Music_HandleNext(audio, false);
		else if (kDown & KEY_RIGHT)
			Music_HandleNext(audio, true);

		if (touchInfo.state == TouchEnded && touchInfo.tapType != TapNone)
		{
			if (tapped_inside(touchInfo, 40, 66, 108, 114))
			{
				Mix_HaltMusic();
				break;
			}

			if (tapped_inside(touchInfo, 570 + ((710 - btn_width) / 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2)) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height)))
				Music_HandlePause(isPlaying);
			else if (tapped_inside(touchInfo, (570 + ((710 - btn_width) / 2)) - (btn_width * 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2)) - (btn_width * 2) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height)))
				Music_HandleNext(audio, false);
			else if (tapped_inside(touchInfo, (570 + ((710 - btn_width) / 2)) + (btn_width * 2), 141 + ((559 - btn_height) / 2), (570 + ((710 - btn_width) / 2)) + (btn_width * 2) + btn_width, (141 + ((559 - btn_height) / 2) + btn_height)))
				Music_HandleNext(audio, true);
		}
	}

	Mix_FreeMusic(audio);

	switch(Mix_GetMusicType(audio))
	{
		case MUS_MP3:
			MP3_Exit();
			break;
	}

	Mix_CloseAudio();

	MENU_DEFAULT_STATE = MENU_STATE_HOME;
}