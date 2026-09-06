#include "launcherTheme.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QStyleFactory>

void LauncherTheme::Initialize(QApplication& application) {
	const int ui_font = QFontDatabase::addApplicationFont(":/fonts/Roboto.ttf");
	QFontDatabase::addApplicationFont(":/fonts/RobotoMono.ttf");
	if (ui_font >= 0) {
		const auto families = QFontDatabase::applicationFontFamilies(ui_font);
		if (!families.isEmpty()) application.setFont(QFont(families.first(), 10));
	}
	application.setStyle(QStyleFactory::create("Fusion"));
	QPalette palette;
	palette.setColor(QPalette::Window, QColor("#1B1B1B"));
	palette.setColor(QPalette::WindowText, QColor("#F7F8FB"));
	palette.setColor(QPalette::Base, QColor("#161616"));
	palette.setColor(QPalette::AlternateBase, QColor("#202020"));
	palette.setColor(QPalette::Text, QColor("#F7F8FB"));
	palette.setColor(QPalette::Button, QColor("#242424"));
	palette.setColor(QPalette::ButtonText, QColor("#F7F8FB"));
	palette.setColor(QPalette::Highlight, QColor("#B87950"));
	palette.setColor(QPalette::HighlightedText, QColor("#1C1510"));
	palette.setColor(QPalette::ToolTipBase, QColor("#242424"));
	palette.setColor(QPalette::ToolTipText, QColor("#F7F8FB"));
	palette.setColor(QPalette::PlaceholderText, QColor("#777F8E"));
	palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#777F8E"));
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#777F8E"));
	application.setPalette(palette);
	QFile file(":/styles/launcher.qss");
	if (file.open(QIODevice::ReadOnly))
		application.setStyleSheet(QString::fromUtf8(file.readAll()));
}
