#pragma once

namespace ps::ecsModule::ui {
    struct InteractionComponent {
        enum class Type {
            None,
            Hovered,
            Clicked
        };

        Type type;
    };
}