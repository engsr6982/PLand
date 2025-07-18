#include "EmtpyInterface.h"


namespace land ::internals {


EmtpyInterface::EmtpyInterface() = default;

llong EmtpyInterface::get(Player&) const { return 0; }
llong EmtpyInterface::get(mce::UUID const&) const { return 0; }

bool EmtpyInterface::set(Player&, llong) const { return true; }
bool EmtpyInterface::set(mce::UUID const&, llong) const { return true; }

bool EmtpyInterface::add(Player&, llong) const { return true; }
bool EmtpyInterface::add(mce::UUID const&, llong) const { return true; }

bool EmtpyInterface::reduce(Player&, llong) const { return true; }
bool EmtpyInterface::reduce(mce::UUID const&, llong) const { return true; }

bool EmtpyInterface::transfer(Player&, Player&, llong) const { return true; }
bool EmtpyInterface::transfer(mce::UUID const&, mce::UUID const&, llong) const { return true; }


} // namespace land::internals