//
// Created by jkshi on 27/10/2025.
//
#include <stack>
#include <utility>
#include <fstream>
#include <sstream>

#include "imgui.h"
#include "implot.h"
#include "searchers.h"

namespace algos {
    namespace pso {
        struct Particle {
            double x;
            double y;
            double best_x;
            double best_y;
            double best_fitness;
        };

        struct PSOConfig : AppConfig {
            int n_particles = 5;
            float cognitive_factor = 0.5;
            float social_factor = 0.8;
            float inertia_weight = 0.5;
            float seconds_per_iteration = 1;

        };

        struct UpdateCycle {
            Particle *particles;
            int iterations;
        };

        struct StoredCycle {
            Particle *particles;
            int iterations;
            int n_particles;
        };
    };
    class PSO : public Optimiser {
    private:
        FitnessFunction fitness_function;
        pso::PSOConfig config;
        std::stack<pso::StoredCycle> cycles;

        pso::Particle *initialise_particles(int n_particles, pso::PSOConfig *config) {
            auto* particles = new pso::Particle[n_particles];
            for (int i = 0; i < n_particles; i++) {
                particles[i].x = config->min_x + (double) (rand()) / ((double) (RAND_MAX / (config->max_x - config->min_x)));
                particles[i].y = config->min_y + (double) (rand()) / ((double) (RAND_MAX / (config->max_y - config->min_y)));
                particles[i].best_x = particles[i].x;
                particles[i].best_y = particles[i].y;
                particles[i].best_fitness = fitness_function(particles[i].x, particles[i].y, config);
                if (particles[i].best_fitness < config->global_best_fitness) {
                    config->global_best_x = particles[i].best_x;
                    config->global_best_y = particles[i].best_y;
                    config->global_best_fitness = particles[i].best_fitness;
                }
            }
            return particles;
        };

        pso::StoredCycle create_stored_cycle(pso::Particle* particles, int iterations, int n_particles) {
            auto* new_particles = new pso::Particle[n_particles];
            std::copy(particles, particles + n_particles, new_particles);
            return {new_particles, iterations, n_particles};
        };

        void clear_cycles() {
            while (!this->cycles.empty()) {
                delete[] this->cycles.top().particles;
                this->cycles.pop();
            }
        }
    public:
        PSO(FitnessFunction func, pso::PSOConfig cfg) : config(cfg) {
            this->fitness_function = std::move(func);
            pso::Particle* temp = initialise_particles(config.n_particles, &config);
            cycles.push({temp, 0, config.n_particles});
        };

        void forward_step() override {
            this->step();
        }

        void step() override {
            if (cycles.top().iterations == config.max_iterations) {
#ifndef NDEBUG
                printf("Max iterations reached\n");
#endif
                return;
            }
            pso::StoredCycle next_cycle = create_stored_cycle(cycles.top().particles, cycles.top().iterations+1, this->config.n_particles);
            pso::Particle* particles = next_cycle.particles;
            for (int i = 0; i < config.n_particles; i++) {
                double r1 = (double) (rand()) / ((double) (RAND_MAX));
                double r2 = (double) (rand()) / ((double) (RAND_MAX));

                double cognitive_component_x = config.cognitive_factor * r1 * (particles[i].best_x - particles[i].x);
                double cognitive_component_y = config.cognitive_factor * r1 * (particles[i].best_y - particles[i].y);

                double social_component_x = config.social_factor * r2 * (config.global_best_x - particles[i].x);
                double social_component_y = config.social_factor * r2 * (config.global_best_y - particles[i].y);

                double new_x = particles[i].x + config.inertia_weight + cognitive_component_x + social_component_x;
                double new_y = particles[i].y + config.inertia_weight + cognitive_component_y + social_component_y;

#ifndef NDEBUG
                printf("Particle %d: x = %f, y = %f, new_x = %f, new_y = %f\n", i, particles[i].x, particles[i].y, new_x, new_y);
#endif
                double new_fitness = fitness_function(new_x, new_y, &this->config);
                if (new_fitness < particles[i].best_fitness) {
                    particles[i].best_x = new_x;
                    particles[i].best_y = new_y;
                    particles[i].best_fitness = new_fitness;
                }
                if (new_fitness < config.global_best_fitness) {
                    config.global_best_x = new_x;
                    config.global_best_y = new_y;
                    config.global_best_fitness = new_fitness;
                }
                particles[i].x = new_x;
                particles[i].y = new_y;
            }

            cycles.push( next_cycle);
        };

