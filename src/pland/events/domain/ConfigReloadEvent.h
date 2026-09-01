#pragma once
#include "ll/api/event/Event.h"

namespace land::event {

class ConfigReloadEvent final : public ll::event::Event {
public:
    explicit ConfigReloadEvent() {}
};

} // namespace land::event


namespace land::events::inline infra {

using ConfigReloadEvent [[deprecated("Use land::event::ConfigReloadEvent")]] = event::ConfigReloadEvent;

} // namespace land::events::inline infra