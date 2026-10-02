#include "application.hpp"
#include "graph.hpp"

int main(void) {
    auto app = npk::application();

    auto graph = npk::graph();

    app.render([&] {
        graph.render();
    });
}
