#ifndef FIGURER
#define FIGURER

#include <functional>
#include <vector>

namespace figurer {

    class Context {
        int state_size_;
        int actuation_size_;
        std::function<double(std::vector<double>)> value_fn_;
    public:
        Context();
        ~Context();
        void set_state_size(int state_size);
        void set_actuation_size(int actuation_size);
        void set_value_fn(std::function<double(std::vector<double>)> value_fn);

        double apply_value_fn(std::vector<double> state);
    };

}

#endif
