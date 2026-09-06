#pragma once
#include <QVariantList>
#include <QVariantMap>
class Configuration;
namespace LibrarySettings {
QVariantList Fields(const Configuration& info);
QString      Apply(Configuration& info, const QVariantMap& values);
} // namespace LibrarySettings
