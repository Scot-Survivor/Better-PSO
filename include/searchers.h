#ifndef SEARCHERS_H
#define SEARCHERS_H
#include <string>
#include <functional>

namespace algos {
    struct AppConfig {
        int min_x = -64.0;
        int max_x = 64.0;

        int min_y = -64.0;
        int max_y = 64.0;

        int max_iterations = 1000;


        double global_best_x{};
        double global_best_y{};
        double global_best_fitness = 1e12;

        double goal_x = 0;
        double goal_y = 0;
    };
    class Optimiser {
    public:
        virtual ~Optimiser() = default;

        Optimiser() = default;

        virtual void step();
        virtual void forward_step();
        virtual void backward_step();
        virtual void reset();
        virtual void save_to_file(const std::string& filename);
        virtual void load_from_file(const std::string& filename);
        virtual void display_config_window();  // GUI for configuration

        virtual std::string get_title();

        virtual AppConfig get_config();

        virtual void plot();
    };

    typedef std::function<double(double, double, AppConfig*)> FitnessFunction;
}
#endif //SEARCHERS_H