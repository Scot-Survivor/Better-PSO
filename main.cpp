#define IMGUI_USER_CONFIG "../include/imconfig.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include "implot.h"
#include "imgui_internal.h"
#include <cstdio>
#include <SDL.h>
#include <SDL_opengl.h>
#include <string>
#include <cmath>
#include "pso.cpp"

/*
 * Fitness function, the lower the better
 * hence why we use the euclidean distance
 */
double fitness_function(double x, double y, algos::AppConfig* config) {
    return std::sqrt(std::pow(x - config->goal_x, 2) + std::pow(y - config->goal_y, 2));
}


// Main code
int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Particle Swarm Optimisation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    io.ConfigViewportsNoAutoMerge = true;
    io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool chosen_optimiser = false;
    algos::Optimiser optimiser;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    /*algos::pso::PSOConfig config = algos::pso::PSOConfig();
    optimiser = algos::PSO(fitness_function, config); */
    char filename[1024] = "cycles.csv";

    bool do_pso = false;

    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RIGHT)
                optimiser.forward_step();

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_LEFT)
                optimiser.backward_step();

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
                do_pso = !do_pso;

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r)
                optimiser.reset();
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();


        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
        if (ImGui::GetFrameCount() == 1) {
            ImGui::SetNextWindowFocus();
        }
        if (!chosen_optimiser) {
            ImGui::Begin("Optimisation Picker", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                                                  ImGuiWindowFlags_NoScrollWithMouse
                                                                 | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
            ImGui::ShowDemoWindow();
            ImGui::End();
        }
        else {
            ImGui::Begin("Optimisation Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                                                  ImGuiWindowFlags_NoScrollWithMouse
                                                                 | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);

            optimiser.plot();
            ImGui::End();

            ImGui::Begin("Controls");
            std::string button_text = do_pso ? "Stop PSO" : "Start PSO";
            if (ImGui::Button(button_text.c_str())) {
                do_pso = !do_pso;
            }
            ImGui::SameLine();

            if (ImGui::Button("Reset")) {
                optimiser.reset();
            }

            ImGui::SameLine();
            // Backward arrow
            if (ImGui::Button("<")) {
                optimiser.backward_step();
            }
            ImGui::SameLine();
            // Forward arrow
            if (ImGui::Button(">")) {
                optimiser.forward_step();
            }

            ImGui::InputText("Filename", filename, 1024);

            if (ImGui::Button("Save")) {
                optimiser.save_to_file(filename);
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                optimiser.load_from_file(filename);
            }

            ImGui::End();

            ImGui::Begin("Configuration");
            optimiser.display_config_window();
            ImGui::End();

            if ( do_pso && optimiser.should_step()) {
                optimiser.forward_step();
                    }

            if (ImGui::GetFrameCount() == 1) {
                ImGuiID parent_node = ImGui::DockBuilderAddNode();

                ImVec2 size_dockspace = ImVec2{0.35, 0.35} * ImGui::GetMainViewport()->Size;
                ImGui::DockBuilderSetNodeSize(parent_node, size_dockspace);

                // place at bottom right corner
                ImVec2 pos = ImGui::GetMainViewport()->Pos + ImGui::GetMainViewport()->Size - (size_dockspace + ImVec2{0.1, 0.1});

                ImGui::DockBuilderSetNodePos(parent_node, pos);

                ImGuiID nodeA;
                ImGuiID nodeB;
                ImGui::DockBuilderSplitNode(parent_node, ImGuiDir_Up, 0.30f,
                                            &nodeB, &nodeA);

                ImGui::DockBuilderDockWindow("Controls", nodeB);
                ImGui::DockBuilderDockWindow("Configuration", nodeA);

                ImGui::DockBuilderFinish(parent_node);
            }
        }


        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
