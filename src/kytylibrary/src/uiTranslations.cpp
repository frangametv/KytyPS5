#include "uiTranslations.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QRegularExpression>

#include <algorithm>

UiTranslations& UiTranslations::Instance() {
	static auto* instance = new UiTranslations();
	return *instance;
}
UiTranslations::UiTranslations(): QTranslator(QCoreApplication::instance()) {
	for (const auto& fileName:
	     QDir(":/translations").entryList({"*.json"}, QDir::Files, QDir::Name)) {
		QFile file(":/translations/" + fileName);
		if (!file.open(QIODevice::ReadOnly)) continue;
		const auto catalog = QJsonDocument::fromJson(file.readAll()).object();
		const auto code    = catalog.value("code").toString();
		const auto label   = catalog.value("label").toString();
		if (!QRegularExpression("^[a-z]{2,3}(-[A-Za-z0-9]+)*$").match(code).hasMatch() ||
		    label.isEmpty() || !catalog.value("translations").isObject() ||
		    m_catalogs.contains(code))
			continue;
		m_catalogs.insert(code, catalog.value("translations").toObject());
		m_languages.append(QVariantMap {{"code", code}, {"label", label}});
	}
	std::stable_sort(m_languages.begin(), m_languages.end(),
	                 [](const QVariant& a, const QVariant& b) {
		                 const auto left  = a.toMap();
		                 const auto right = b.toMap();
		                 if (left.value("code") == "en") return right.value("code") != "en";
		                 if (right.value("code") == "en") return false;
		                 return left.value("label").toString() < right.value("label").toString();
	                 });
	QCoreApplication::installTranslator(this);
}
bool UiTranslations::SetLanguage(const QString& code) {
	const auto selected = m_catalogs.contains(code) ? code : QStringLiteral("en");
	if (selected == m_language) return false;
	QCoreApplication::removeTranslator(this);
	m_language = selected;
	QCoreApplication::installTranslator(this);
	return true;
}
QString UiTranslations::translate(const char*, const char* source, const char*, int n) const {
	const auto value = m_catalogs.value(m_language).value(QString::fromUtf8(source));
	if (value.isObject()) return value.toObject().value(n == 1 ? "one" : "other").toString();
	return value.toString();
}
