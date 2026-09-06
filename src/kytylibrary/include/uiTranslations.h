#pragma once
#include <QJsonObject>
#include <QMap>
#include <QTranslator>
#include <QVariantList>

class UiTranslations final: public QTranslator {
public:
	static UiTranslations& Instance();
	QVariantList           Languages() const { return m_languages; }
	QString                Language() const { return m_language; }
	bool                   SetLanguage(const QString& code);
	QString translate(const char* context, const char* source, const char* disambiguation = nullptr,
	                  int n = -1) const override;
	bool    isEmpty() const override { return false; }

private:
	UiTranslations();
	QVariantList               m_languages;
	QMap<QString, QJsonObject> m_catalogs;
	QString                    m_language = "en";
};
