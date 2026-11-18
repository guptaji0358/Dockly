#pragma once

#include <QObject>

namespace LH {
Q_NAMESPACE

enum Category { AllCategories = -1, Software = 0, Document, Entertainment, Web, Collection };
Q_ENUM_NS(Category)

// Not a stored category: SiteViewBase + n is the view of the n-th site (see Sites::viewIds), listing the
// links of all its groups.
constexpr int SiteViewBase = 100;

enum Status { Normal = 0, Warning, Missing };
Q_ENUM_NS(Status)

// How an ItemModel picks its rows.
enum Mode { ModeCategory = 0, ModeFavorites, ModeRecent, ModeMembers };
Q_ENUM_NS(Mode)

enum LaunchResult { Launched = 0, TargetMissing, LaunchFailed };
Q_ENUM_NS(LaunchResult)
} // namespace LH
