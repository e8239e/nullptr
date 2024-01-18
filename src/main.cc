#include <algorithm>
#include <array>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <optional>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"

#include "IosevkaNerdFontMono-Regular.h"
#include "wqy-microhei.h"

#include <cpr/cpr.h>
#include <cpr/api.h>

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

struct upload_state {
	bool failed = false;
	bool succeed = false;
	std::string err_msg = "";
	std::string result_url = "";
};

struct app_settings {
	std::string instance = "https://0x0.st";
	std::optional<std::string> expires = {};
	bool secret = true;
};

static void glfw_error_callback(int error, const char *description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// TODO: save history somewhere in $HOME ?
void gen_history(const std::vector<std::string> &history,
		 std::mutex *history_mutex,
		 std::vector<std::array<std::string, 2> > &result)
{
	auto lock = std::scoped_lock(*history_mutex);
	static std::size_t last_hist_size = 0;
	if (history.size() <= last_hist_size) {
		return;
	}
	last_hist_size = history.size();
	result.clear();
	result.reserve(history.size());
	const auto gen = [&result](auto &e) {
		std::array<std::string, 2> a = { e, "copy##" + e };
		result.push_back(a);
	};
	std::for_each(history.begin(), history.end(), gen);
}

void upload(const app_settings *settings, const std::string *content,
	    upload_state *upload_result, std::mutex *upload_result_mutex,
	    std::vector<std::string> *history, std::mutex *history_mutex)
{
	auto data = cpr::Multipart{
		{ "file", cpr::Buffer{ content->begin(), content->end(), "" } }
	};

	if (settings->secret) {
		data.parts.push_back({ "secret", "1" });
	}
	if (settings->expires.has_value()) {
		data.parts.push_back({ "expires", settings->expires.value() });
	}

	std::cerr << "instance: " << settings->instance << std::endl;
	std::cerr << "secret?: " << settings->secret << std::endl;
	std::cerr << "expires: " << settings->expires.value_or("no")
		  << std::endl;
	auto response = cpr::Post(cpr::Url(settings->instance), data);
	std::cerr << "response code: " << response.status_code << std::endl;
	std::cerr << "response: " << response.text << std::endl;

	auto lock = std::scoped_lock(*upload_result_mutex);
	if (response.status_code >= 200 && response.status_code < 300) {
		upload_result->result_url = response.text;
		upload_result->succeed = true;
		auto lock = std::scoped_lock(*history_mutex);
		history->push_back(upload_result->result_url);
	} else {
		upload_result->failed = true;
		upload_result->err_msg = response.text;
	}
}

int main(int, char **)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) {
		return 1;
	}

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char *glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char *glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE,
		       GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char *glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	GLFWwindow *window =
		glfwCreateWindow(800, 600, "nullptr", nullptr, nullptr);
	if (window == nullptr)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // enable Vsync; fps = SCREEN_REFRESH_RATE / 1

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.IniFilename = nullptr; // don't read & write imgui.ini config
	io.ConfigFlags |=
		ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

	{
		ImVector<ImWchar> MAIN_ranges;
		{
#define FONT_SIZE \
	18.0f // TODO: don't hardcode the font size, calculate it according to the screen resolution

			ImFontGlyphRangesBuilder builder;
			builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
			// clang-format off
            // https://github.com/ryanoasis/nerd-fonts/wiki/Glyph-Sets-and-Code-Points
            static const ImWchar icon_ranges[] = {
                0xe700, 0xe7c5, // Devicons
                0xf000, 0xf2e0, // Font Awesome
                0xe200, 0xe2a9, // Font Awesome Extension
                // 0xf0001, 0xf1af0, // Material Design Icons (don't fit in unsigned short)
                0xe300, 0xe3e3, // Weather
                0xf400, 0xf532, 0x2665, 0x26A1, // Octicons
                0xe0a0, 0xe0a2, 0xe0b0, 0xe0b3, // [Powerline Symbols]
                0xe0a3, 0xe0b4, 0xe0c8, 0xe0ca, 0xe0cc, 0xe0d4, // Powerline Extra Symbols
                0x23fb, 0x23fe, 0x2b58, // IEC Power Symbols
                0xf300, 0xf372, // Font Logos (Formerly Font Linux)
                0xe000, 0xe00a, // Pomicons
                0xea60, 0xebeb, // Codicons
                0x2500, 0x259f, // Additional sets
                0,
            };
			// clang-format on
			builder.AddRanges(icon_ranges);
			builder.BuildRanges(&MAIN_ranges);
		}

		io.Fonts->AddFontFromMemoryCompressedTTF(
			font_IosevkaNerdFontMono_Regular_compressed_data,
			font_IosevkaNerdFontMono_Regular_compressed_size,
			FONT_SIZE, nullptr, MAIN_ranges.Data);

		ImVector<ImWchar> CJK_ranges;

		{
			ImFontGlyphRangesBuilder builder;
			builder.AddRanges(io.Fonts->GetGlyphRangesKorean());
			builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
			builder.AddRanges(
				io.Fonts->GetGlyphRangesChineseFull());
			builder.AddRanges(io.Fonts->GetGlyphRangesThai());
			builder.AddRanges(io.Fonts->GetGlyphRangesVietnamese());
			builder.BuildRanges(&CJK_ranges);
		}

		ImFontConfig config;
		config.MergeMode = true;

		io.Fonts->AddFontFromMemoryCompressedTTF(
			font_wqy_microhei_compressed_data,
			font_wqy_microhei_compressed_size, FONT_SIZE, &config,
			CJK_ranges.Data);

		io.Fonts->Build();
	}

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// State
	app_settings settings;
	std::vector<std::string> upload_history;
	std::mutex upload_history_mutex;
	upload_state upload_result;
	std::mutex upload_result_mutex;
	std::vector<std::array<std::string, 2> > upload_history_button_content;
	std::string upload_text = "";
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		const auto output_history = [](auto &e) {
			ImGui::Text("%s", e[0].c_str());
			ImGui::SameLine();
			if (ImGui::Button(e[1].c_str())) {
				ImGui::SetClipboardText(e[0].c_str());
			}
		};

		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			static ImVec2 window_size;
			window_size = ImGui::GetMainViewport()->Size;
			ImGui::SetNextWindowSizeConstraints(window_size,
							    window_size);
			static const auto flags = ImGuiWindowFlags_NoCollapse |
						  ImGuiWindowFlags_NoResize |
						  ImGuiWindowFlags_NoMove;
			ImGui::Begin("nullptr", nullptr,
				     flags); // nullptr window

			if (ImGui::BeginTabBar("##", ImGuiTabBarFlags_None)) {
				ImGuiIO &io = ImGui::GetIO();
				if (ImGui::BeginTabItem("\uf40a upload")) {
					ImGui::Text(
						"paste text to upload here");
					ImGui::InputTextMultiline(
						"upload_text", &upload_text,
						ImVec2(window_size.x,
						       window_size.y * 0.82));
					if (ImGui::Button(
						    "upload",
						    ImVec2(window_size.x,
							   window_size.y *
								   0.09))) {
						std::thread(
							upload, &settings,
							&upload_text,
							&upload_result,
							&upload_result_mutex,
							&upload_history,
							&upload_history_mutex)
							.detach();
					}
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("\uea82 history")) {
					if (ImGui::BeginListBox(
						    "##",
						    ImVec2(window_size.x,
							   window_size.y *
								   0.93))) {
						gen_history(
							upload_history,
							&upload_history_mutex,
							upload_history_button_content);
						std::for_each(
							upload_history_button_content
								.begin(),
							upload_history_button_content
								.end(),
							output_history);
						ImGui::EndListBox();
					}
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("\ue690 settings")) {
					static std::string expires;
					expires =
						settings.expires.value_or("no");
					ImGui::Text(
						"instance url to upload to: ");
					ImGui::SameLine();
					ImGui::InputText("##instance",
							 &settings.instance);
					ImGui::Text("expires in ");
					ImGui::SameLine();
					ImGui::InputText("##expires", &expires);
					if (expires != "no") {
						settings.expires = expires;
					}
					ImGui::SameLine();
					ImGui::Text("hours");
					ImGui::Text(
						"request a hard to guess url: ");
					ImGui::SameLine();
					ImGui::Checkbox("##secret",
							&settings.secret);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			ImGui::End(); // nullptr window
		}

		{
			auto lock = std::scoped_lock(upload_result_mutex);
			if (upload_result.failed) {
				ImGui::OpenPopup("upload error");
				upload_result.failed = false;
			} else if (upload_result.succeed) {
				ImGui::OpenPopup("upload succeed");
				upload_result.succeed = false;
			}
			if (ImGui::BeginPopup("upload error")) {
				ImGui::Text("upload failed");
				ImGui::Text("%s",
					    upload_result.err_msg.c_str());
				if (ImGui::Button("copy")) {
					ImGui::SetClipboardText(
						upload_result.err_msg.c_str());
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			} else if (ImGui::BeginPopup("upload succeed")) {
				ImGui::Text("uploaded successfuly");
				ImGui::Text("%s",
					    upload_result.result_url.c_str());
				if (ImGui::Button("copy")) {
					ImGui::SetClipboardText(
						upload_result.result_url
							.c_str());
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w,
			     clear_color.y * clear_color.w,
			     clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