        void backward_step() override {
            if (cycles.size() > 1) {
                delete[] cycles.top().particles;
                cycles.pop();
            }
        };

        void reset() override {
            this->clear_cycles();

            pso::Particle* temp = initialise_particles(config.n_particles, &config);
            cycles.push(create_stored_cycle(temp, 0, config.n_particles));
            delete[] temp;
            config.global_best_x = 0;
            config.global_best_y = 0;
            config.global_best_fitness = 1e12;
        };

        void save_to_file(const std::string &filename) override {
            FILE* file = fopen(filename.c_str(), "w");
            if (file == nullptr) {
                printf("Error opening file\n");
                return;
            }
            // Save config
            fprintf(file, "n_particles,%d\n", config.n_particles);
            fprintf(file, "cognitive_factor,%f\n", config.cognitive_factor);
            fprintf(file, "social_factor,%f\n", config.social_factor);
            fprintf(file, "inertia_weight,%f\n", config.inertia_weight);
            fprintf(file, "seconds_per_iteration,%f\n", config.seconds_per_iteration);
            fprintf(file, "min_x,%d\n", config.min_x);
            fprintf(file, "max_x,%d\n", config.max_x);
            fprintf(file, "min_y,%d\n", config.min_y);
            fprintf(file, "max_y,%d\n", config.max_y);
            fprintf(file, "max_iterations,%d\n", config.max_iterations);
            fprintf(file, "goal_x,%f\n", config.goal_x);
            fprintf(file, "goal_y,%f\n", config.goal_y);
            fprintf(file, "\n\n\n\n");

            std::vector<pso::StoredCycle> temp;

            while (!cycles.empty()) {
                temp.push_back(cycles.top());
                cycles.pop();
            }

            for (int i = temp.size() - 1; i >= 0; i--) {
                for (int j = 0; j < temp[i].n_particles; j++) {
                    fprintf(file, "%f,%f", temp[i].particles[j].x, temp[i].particles[j].y);
                    if (j != temp[i].n_particles - 1) {
                        fprintf(file, ",");
                    }
                }
                fprintf(file, "\n");
            }

            fclose(file);
            for (auto i : temp) {
                cycles.push(i);
            }
        };

        void load_from_file(const std::string &filename) override {
            std::stack<pso::StoredCycle> read_cycles;
            std::fstream file;
            file.open(filename, std::ios::in);
            if (!file.is_open()) {
                printf("Error opening file\n");
                this->cycles = read_cycles;
                return;
            }
            std::string line;
            int iter = 0;

            pso::PSOConfig read_config;

            // Load config
            while (std::getline(file, line)){
                if (line.empty() && line[0] != '\n') {
                    break;
                }
                std::string key = line.substr(0, line.find(","));
                std::string value = line.substr(line.find(",") + 1);
                if (key == "n_particles") {
                    read_config.n_particles = std::stoi(value);
                } else if (key == "cognitive_factor") {
                    read_config.cognitive_factor = std::stof(value);
                } else if (key == "social_factor") {
                    read_config.social_factor = std::stof(value);
                } else if (key == "inertia_weight") {
                    read_config.inertia_weight = std::stof(value);
                } else if (key == "seconds_per_iteration") {
                    read_config.seconds_per_iteration = std::stof(value);
                } else if (key == "min_x") {
                    read_config.min_x = std::stoi(value);
                } else if (key == "max_x") {
                    read_config.max_x = std::stoi(value);
                } else if (key == "min_y") {
                    read_config.min_y = std::stoi(value);
                } else if (key == "max_y") {
                    read_config.max_y = std::stoi(value);
                } else if (key == "max_iterations") {
                    read_config.max_iterations = std::stoi(value);
                } else if (key == "goal_x") {
                    read_config.goal_x = std::stof(value);
                } else if (key == "goal_y") {
                    read_config.goal_y = std::stof(value);
                }
            }

            // Load data
            while (std::getline(file, line)) {
                // Skip empty lines
                if (line.empty()) {
                    continue;
                }

                int i = 0;
                auto* particles = new pso::Particle[read_config.n_particles];
                std::string token;
                std::istringstream tokenStream(line);
                while (std::getline(tokenStream, token, ',')) {
                    if (i % 2 == 0) {
                        particles[i / 2].x = std::stof(token);
                    } else {
                        particles[i / 2].y = std::stof(token);
                    }
                    i++;
                }
                read_cycles.push({particles, iter, read_config.n_particles});
                iter++;
            }

            file.close();
            this->cycles = read_cycles;
            this->config = read_config;
        };

