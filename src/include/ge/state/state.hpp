#ifndef STATE_HPP
#define STATE_HPP

namespace ge {
    struct Data;

    class State {
    public:
        State(Data *data): data(data){}
        virtual ~State(){}

        virtual void update() = 0;
        virtual void render() = 0;

    protected:
        Data *data;
    };
}

#endif // !STATE_HPP

#include "handler.hpp"