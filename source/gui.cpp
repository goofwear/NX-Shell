#include <stdio.h>
#include <iostream>
#include "config.h"
#include "fs.h"
#include "gui.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "keyboard.h"
#include "texture.h"
#include "utils.h"

enum MENU_STATES {
	MENU_STATE_HOME = 0,
	MENU_STATE_OPTIONS = 1,
	MENU_STATE_DELETE = 2,
	MENU_STATE_PROPERTIES = 3,
	MENU_STATE_SETTINGS = 4
};

typedef struct {
	int state = 0;
	bool copy = false;
	bool move = false;
	int selected = 0;
	int file_count = 0;
	std::string selected_filename;
	FsDirectoryEntry *entries = nullptr;
} GUI_MenuItem;

static void GUI_RenderOptionsMenu(GUI_MenuItem *item) {
	ImGui::OpenPopup("Options");
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
	if (ImGui::BeginPopupModal("Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (ImGui::Button("Properties", ImVec2(200, 50))) {
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_PROPERTIES;
		}
		ImGui::SameLine(0.0f, 15.0f);
		if (ImGui::Button("Rename", ImVec2(200, 50))) {
			char path[FS_MAX_PATH];
			strcpy(path, Keyboard_GetText("Enter name", item->entries[item->selected].name));
			if (R_SUCCEEDED(FS_Rename(&item->entries[item->selected], path)))
				item->file_count = FS_RefreshEntries(&item->entries, item->file_count);
			
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_HOME;
		}
		
		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
		
		if (ImGui::Button("New Folder", ImVec2(200, 50))) {
			std::string path = config.cwd;
			path += "/";
			std::string name = Keyboard_GetText("Enter folder name", "New folder");
			path += name;
			
			if (R_SUCCEEDED(fsFsCreateDirectory(fs, path.c_str())))
				item->file_count = FS_RefreshEntries(&item->entries, item->file_count);
			
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_HOME;
		}
		ImGui::SameLine(0.0f, 15.0f);
		if (ImGui::Button("New File", ImVec2(200, 50))) {
			std::string path = config.cwd;
			path += "/";
			std::string name = Keyboard_GetText("Enter file name", "New file");
			path += name;
			
			if (R_SUCCEEDED(fsFsCreateFile(fs, path.c_str(), 0, 0)))
				item->file_count = FS_RefreshEntries(&item->entries, item->file_count);
			
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_HOME;
		}
		
		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
		
		if (ImGui::Button(!item->copy? "Copy" : "Paste", ImVec2(200, 50))) {
			if (!item->copy)
				FS_Copy(&item->entries[item->selected]);
			else {
				if (R_SUCCEEDED(FS_Paste()))
					item->file_count = FS_RefreshEntries(&item->entries, item->file_count);
			}
			
			item->copy = !item->copy;
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_HOME;
		}
		ImGui::SameLine(0.0f, 15.0f);
		if (ImGui::Button(!item->move? "Move" : "Paste", ImVec2(200, 50))) {
			if (!item->move)
				FS_Copy(&item->entries[item->selected]);
			else {
				if (R_SUCCEEDED(FS_Move()))
					item->file_count = FS_RefreshEntries(&item->entries, item->file_count);
			}

			item->move = !item->move;
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_HOME;
		}
		
		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
		
		if (ImGui::Button("Delete", ImVec2(200, 50))) {
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_DELETE;
		}
		ImGui::SameLine(0.0f, 15.0f);
		if (ImGui::Button("Set archive bit", ImVec2(200, 50))) {
			if (R_SUCCEEDED(FS_SetArchiveBit(&item->entries[item->selected])))
				item->file_count = FS_RefreshEntries(&item->entries, item->file_count);
			
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_HOME;
		}
		
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();
}

static char *GUI_FormatDate(char *string, time_t timestamp) {
	strftime(string, 36, "%Y/%m/%d %H:%M:%S", localtime(&timestamp));
	return string;
}

static void GUI_RenderPropertiesMenu(GUI_MenuItem *item) {
	ImGui::OpenPopup("Properties");
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
	if (ImGui::BeginPopupModal("Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		std::string name_text = "Name: " + item->selected_filename;
		ImGui::Text(name_text.c_str());
		
		if (item->entries[item->selected].type == FsDirEntryType_File) {
			char size[16];
			Utils_GetSizeString(size, item->entries[item->selected].file_size);
			std::string size_text = "Size: ";
			size_text += size;
			ImGui::Text(size_text.c_str());
		}

		FsTimeStampRaw timestamp;
		if (R_SUCCEEDED(FS_GetTimeStamp(&item->entries[item->selected], &timestamp))) {
			if (timestamp.is_valid == 1) { // Confirm valid timestamp
				char date[3][36];

				std::string created_time = "Created: ";
				created_time += GUI_FormatDate(date[0], timestamp.created);
				ImGui::Text(created_time.c_str());

				std::string modified_time = "Modified: ";
				modified_time += GUI_FormatDate(date[1], timestamp.modified);
				ImGui::Text(modified_time.c_str());

				std::string accessed_time = "Accessed: ";
				accessed_time += GUI_FormatDate(date[2], timestamp.accessed);
				ImGui::Text(accessed_time.c_str());
			}
		}

		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
		
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_OPTIONS;
		}
		
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();
}

static void GUI_RenderDeleteMenu(GUI_MenuItem *item) {
	ImGui::OpenPopup("Delete");
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
	if (ImGui::BeginPopupModal("Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("This action cannot be undone");
		std::string text = "Do you wish to delete " + item->selected_filename + "?";
		ImGui::Text(text.c_str());

		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
		
		if (ImGui::Button("OK", ImVec2(120, 0))) {
			if (R_SUCCEEDED(FS_Delete(&item->entries[item->selected])))
				item->file_count = FS_RefreshEntries(&item->entries, item->file_count);
			
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_HOME;
		}
		
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine(0.0f, 15.0f);
		
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
			item->state = MENU_STATE_OPTIONS;
		}
		
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();
}

static void GUI_RenderSettingsMenu(GUI_MenuItem *item) {
	ImGui::SetNextWindowPos({0.0f, 0.0f}, ImGuiCond_Once);
	ImGui::SetNextWindowSize({1280.0f, 720.0f}, ImGuiCond_Once);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
		if (ImGui::TreeNode("Sort Settings")) {
			ImGui::SetItemDefaultFocus();
			ImGui::RadioButton(" By name (ascending)", &config.sort, 0);
			ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
			ImGui::RadioButton(" By name (descending)", &config.sort, 1);
			ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
			ImGui::RadioButton(" By size (largest first)", &config.sort, 2);
			ImGui::Dummy(ImVec2(0.0f, 10.0f)); // Spacing
			ImGui::RadioButton(" By size (smallest first)", &config.sort, 3);
			ImGui::TreePop();
		}
		ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
		if (ImGui::TreeNode("About")) {
			ImGui::Text("NX-Shell Version: v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			ImGui::Text("ImGui Version: %s",  ImGui::GetVersion());
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			ImGui::Text("Author: Joel16");
			ImGui::Dummy(ImVec2(0.0f, 5.0f)); // Spacing
			ImGui::Text("Banner: Preetisketch");
			ImGui::TreePop();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar();
	Config_Save(config);
	item->file_count = FS_RefreshEntries(&item->entries, item->file_count);
}

int GUI_RenderMainMenu(SDL_Window *window) {
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	GUI_MenuItem item;
	item.state = MENU_STATE_HOME;
	item.selected = 0;
	item.file_count = FS_GetDirList(config.cwd, &item.entries);
	if (item.file_count < 0)
		return -1;
    
	// Main loop
	bool done = false, set_focus = false, first = true;
	while (!done) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_JOYBUTTONDOWN) {
				if (event.jbutton.which == 0) {
					if (event.jbutton.button == 0) {
						if (item.state == MENU_STATE_HOME) {
							if (item.entries[item.selected].type == FsDirEntryType_Dir) {
								if (item.file_count != 0) {
									s64 value = FS_ChangeDirNext(item.entries[item.selected].name, &item.entries, item.file_count);
									if (value >= 0) {
										item.file_count = value;
										GImGui->NavId = 0;
									}
								}
							}
						}
					}
					else if (event.jbutton.button == 1) {
						if (item.state == MENU_STATE_HOME) {
							s64 value = FS_ChangeDirPrev(&item.entries, item.file_count);
							if (value >= 0) {
								item.file_count = value;
								GImGui->NavId = 0;
							}
						}
						else if ((item.state == MENU_STATE_PROPERTIES) || (item.state == MENU_STATE_DELETE))
							item.state = MENU_STATE_OPTIONS;
						else
							item.state = MENU_STATE_HOME;
					}
					else if (event.jbutton.button == 2)
						item.state = MENU_STATE_OPTIONS;
					else if (event.jbutton.button == 11)
						item.state = MENU_STATE_SETTINGS;
					else if (event.jbutton.button == 10)
						done = true;
				}
			}
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}
		
		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		if (ImGui::Begin("NX-Shell", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
			ImGui::SetNextWindowSize({1280.0f, 720.0f}, ImGuiCond_Once);
			
			// Initially set default focus to next window (FS_DirList)
			if (!set_focus) {
				ImGui::SetNextWindowFocus();
				set_focus = true;
			}
			
			// Display current working directory
			ImGui::TextColored(ImVec4(1.00f, 1.00f, 1.00f, 1.00f), config.cwd);
			
			ImGui::BeginChild("##FS_DirList");
			if (item.file_count != 0) {
				for (s64 i = 0; i < item.file_count; i++) {
					if (item.entries[i].type == FsDirEntryType_Dir)
						ImGui::Image((void *)(intptr_t)folder_texture.tex_id, ImVec2(folder_texture.width, folder_texture.height));
					else {
						std::string filename = item.entries[i].name;
						std::string ext = filename.substr(filename.find_last_of(".") + 1);
						if (ext == "zip" || ext == "rar" || ext == "7z")
							ImGui::Image((void *)(intptr_t)archive_texture.tex_id, ImVec2(archive_texture.width, archive_texture.height));
						else if (ext == "flac" || ext == "it" || ext == "mod" || ext == "mp3" || ext == "ogg" || ext == "opus" || ext == "s3m" || ext == "wav" || ext == "xm")
							ImGui::Image((void *)(intptr_t)audio_texture.tex_id, ImVec2(audio_texture.width, audio_texture.height));
						else if (ext == "bmp" || ext == "gif" || ext == "jpg" || ext == "jpeg" || ext == "png")
							ImGui::Image((void *)(intptr_t)image_texture.tex_id, ImVec2(image_texture.width, image_texture.height));
						else
							ImGui::Image((void *)(intptr_t)file_texture.tex_id, ImVec2(file_texture.width, file_texture.height));
					}
					ImGui::SameLine();
					
					ImGui::Selectable(item.entries[i].name);
					
					if (first) {
						ImGui::SetFocusID(ImGui::GetID((item.entries[0].name)), ImGui::GetCurrentWindow());
						ImGuiContext& g = *ImGui::GetCurrentContext();
						g.NavDisableHighlight = false;
						first = false;
					}
					
					if (!ImGui::IsAnyItemFocused())
						GImGui->NavId = GImGui->CurrentWindow->DC.LastItemId;
						
					if (ImGui::IsItemHovered()) {
						item.selected = i;
						item.selected_filename = item.entries[item.selected].name;
					}
				}
			}
			else
				ImGui::Text("No file entries");
			
			ImGui::EndChild();
			
			if (item.state == MENU_STATE_OPTIONS)
				GUI_RenderOptionsMenu(&item);
			else if (item.state == MENU_STATE_PROPERTIES)
				GUI_RenderPropertiesMenu(&item);
			else if (item.state == MENU_STATE_DELETE)
				GUI_RenderDeleteMenu(&item);
			else if (item.state == MENU_STATE_SETTINGS)
				GUI_RenderSettingsMenu(&item);
			
			ImGui::End();
		}
		ImGui::PopStyleVar();
		
		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(0.00f, 0.00f, 0.00f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}
	
	return 0;
}