        void display_config_window() override {
            ImGui::InputInt("Number of Particles", &config.n_particles, 1, 1000);
            // If this changes we must reset cycles and reinitialise particles
            if (config.n_particles != cycles.top().n_particles) {
                if (config.n_particles > 0) {
                    clear_cycles();
                    pso::Particle* temp = initialise_particles(config.n_particles, &config);
                    cycles.push(create_stored_cycle(temp, 0, config.n_particles));
                    delete[] temp;
                }
            }

            ImGui::SliderFloat("Cognitive Factor", &config.cognitive_factor, 0.0, 1.0);
            ImGui::SliderFloat("Social Factor", &config.social_factor, 0.0, 1.0);
            ImGui::SliderFloat("Inertia Weight", &config.inertia_weight, 0.0, 1.0);
            ImGui::InputFloat("Seconds per Iteration", &config.seconds_per_iteration, 0.1, 10.0);
            ImGui::InputInt("Min X", &config.min_x, -100.0, 100.0);
            ImGui::InputInt("Max X", &config.max_x, -100.0, 100.0);
            ImGui::InputInt("Min Y", &config.min_y, -100.0, 100.0);
            ImGui::InputInt("Max Y", &config.max_y, -100.0, 100.0);
            ImGui::InputInt("Max Iterations", &config.max_iterations, 1, 10000);
        };

        AppConfig get_config() override {
            return config;
        };

        std::string get_title() override {
            return "Global Best Fitness: " + std::to_string(config.global_best_fitness) + " Iterations: " +
                std::to_string(cycles.top().iterations+1) + "/" +
                std::to_string(cycles.size()) + "("  + std::to_string(config.max_iterations) + ")";
        };

        void plot() override {
            std::string title = this->get_title();
            ImPlot::SetNextAxesLimits(config.min_x, config.max_x, config.min_y, config.max_y);
            if (ImPlot::BeginPlot(title.c_str(), "X", "Y", ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y),
                                   ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoFrame)) {

                auto* xs = new double[config.n_particles];
                auto* ys = new double[config.n_particles];
                for (int i = 0; i < config.n_particles; i++) {
                    xs[i] = cycles.top().particles[i].x;
                    ys[i] = cycles.top().particles[i].y;
                }

                ImPlot::PlotScatter("Particles", xs, ys, config.n_particles);

                ys[0] = config.goal_y;
                xs[0] = config.goal_x;
                ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(1, 0, 0, 1));
                ImPlot::PlotScatter("Goal", xs, ys, 1);
                ImPlot::PopStyleColor();

                if (ImGui::GetIO().MouseClicked[1]) {
                    config.goal_x = ImPlot::GetPlotMousePos().x;
                    config.goal_y = ImPlot::GetPlotMousePos().y;
                }

                ImPlot::EndPlot();
                                   };
        };

        bool should_step() {
            return ImGui::GetFrameCount() % (int)(ImGui::GetIO().Framerate * config.seconds_per_iteration) == 0 ||
                    ImGui::GetFrameCount() == 0;
        }
    };
}