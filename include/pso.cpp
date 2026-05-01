//
// Created by jkshi on 27/10/2025.
//
#include <stack>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <utility>
#include <fstream>
#include <sstream>
#include <random>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "imgui.h"
#include "implot.h"
#include "searchers.h"

namespace algos {
    namespace pso {
        struct Particle {
            double x;
            double y;
            double vx;
            double vy;
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
            int max_history = 100;
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
        std::mt19937 rng;  // Fast random number generator
        std::uniform_real_distribution<double> uniform_dist;

        double random_between(double min_value, double max_value) {
            return min_value + uniform_dist(rng) * (max_value - min_value);
        }

        pso::Particle *initialise_particles(int n_particles, pso::PSOConfig *settings) {
            auto* particles = new pso::Particle[n_particles];
            for (int i = 0; i < n_particles; i++) {
                particles[i].x = settings->min_x + uniform_dist(rng) * (settings->max_x - settings->min_x);
                particles[i].y = settings->min_y + uniform_dist(rng) * (settings->max_y - settings->min_y);
                particles[i].vx = uniform_dist(rng) * 2 - 1;  // Random velocity in [-1, 1]
                particles[i].vy = uniform_dist(rng) * 2 - 1;
                particles[i].best_x = particles[i].x;
                particles[i].best_y = particles[i].y;
                particles[i].best_fitness = fitness_function(particles[i].x, particles[i].y, settings);
                if (particles[i].best_fitness < settings->global_best_fitness) {
                    settings->global_best_x = particles[i].best_x;
                    settings->global_best_y = particles[i].best_y;
                    settings->global_best_fitness = particles[i].best_fitness;
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

        void trim_cycles() {
            if (config.max_history > 0) {
                while ((int)cycles.size() > config.max_history) {
                    pso::StoredCycle oldest = cycles.top();
                    cycles.pop();
                    if ((int)cycles.size() < (int)config.max_history) {
                        cycles.push(oldest);
                        break;
                    }
                    delete[] oldest.particles;
                }
            }
        }
    public:
        PSO(FitnessFunction func, pso::PSOConfig cfg) : config(cfg), uniform_dist(0.0, 1.0) {
            this->fitness_function = std::move(func);
            rng.seed(std::random_device{}());
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

            #pragma omp parallel for schedule(static)
            for (int i = 0; i < config.n_particles; i++) {
                double r1 = uniform_dist(rng);
                double r2 = uniform_dist(rng);

                particles[i].vx = config.inertia_weight * particles[i].vx +
                                 config.cognitive_factor * r1 * (particles[i].best_x - particles[i].x) +
                                 config.social_factor * r2 * (config.global_best_x - particles[i].x);

                particles[i].vy = config.inertia_weight * particles[i].vy +
                                 config.cognitive_factor * r1 * (particles[i].best_y - particles[i].y) +
                                 config.social_factor * r2 * (config.global_best_y - particles[i].y);

                double new_x = particles[i].x + particles[i].vx;
                double new_y = particles[i].y + particles[i].vy;

#ifndef NDEBUG
                if (i < 3)
                    printf("Particle %d: x = %f, y = %f, new_x = %f, new_y = %f\n", i, particles[i].x, particles[i].y, new_x, new_y);
#endif
                double new_fitness = fitness_function(new_x, new_y, &this->config);
                if (new_fitness < particles[i].best_fitness) {
                    particles[i].best_x = new_x;
                    particles[i].best_y = new_y;
                    particles[i].best_fitness = new_fitness;
                }
                if (new_fitness < config.global_best_fitness) {
                    #pragma omp critical
                    {
                        if (new_fitness < config.global_best_fitness) {
                            config.global_best_x = new_x;
                            config.global_best_y = new_y;
                            config.global_best_fitness = new_fitness;
                        }
                    }
                }
                particles[i].x = new_x;
                particles[i].y = new_y;
            }

            cycles.push(next_cycle);
            trim_cycles();  // Enforce max history limit
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

        void randomize_goal() override {
            config.goal_x = random_between(config.min_x, config.max_x);
            config.goal_y = random_between(config.min_y, config.max_y);
            reset();
        }

        void randomize_bounds() override {
            int center_x = (int)random_between(-64, 64);
            int center_y = (int)random_between(-64, 64);
            int half_width = (int)random_between(16, 64);
            int half_height = (int)random_between(16, 64);

            config.min_x = center_x - half_width;
            config.max_x = center_x + half_width;
            config.min_y = center_y - half_height;
            config.max_y = center_y + half_height;
            config.goal_x = random_between(config.min_x, config.max_x);
            config.goal_y = random_between(config.min_y, config.max_y);
            reset();
        }

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
            ImGui::InputInt("Max History (limit memory)", &config.max_history, 10, 100);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted("Limit stored cycles to save memory. Set to -1 for unlimited.");
                ImGui::EndTooltip();
            }
        };

        const AppConfig& get_config() override {
            return config;
        };

        std::string get_title() override {
            return "Global Best Fitness: " + std::to_string(config.global_best_fitness) + " Iterations: " +
                std::to_string(cycles.top().iterations+1) + "/" +
                std::to_string(cycles.size()) + "("  + std::to_string(config.max_iterations) + ")";
        };

        void plot_history() override {
            std::vector<pso::StoredCycle> ordered_cycles;
            auto temp = cycles;
            while (!temp.empty()) {
                ordered_cycles.push_back(temp.top());
                temp.pop();
            }
            std::reverse(ordered_cycles.begin(), ordered_cycles.end());

            std::vector<double> iterations;
            std::vector<double> best_fitness;
            iterations.reserve(ordered_cycles.size());
            best_fitness.reserve(ordered_cycles.size());

            for (size_t i = 0; i < ordered_cycles.size(); ++i) {
                double best = ordered_cycles[i].particles[0].best_fitness;
                for (int j = 1; j < ordered_cycles[i].n_particles; ++j) {
                    best = std::min(best, ordered_cycles[i].particles[j].best_fitness);
                }
                iterations.push_back((double)i);
                best_fitness.push_back(best);
            }

            if (ImPlot::BeginPlot("Fitness History", ImVec2(-1, 220))) {
                if (!iterations.empty()) {
                    ImPlot::PlotLine("Best Fitness", iterations.data(), best_fitness.data(), (int)iterations.size());
                }
                ImPlot::EndPlot();
            }
        }

        void plot() override {
            std::string title = this->get_title();

            double center_x = 0.0;
            double center_y = 0.0;
            for (int i = 0; i < config.n_particles; ++i) {
                center_x += cycles.top().particles[i].x;
                center_y += cycles.top().particles[i].y;
            }
            center_x /= config.n_particles;
            center_y /= config.n_particles;

            double dx = std::abs(config.goal_x - center_x);
            double dy = std::abs(config.goal_y - center_y);
            double max_distance = std::max(dx, dy);

            double padding = max_distance * 0.5;
            double view_half_width = std::max(max_distance + padding, 30.0);

            double plot_min_x = config.goal_x - view_half_width;
            double plot_max_x = config.goal_x + view_half_width;
            double plot_min_y = config.goal_y - view_half_width;
            double plot_max_y = config.goal_y + view_half_width;

            ImPlot::SetNextAxesLimits(plot_min_x, plot_max_x, plot_min_y, plot_max_y);
            if (ImPlot::BeginPlot(title.c_str(), "X", "Y", ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y),
                                   ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoFrame)) {

                // For large swarms, sample particles to avoid rendering performance issues
                int particles_to_render = config.n_particles;
                int sample_rate = 1;
                if (config.n_particles > 5000) {
                    sample_rate = config.n_particles / 5000;
                    particles_to_render = (config.n_particles + sample_rate - 1) / sample_rate;
                }

                auto* xs = new double[particles_to_render];
                auto* ys = new double[particles_to_render];
                int rendered = 0;
                for (int i = 0; i < config.n_particles; i += sample_rate) {
                    xs[rendered] = cycles.top().particles[i].x;
                    ys[rendered] = cycles.top().particles[i].y;
                    rendered++;
                }

                ImPlot::PlotScatter("Particles", xs, ys, rendered);

                double center_xs[1] = {center_x};
                double center_ys[1] = {center_y};
                ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.20f, 0.50f, 1.00f, 1.00f));
                ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.20f, 0.50f, 1.00f, 1.00f));
                ImPlot::PlotScatter("Swarm Center", center_xs, center_ys, 1);
                ImPlot::PopStyleColor(2);

                xs[0] = config.goal_x;
                ys[0] = config.goal_y;
                ImVec4 gold = ImVec4(1.0f, 0.84f, 0.0f, 1.0f);
                ImPlot::PushStyleColor(ImPlotCol_MarkerFill, gold);
                ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, gold);
                ImPlot::PlotScatter("Goal", xs, ys, 1);
                ImPlot::PopStyleColor(2);

                if (ImGui::GetIO().MouseClicked[1]) {
                    config.goal_x = ImPlot::GetPlotMousePos().x;
                    config.goal_y = ImPlot::GetPlotMousePos().y;
                }

                ImPlot::EndPlot();
                delete[] xs;
                delete[] ys;
                                   };
        };

        bool should_step() override {
            return ImGui::GetFrameCount() % (int)(ImGui::GetIO().Framerate * config.seconds_per_iteration) == 0 ||
                    ImGui::GetFrameCount() == 0;
        }
    };
}