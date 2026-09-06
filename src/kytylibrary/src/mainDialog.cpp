#include "mainDialog.h"

#include "libraryController.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QGuiApplication>
#include <QLibrary>
#include <QPainterPath>
#include <QRegion>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWidget>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

namespace {
QByteArray last_geometry;
bool       check_updates = true;
} // namespace

MainDialog::MainDialog(QWidget* parent): QDialog(parent) {
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint |
	               Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
	setWindowTitle("KytyPS5");
	setMinimumSize(980, 640);
	resize(1280, 820);
	m_controller = new LibraryController(this);
	if (!last_geometry.isEmpty()) restoreGeometry(last_geometry);
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	m_quick = new QQuickWidget(this);
	m_quick->setResizeMode(QQuickWidget::SizeRootObjectToView);
	m_quick->setClearColor(QColor("#0C0C0C"));
	m_quick->rootContext()->setContextProperty("library", m_controller);
	m_quick->rootContext()->setContextProperty("windowChrome", this);
	m_quick->setSource(QUrl("qrc:/library/Library.qml"));
	layout->addWidget(m_quick);
	UpdateWindowShape();
	if (m_quick->status() == QQuickWidget::Error) {
		QStringList errors;
		for (const auto& error: m_quick->errors())
			errors.append(error.toString());
		QTimer::singleShot(0, this, [this, errors] {
			QMessageBox::critical(this, "Cannot load library", errors.join('\n'));
			close();
		});
	}
	QTimer::singleShot(0, m_controller, [this] { m_controller->Initialize(); });
}

MainDialog::~MainDialog() {
	// Tear down bindings while their controller and models are still alive.
	delete m_quick;
	delete m_controller;
}

void MainDialog::closeEvent(QCloseEvent* event) {
	auto* settings = m_quick->rootObject()
	                     ? m_quick->rootObject()->findChild<QObject*>("settingsPage")
	                     : nullptr;
	if (settings && settings->property("dirty").toBool() &&
	    QMessageBox::question(this, "Unsaved changes", "Discard your unsaved settings and close?",
	                          QMessageBox::Discard | QMessageBox::Cancel,
	                          QMessageBox::Cancel) != QMessageBox::Discard) {
		event->ignore();
		return;
	}
	if (settings) settings->setProperty("dirty", false);
	if (!m_controller->RequestClose()) {
		event->ignore();
		return;
	}
	last_geometry = saveGeometry();
	m_controller->SaveGeometry();
	event->accept();
}

void MainDialog::resizeEvent(QResizeEvent* event) {
	QDialog::resizeEvent(event);
	UpdateWindowShape();
	last_geometry = saveGeometry();
}

void MainDialog::UpdateWindowShape() {
#ifdef Q_OS_WIN
	// Region-based rounding prevents DWM from supplying native corners and shadows.
	clearMask();
	return;
#endif
	if (isMaximized() || isFullScreen()) {
		clearMask();
		return;
	}
	// QWidget uses logical pixels, so the radius also follows the display scale.
	// Qt clips the entire window, including the Quick header and its buttons.
	QPainterPath outline;
	outline.addRoundedRect(QRectF(rect()), 10.0, 10.0);
	setMask(QRegion(outline.toFillPolygon().toPolygon()));
}

void MainDialog::showEvent(QShowEvent* event) {
	InitializeNativeFrame();
	QDialog::showEvent(event);
}

void MainDialog::InitializeNativeFrame() {
#ifdef Q_OS_WIN
	if (QGuiApplication::platformName() != "windows") return;
	const auto hwnd = reinterpret_cast<HWND>(winId());
	m_native_frame = true;
	const auto style = GetWindowLongPtrW(hwnd, GWL_STYLE);
	SetWindowLongPtrW(hwnd, GWL_STYLE, style | WS_CAPTION | WS_THICKFRAME |
	                                     WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
	using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
	using ExtendFrame = HRESULT(WINAPI*)(HWND, const MARGINS*);
	static QLibrary dwm(QStringLiteral("dwmapi"));
	const auto setAttribute = reinterpret_cast<SetAttribute>(dwm.resolve("DwmSetWindowAttribute"));
	const auto extendFrame = reinterpret_cast<ExtendFrame>(dwm.resolve("DwmExtendFrameIntoClientArea"));
	if (setAttribute) {
		const DWORD round = 2; // DWMWCP_ROUND (Windows 11; ignored by older versions).
		const BOOL enabled = TRUE;
		const BOOL disabled = FALSE;
		setAttribute(hwnd, 33, &round, sizeof(round));
		setAttribute(hwnd, 20, &enabled, sizeof(enabled)); // Immersive dark frame.
		setAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED, &disabled, sizeof(disabled));
	}
	if (extendFrame) {
		const MARGINS margins {1, 1, 1, 1};
		extendFrame(hwnd, &margins);
	}
	SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
	             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#endif
}

