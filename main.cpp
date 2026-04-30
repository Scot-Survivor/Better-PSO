#define IMGUI_USER_CONFIG "../include/imconfig.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "imgui_internal.h"

#include <cmath>
#include <cstdio>
#include <string>

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include "pso.cpp"

/*
 * Fitness function, the lower the better
 * hence why we use the euclidean distance
 */
double fitness_function(double x, double y, algos::AppConfig* config) {
    return std::sqrt(std::pow(x - config->goal_x, 2) + std::pow(y - config->goal_y, 2));
}

struct AppState {
    SDL_Window* window = nullptr;
    SDL_GLContext gl_context = nullptr;
    bool chosen_optimiser = false;
    algos::Optimiser* optimiser = nullptr;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    char filename[1024] = "cycles.csv";
    bool do_pso = false;
    bool done = false;
};

static void handle_events(AppState& state)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT)
            state.done = true;

        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(state.window))
            state.done = true;

        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RIGHT)
            if (state.optimiser != nullptr) state.optimiser->forward_step();

        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_LEFT)
            if (state.optimiser != nullptr) state.optimiser->backward_step();

        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE)
            state.do_pso = !state.do_pso;

        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r)
            if (state.optimiser != nullptr) state.optimiser->reset();
    }
}

static void draw_frame(AppState& state)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
    if (ImGui::GetFrameCount() == 1) {
        ImGui::SetNextWindowFocus();
    }

    if (!state.chosen_optimiser) {
        ImGui::Begin("Optimisation Picker", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                                     ImGuiWindowFlags_NoScrollWithMouse |
                                                     ImGuiWindowFlags_NoDecoration);
        if (ImGui::CollapsingHeader("Particle Swarm Optimisation (PSO)")) {
            ImGui::TextWrapped("%s", "Particle Swarm Optimisation (PSO) is a computational method that optimizes a problem by iteratively trying to improve a candidate solution with regard to a given measure of quality. It solves problems by having a population of candidate solutions, here dubbed particles, and moving these particles around in the search-space according to simple mathematical formulae over the particle's position and velocity. Each particle's movement is influenced by its local best known position, but is also guided toward the best known positions in the search-space, which are updated as better positions are found by other particles. This is expected to move the swarm toward the best solutions.");
            if (ImGui::Button("Select PSO")) {
                state.optimiser = new algos::PSO(fitness_function, algos::pso::PSOConfig());
                state.chosen_optimiser = true;
            }
        }
        ImGui::End();
    }
    else {
        if (state.optimiser == nullptr) {
            return;
        }

        ImGui::Begin("Optimisation Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                                     ImGuiWindowFlags_NoScrollWithMouse |
                                                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration
                                                     | ImGuiWindowFlags_NoBringToFrontOnFocus);

        state.optimiser->plot();
        ImGui::End();

        ImGui::Begin("Controls");
        std::string button_text = state.do_pso ? "Stop PSO" : "Start PSO";
        if (ImGui::Button(button_text.c_str())) {
            state.do_pso = !state.do_pso;
        }
        ImGui::SameLine();

        if (ImGui::Button("Reset")) {
            state.optimiser->reset();
        }

        ImGui::SameLine();
        if (ImGui::Button("<")) {
            state.optimiser->backward_step();
        }
        ImGui::SameLine();
        if (ImGui::Button(">")) {
            state.optimiser->forward_step();
        }

        ImGui::InputText("Filename", state.filename, 1024);

        if (ImGui::Button("Save")) {
            state.optimiser->save_to_file(state.filename);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            state.optimiser->load_from_file(state.filename);
        }

        ImGui::End();

        ImGui::Begin("Configuration");
        state.optimiser->display_config_window();
        ImGui::End();

        if (state.do_pso && state.optimiser->should_step()) {
            state.optimiser->forward_step();
        }

        if (ImGui::GetFrameCount() == 1) {
            ImGuiID parent_node = ImGui::DockBuilderAddNode();

            ImVec2 size_dockspace = ImVec2{0.35f, 0.35f} * ImGui::GetMainViewport()->Size;
            ImGui::DockBuilderSetNodeSize(parent_node, size_dockspace);

            ImVec2 pos = ImGui::GetMainViewport()->Pos + ImGui::GetMainViewport()->Size - (size_dockspace + ImVec2{0.1f, 0.1f});
            ImGui::DockBuilderSetNodePos(parent_node, pos);

            ImGuiID nodeA;
            ImGuiID nodeB;
            ImGui::DockBuilderSplitNode(parent_node, ImGuiDir_Up, 0.30f, &nodeB, &nodeA);

            ImGui::DockBuilderDockWindow("Controls", nodeB);
            ImGui::DockBuilderDockWindow("Configuration", nodeA);

            ImGui::DockBuilderFinish(parent_node);
        }
    }

    ImGui::Render();
    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
    glClearColor(state.clear_color.x * state.clear_color.w, state.clear_color.y * state.clear_color.w, state.clear_color.z * state.clear_color.w, state.clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

#ifndef __EMSCRIPTEN__
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
#endif

    SDL_GL_SwapWindow(state.window);
}

static void run_frame(AppState& state)
{
    handle_events(state);
    if (!state.done) {
        draw_frame(state);
    }
}

static bool init_app(AppState& state)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return false;
    }

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    state.window = SDL_CreateWindow("Particle Swarm Optimisation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    1280, 720, window_flags);
    if (state.window == nullptr)
    {
        printf("Error: %s\n", SDL_GetError());
        return false;
    }

    state.gl_context = SDL_GL_CreateContext(state.window);
    if (state.gl_context == nullptr)
    {
        printf("Error: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(state.window, state.gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#ifndef __EMSCRIPTEN__
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigViewportsNoAutoMerge = true;
    io.ConfigViewportsNoTaskBarIcon = true;
#endif

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForOpenGL(state.window, state.gl_context))
    {
        printf("Error: failed to initialize ImGui SDL2 backend\n");
        return false;
    }

#ifdef __EMSCRIPTEN__
    const char* glsl_version = "#version 100";
#else
    const char* glsl_version = "#version 120";
#endif
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        printf("Error: failed to initialize ImGui OpenGL3 backend\n");
        return false;
    }

    return true;
}

static void shutdown_app(AppState& state)
{
    delete state.optimiser;
    state.optimiser = nullptr;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    if (state.gl_context != nullptr)
    {
        SDL_GL_DeleteContext(state.gl_context);
        state.gl_context = nullptr;
    }

    if (state.window != nullptr)
    {
        SDL_DestroyWindow(state.window);
        state.window = nullptr;
    }

    SDL_Quit();
}

#ifdef __EMSCRIPTEN__
static void em_main_loop(void* arg)
{
    auto* state = static_cast<AppState*>(arg);
    run_frame(*state);
    if (state->done)
    {
        emscripten_cancel_main_loop();
    }
}
#endif

// Main code
int main(int, char**)
{
    AppState state;
    if (!init_app(state)) {
        shutdown_app(state);
        return -1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(em_main_loop, &state, 0, true);
    return 0;
#else
    while (!state.done)
    {
        run_frame(state);
    }

    shutdown_app(state);
    return 0;
#endif
}
