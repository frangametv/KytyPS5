#include "launcherTheme.h"
#include "mainDialog.h"

#include <QApplication>
#include <QQuickStyle>

int main(int argc, char* argv[]) {
	QApplication application(argc, argv);
	application.setApplicationName("KytyPS5");
	QQuickStyle::setStyle("Basic");
	LauncherTheme::Initialize(application);
	MainDialog window;
	window.show();
	return QApplication::exec();
}