#ifdef Q_OS_WIN
bool MainDialog::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
	const auto* msg = static_cast<MSG*>(message);
	if (m_native_frame && msg->message == WM_NCCALCSIZE && msg->wParam) {
		// Keep the native window styles while drawing our integrated title bar.
		if (IsZoomed(msg->hwnd)) {
			MONITORINFO monitor {sizeof(MONITORINFO)};
			if (GetMonitorInfoW(MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST), &monitor)) {
				auto* bounds = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
				bounds->rgrc[0] = monitor.rcWork;
			}
		}
		*result = 0;
		return true;
	}
	if (m_native_frame && msg->message == WM_GETMINMAXINFO) {
		MONITORINFO monitor {sizeof(MONITORINFO)};
		if (GetMonitorInfoW(MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST), &monitor)) {
			auto* limits = reinterpret_cast<MINMAXINFO*>(msg->lParam);
			limits->ptMaxPosition = {monitor.rcWork.left - monitor.rcMonitor.left,
			                        monitor.rcWork.top - monitor.rcMonitor.top};
			limits->ptMaxSize = {monitor.rcWork.right - monitor.rcWork.left,
			                    monitor.rcWork.bottom - monitor.rcWork.top};
			limits->ptMinTrackSize = {LONG(minimumWidth() * devicePixelRatioF()),
			                         LONG(minimumHeight() * devicePixelRatioF())};
			*result = 0;
			return true;
		}
	}
	return QDialog::nativeEvent(eventType, message, result);
}
#endif

void MainDialog::moveEvent(QMoveEvent* event) {
	QDialog::moveEvent(event);
	last_geometry = saveGeometry();
}

void MainDialog::WriteSettings(QSettings& settings) {
	settings.beginGroup("MainDialog");
	if (!last_geometry.isEmpty()) settings.setValue("geometry", last_geometry);
	settings.setValue("check_updates_on_startup", check_updates);
	settings.endGroup();
}
void MainDialog::ReadSettings(QSettings& settings) {
	settings.beginGroup("MainDialog");
	last_geometry = settings.value("geometry").toByteArray();
	check_updates = settings.value("check_updates_on_startup", true).toBool();
	settings.endGroup();
}
bool MainDialog::CheckUpdatesOnStartup() {
	return check_updates;
}

void MainDialog::minimizeWindow() {
#ifdef Q_OS_WIN
	if (m_native_frame) {
		SendMessageW(reinterpret_cast<HWND>(winId()), WM_SYSCOMMAND, SC_MINIMIZE, 0);
		return;
	}
#endif
	showMinimized();
}

void MainDialog::toggleMaximized() {
#ifdef Q_OS_WIN
	if (m_native_frame) {
		SendMessageW(reinterpret_cast<HWND>(winId()), WM_SYSCOMMAND,
		             isMaximized() ? SC_RESTORE : SC_MAXIMIZE, 0);
		return;
	}
#endif
	if (isMaximized())
		showNormal();
	else
		showMaximized();
}

void MainDialog::closeWindow() {
	close();
}

bool MainDialog::beginWindowMove() {
	return windowHandle() && windowHandle()->startSystemMove();
}

bool MainDialog::beginWindowResize(int edges) {
	const auto sides = Qt::Edges(edges);
	const bool valid = sides == Qt::LeftEdge || sides == Qt::RightEdge || sides == Qt::TopEdge ||
	                   sides == Qt::BottomEdge || sides == (Qt::TopEdge | Qt::LeftEdge) ||
	                   sides == (Qt::TopEdge | Qt::RightEdge) ||
	                   sides == (Qt::BottomEdge | Qt::LeftEdge) ||
	                   sides == (Qt::BottomEdge | Qt::RightEdge);
	return valid && !isMaximized() && windowHandle() && windowHandle()->startSystemResize(sides);
}

void MainDialog::changeEvent(QEvent* event) {
	QDialog::changeEvent(event);
	if (event->type() == QEvent::WindowStateChange) {
		UpdateWindowShape();
		emit WindowStateChanged();
	}
}
void MainDialog::SetCheckUpdatesOnStartup(bool checked) {
	check_updates = checked;
}
