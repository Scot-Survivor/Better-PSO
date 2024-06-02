// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_sdl2_opengl3/ folder**
// See imgui_impl_sdl2.cpp for details.

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include "implot.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>


double fitness_function(double x, double y) {
    return std::pow(x, 2) * std::pow(y, 2) - 24 * x * y + std::pow(y, 2) - (8 * y) + 160;
}

struct Particle {
    double x;
    double y;
    double best_x;
    double best_y;
    double best_fitness;
};

struct AppConfig {
    int n_particles = 5;
    float cognitive_factor = 0.5;
    float social_factor = 0.8;
    float inertia_weight = 0.5;

    int min_x = -64.0;
    int max_x = 64.0;

    int min_y = -64.0;
    int max_y = 64.0;

    int max_iterations = 1000;


    double global_best_x;
    double global_best_y;
    double global_best_fitness = 1e12;
};

struct UpdateCycle {
    Particle *particles;
    int iterations;
};


void update_particles(UpdateCycle* cycle, AppConfig* config) {
    Particle* particles = cycle->particles;
    for (int i = 0; i < config->n_particles; i++) {

        double cognitive_component_x = config->cognitive_factor * (particles[i].best_x - particles[i].x);
        double cognitive_component_y = config->cognitive_factor * (particles[i].best_y - particles[i].y);

        double social_component_x = config->social_factor * (config->global_best_x - particles[i].x);
        double social_component_y = config->social_factor * (config->global_best_y - particles[i].y);

        double new_x = particles[i].x + config->inertia_weight * cognitive_component_x + social_component_x;
        double new_y = particles[i].y + config->inertia_weight * cognitive_component_y + social_component_y;

        printf("Particle %d: x = %f, y = %f, new_x = %f, new_y = %f\n", i, particles[i].x, particles[i].y, new_x, new_y);

        double new_fitness = fitness_function(new_x, new_y);
        if (new_fitness < particles[i].best_fitness) {
            particles[i].best_x = new_x;
            particles[i].best_y = new_y;
            particles[i].best_fitness = new_fitness;
        }
        if (new_fitness < config->global_best_fitness) {
            config->global_best_x = new_x;
            config->global_best_y = new_y;
            config->global_best_fitness = new_fitness;
        }
        particles[i].x = new_x;
        particles[i].y = new_y;
    }
    cycle->iterations++;
}


void print_particles(Particle* particles, int n_particles) {
    for (int i = 0; i < n_particles; i++) {
        printf("Particle %d: x = %f, y = %f, best_x = %f, best_y = %f, best_fitness = %f\n", i, particles[i].x, particles[i].y, particles[i].best_x, particles[i].best_y, particles[i].best_fitness);
    }
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
    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    AppConfig config;
    Particle particles[config.n_particles];
    UpdateCycle cycle = {particles, 0};

    for (int i = 0; i < config.n_particles; i++) {
        particles[i].x = config.min_x + (double) (rand()) / ((double) (RAND_MAX / (config.max_x - config.min_x)));
        particles[i].y = config.min_y + (double) (rand()) / ((double) (RAND_MAX / (config.max_y - config.min_y)));
        particles[i].best_x = particles[i].x;
        particles[i].best_y = particles[i].y;
        particles[i].best_fitness = fitness_function(particles[i].x, particles[i].y);
        if (particles[i].best_fitness < config.global_best_fitness) {
            config.global_best_x = particles[i].best_x;
            config.global_best_y = particles[i].best_y;
            config.global_best_fitness = particles[i].best_fitness;
        }
    }

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
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Particle Swarm Optimisation", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                                             ImGuiWindowFlags_NoCollapse |ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse |
                                                             ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground);


        ImPlot::SetNextAxesLimits(config.min_x, config.max_x, config.min_y, config.max_y);

        if (ImPlot::BeginPlot("PSO", "X", "Y", ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y),
                              ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoTitle | ImPlotFlags_NoFrame)) {
            // ImPlot::SetupAxesLimits(config.min_x, config.max_x, config.min_y, config.max_y);
            double* xs = new double[config.n_particles];
            double* ys = new double[config.n_particles];
            for (int i = 0; i < config.n_particles; i++) {
                xs[i] = cycle.particles[i].x;
                ys[i] = cycle.particles[i].y;
            }
            ImPlot::PlotScatter("Particles", xs, ys, config.n_particles);
            ImPlot::EndPlot();
        }
        ImGui::End();

        if (ImGui::GetFrameCount() % (int)(ImGui::GetIO().Framerate * 2) == 0 || ImGui::GetFrameCount() == 0) {
            update_particles(&cycle, &config);
        }


        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
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
