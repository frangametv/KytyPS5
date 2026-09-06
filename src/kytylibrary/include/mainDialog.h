#pragma once
#include "common.h"

#include <QDialog>
class QSettings;
class LibraryController;
class QQuickWidget;
class MainDialog: public QDialog {
	Q_OBJECT
	Q_PROPERTY(bool maximized READ IsMaximized NOTIFY WindowStateChanged)
	KYTY_QT_CLASS_NO_COPY(MainDialog);

public:
	explicit MainDialog(QWidget* parent = nullptr);
	~MainDialog() override;
	static void      WriteSettings(QSettings& settings);
	static void      ReadSettings(QSettings& settings);
	static bool      CheckUpdatesOnStartup();
	static void      SetCheckUpdatesOnStartup(bool checked);
	bool             IsMaximized() const { return isMaximized(); }
	Q_INVOKABLE void minimizeWindow();
	Q_INVOKABLE void toggleMaximized();
	Q_INVOKABLE void closeWindow();
	Q_INVOKABLE bool beginWindowMove();
	Q_INVOKABLE bool beginWindowResize(int edges);

signals:
	void WindowStateChanged();

protected:
	void showEvent(QShowEvent* event) override;
#ifdef Q_OS_WIN
	bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif
	void closeEvent(QCloseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void moveEvent(QMoveEvent* event) override;
	void changeEvent(QEvent* event) override;
	void reject() override {}

private:
	void InitializeNativeFrame();
	bool m_native_frame = false;
	void UpdateWindowShape();
	LibraryController* m_controller = nullptr;
	QQuickWidget*      m_quick      = nullptr;
};
